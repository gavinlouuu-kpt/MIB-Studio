#include <iostream>
#include <filesystem>
#include <EGrabber.h>
#include <opencv2/opencv.hpp>
#include <FormatConverter.h>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <conio.h>
#include <chrono>
#include <iomanip>
#include <../CircularBuffer.h>
#include <tuple>

cv::Mat backgroundFrame;

void initializeBackgroundFrame(const GrabberParams &params)
{
    // Create an all-white frame
    backgroundFrame = cv::Mat(static_cast<int>(params.height), static_cast<int>(params.width), CV_8UC1, cv::Scalar(255));
}

using namespace Euresys;

struct GrabberParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

struct SharedResources
{
    std::atomic<bool> done{false};
    std::atomic<bool> paused{false};
    std::atomic<int> currentFrameIndex{-1};
    std::atomic<bool> displayNeedsUpdate{false};
    std::atomic<size_t> frameRateCount{0};
    std::queue<size_t> framesToProcess;
    std::queue<size_t> framesToDisplay;
    std::mutex displayQueueMutex;
    std::mutex processingQueueMutex;
    std::condition_variable displayQueueCondition;
    std::condition_variable processingQueueCondition;
    std::vector < std::tuple<double, double> circularities;
    std::mutex circularitiesMutex;
};

void configure(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.setInteger<RemoteModule>("Width", 512);
    grabber.setInteger<RemoteModule>("Height", 512);
    grabber.setInteger<RemoteModule>("AcquisitionFrameRate", 4700);
    // ... (other configuration settings)
}

void configure_js()
{
    Euresys::EGenTL gentl;
    Euresys::EGrabber<> grabber(gentl);
    grabber.runScript("config.js");
}

GrabberParams initializeGrabber(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.reallocBuffers(3);
    grabber.start(1);
    ScopedBuffer firstBuffer(grabber);

    GrabberParams params;
    params.width = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    params.height = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    params.pixelFormat = firstBuffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    params.imageSize = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    params.bufferCount = 5000; // You can adjust this as needed

    grabber.stop();
    return params;
}

cv::Mat processFrame(const std::vector<uint8_t> &imageData, size_t width, size_t height)
{
    // Blurr background
    cv::Mat blurred_bg = cv::GaussianBlur(backgroundFrame, cv::Size(3, 3), 0);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    // Create OpenCV Mat from the image data
    cv::Mat original(static_cast<int>(height), static_cast<int>(width), CV_8UC1, const_cast<uint8_t *>(imageData.data()));

    // Blur background and target image
    cv::Mat blurred_bg, blurred_target;
    cv::GaussianBlur(backgroundFrame, blurred_bg, cv::Size(3, 3), 0);
    cv::GaussianBlur(original, blurred_target, cv::Size(3, 3), 0);

    // Background subtraction
    cv::Mat bg_sub;
    cv::subtract(blurred_bg, blurred_target, bg_sub);

    // Apply threshold
    cv::Mat binary;
    cv::threshold(bg_sub, binary, 10, 255, cv::THRESH_BINARY);

    // Create kernel for morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    // Erode and dilate to remove noise
    cv::Mat dilate1, erode1, erode2, dilate2;
    cv::dilate(binary, dilate1, kernel, cv::Point(-1, -1), 2);
    cv::erode(dilate1, erode1, kernel, cv::Point(-1, -1), 2);
    cv::erode(erode1, erode2, kernel, cv::Point(-1, -1), 1);
    cv::dilate(erode2, dilate2, kernel, cv::Point(-1, -1), 1);

    // The final processed image is dilate2
    cv::Mat processed = dilate2;

    return processed;
}

struct ContourResult
{
    std::vector<std::vector<cv::Point>> contours;
    double findTime;
};

ContourResult findContours(const cv::Mat &processedImage)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    auto start = std::chrono::high_resolution_clock::now();
    cv::findContours(processedImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    auto end = std::chrono::high_resolution_clock::now();
    double findTime = std::chrono::duration<double, std::milli>(end - start).count();

    return {contours, findTime};
}

std::tuple<double, double> calculateMetrics(const std::vector<cv::Point> &contour)
{
    // use deformability instead of circularity (1-circularity)
    cv::Moments m = cv::moments(contour);
    double area = m.m00;
    double perimeter = cv::arcLength(contour, true);
    double circularity = 0.0;

    if (perimeter > 0)
    {
        circularity = 4 * M_PI * area / (perimeter * perimeter);
    }

    return std::make_tuple(circularity, area);
}

void processingThreadTask(
    std::atomic<bool> &done,
    std::atomic<bool> &paused,
    std::mutex &processingQueueMutex,
    std::condition_variable &processingQueueCondition,
    std::queue<size_t> &framesToProcess,
    const CircularBuffer &circularBuffer,
    size_t width,
    size_t height)
{
    auto lastPrintTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    double totalProcessingTime = 0.0;

    while (!done)
    {
        std::unique_lock<std::mutex> lock(processingQueueMutex);
        processingQueueCondition.wait(lock, [&]()
                                      { return !framesToProcess.empty() || done || paused; });

        if (done)
            break;

        if (!framesToProcess.empty() && !paused)
        {
            size_t frame = framesToProcess.front();
            framesToProcess.pop();
            lock.unlock();

            auto imageData = circularBuffer.get(0);

            // Measure processing time
            auto startTime = std::chrono::high_resolution_clock::now();
            // Preprocess Image
            cv::Mat processedImage = processFrame(imageData, width, height);
            // Find contour
            ContourResult contourResult = findContours(processedImage);
            std::vector<std::vector<cv::Point>> contours = contourResult.contours;
            double contourFindTime = contourResult.findTime;
            // Calculate deformability
            // Calculate circularity for each contour

            std::lock_guard<std::mutex> circularitiesLock(shared.circularitiesMutex);
            for (const auto &contour : contours)
            {
                if (contour.size() >= 5)
                { // Ensure there are enough points to calculate circularity
                    auto [circularity, area] = calculateMetrics(contour);
                    circularities.emplace_back(circularity, area);
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            double processingTime = duration.count(); // Convert to milliseconds
            totalProcessingTime += processingTime;

            // std::cout << "Frame " << frameCount + 1 << " processed in " << processingTime << " ms" << std::endl;

            ++frameCount;

            // Check if 5 seconds have passed
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastPrintTime).count();
            if (elapsedTime >= 5)
            {
                double averageProcessingTime = totalProcessingTime / frameCount;
                std::cout << "Average processing time over last " << frameCount << " frames: "
                          << averageProcessingTime << " us" << std::endl;

                // Reset counters and update last print time
                frameCount = 0;
                totalProcessingTime = 0.0;
                lastPrintTime = currentTime;
            }
        }
        else
        {
            lock.unlock();
        }
    }
}

void onTrackbar(int pos, void *userdata)
{
    auto *shared = static_cast<SharedResources *>(userdata);
    shared->currentFrameIndex = pos;
    shared->displayNeedsUpdate = true;
}

void updateScatterPlot(cv::Mat &plot, const std::vector<std::tuple<double, double>> &circularities, std::mutex &mutex)
{
    std::lock_guard<std::mutex> lock(mutex);

    plot = cv::Scalar(255, 255, 255); // Clear the plot

    if (circularities.empty())
    {
        return;
    }

    // Find the range of values
    double minCirc = std::numeric_limits<double>::max();
    double maxCirc = std::numeric_limits<double>::lowest();
    double minArea = std::numeric_limits<double>::max();
    double maxArea = std::numeric_limits<double>::lowest();

    for (const auto &[circ, area] : circularities)
    {
        minCirc = std::min(minCirc, circ);
        maxCirc = std::max(maxCirc, circ);
        minArea = std::min(minArea, area);
        maxArea = std::max(maxArea, area);
    }

    // Draw axes
    cv::line(plot, cv::Point(50, 350), cv::Point(350, 350), cv::Scalar(0, 0, 0), 2);
    cv::line(plot, cv::Point(50, 350), cv::Point(50, 50), cv::Scalar(0, 0, 0), 2);

    // Draw points
    for (const auto &[circ, area] : circularities)
    {
        int x = 50 + static_cast<int>((circ - minCirc) / (maxCirc - minCirc) * 300);
        int y = 350 - static_cast<int>((area - minArea) / (maxArea - minArea) * 300);
        cv::circle(plot, cv::Point(x, y), 3, cv::Scalar(0, 0, 255), -1);
    }

    // Add labels
    cv::putText(plot, "Circularity", cv::Point(160, 390), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plot, "Area", cv::Point(10, 200), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA, true);
}

void displayThreadTask(
    const std::atomic<bool> &done,
    const std::atomic<bool> &paused,
    const std::atomic<int> &currentFrameIndex,
    std::atomic<bool> &displayNeedsUpdate,
    std::queue<size_t> &framesToDisplay,
    std::mutex &displayQueueMutex,
    const CircularBuffer &circularBuffer,
    size_t width,
    size_t height,
    size_t bufferCount,
    SharedResources &shared)
{
    const std::chrono::duration<double> frameDuration(1.0 / 25.0); // 25 FPS
    auto nextFrameTime = std::chrono::steady_clock::now();

    cv::namedWindow("Live Feed", cv::WINDOW_NORMAL);
    cv::resizeWindow("Live Feed", width, height);

    cv::namedWindow("Processed Feed", cv::WINDOW_NORMAL);
    cv::resizeWindow("Processed Feed", width, height);

    cv::namedWindow("Scatter Plot", cv::WINDOW_NORMAL);
    cv::resizeWindow("Scatter Plot", 400, 400);

    int trackbarPos = 0;
    cv::createTrackbar("Frame", "Live Feed", &trackbarPos, bufferCount - 1, onTrackbar, &shared);

    // Variable for scatter plot
    cv::Mat scatterPlot(400, 400, CV_8UC3, cv::Scalar(255, 255, 255));

    while (!done)
    {
        if (!paused)
        {
            std::unique_lock<std::mutex> lock(displayQueueMutex);
            auto now = std::chrono::steady_clock::now();
            if (now >= nextFrameTime)
            {
                if (!framesToDisplay.empty())
                {
                    size_t frame = framesToDisplay.front();
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    cv::Mat image(height, width, CV_8UC1, imageData.data());
                    cv::imshow("Live Feed", image);

                    // Process and display the binary image using processFrame
                    cv::Mat processedImage = processFrame(imageData, width, height);
                    cv::imshow("Processed Feed", processedImage);

                    // Update scatter plot using the global circularities vector
                    updateScatterPlot(scatterPlot, shared.circularities, shared.circularitiesMutex);
                    cv::imshow("Scatter Plot", scatterPlot);

                    cv::waitKey(1); // Process GUI events

                    nextFrameTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    if (nextFrameTime < now)
                    {
                        nextFrameTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    }
                }
                else
                {
                    lock.unlock();
                }
            }
            else
            {
                lock.unlock();
                cv::waitKey(1); // Process GUI events
            }
        }
        else
        {
            if (displayNeedsUpdate)
            {
                int index = currentFrameIndex;
                if (index >= 0 && index < circularBuffer.size())
                {
                    auto imageData = circularBuffer.get(index);

                    if (!imageData.empty())
                    {
                        cv::Mat image(height, width, CV_8UC1, imageData.data());
                        cv::imshow("Live Feed", image);

                        // Process and display the binary image using processFrame
                        cv::Mat processedImage = processFrame(imageData, width, height);
                        cv::imshow("Processed Feed", processedImage);

                        cv::setTrackbarPos("Frame", "Live Feed", index);
                        std::cout << "Displaying frame: " << index << std::endl;
                    }
                    else
                    {
                        std::cout << "Failed to get image data from buffer" << std::endl;
                    }
                }
                else
                {
                    std::cout << "Invalid frame index: " << index << std::endl;
                }
                displayNeedsUpdate = false;
            }
            cv::waitKey(1); // Process GUI events
        }
    }
}

void keyboardHandlingThread(
    std::atomic<bool> &done,
    std::atomic<bool> &paused,
    std::atomic<int> &currentFrameIndex,
    std::atomic<bool> &displayNeedsUpdate,
    EGrabber<CallbackOnDemand> &grabber,
    const CircularBuffer &circularBuffer,
    size_t bufferCount)
{
    while (!done)
    {
        if (_kbhit())
        {
            int ch = _getch();
            if (ch == 27)
            { // ESC key
                std::cout << "ESC pressed. Stopping capture..." << std::endl;
                done = true;
            }
            else if (ch == 32)
            { // Space bar
                paused = !paused;
                if (paused)
                {
                    uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                    uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                    std::cout << "EGrabber Frame Rate: " << fr << " FPS" << std::endl;
                    std::cout << "EGrabber Data Rate: " << dr << " MB/s" << std::endl;
                    grabber.stop();
                    currentFrameIndex = 0;
                    std::cout << "Paused" << std::endl;
                    displayNeedsUpdate = true;
                }
                else
                {
                    grabber.start();
                    std::cout << "Resumed" << std::endl;
                }
            }
            else if (ch == 98)
            { // 'b' key - capture background frame
                if (currentFrameIndex >= 0 && currentFrameIndex < circularBuffer.size())
                {
                    auto imageData = circularBuffer.get(currentFrameIndex);
                    backgroundFrame = cv::Mat(height, width, CV_8UC1, imageData.data());
                    std::cout << "Background frame captured." << std::endl;
                }
                else
                {
                    std::cout << "Invalid frame index for background capture." << std::endl;
                }
            }
            else if (ch == 97)
            { // 'a' key - move to older frame
                if (currentFrameIndex < circularBuffer.size() - 1)
                {
                    currentFrameIndex++;
                    std::cout << "a key pressed\nMoved to older frame. Current Frame Index: " << currentFrameIndex << std::endl;
                    displayNeedsUpdate = true;
                }
                else
                {
                    std::cout << "Already at oldest frame." << std::endl;
                }
            }
            else if (ch == 100)
            { // 'd' key - move to newer frame
                if (currentFrameIndex > 0)
                {
                    currentFrameIndex--;
                    std::cout << "d key pressed\nMoved to newer frame. Current Frame Index: " << currentFrameIndex << std::endl;
                    displayNeedsUpdate = true;
                }
                else
                {
                    std::cout << "Already at newest frame." << std::endl;
                }
            }
            else if (ch == 113)
            { // 'q' key
                std::lock_guard<std::mutex> lock(shared.circularitiesMutex);
                shared.circularities.clear();
                std::cout << "Circularities vector cleared." << std::endl;
            }
            else if (ch == 115)
            { // 's' key - save the current frame
                std::filesystem::path outputDir = "output";
                if (!std::filesystem::exists(outputDir))
                {
                    std::filesystem::create_directory(outputDir);
                }

                size_t frameCount = circularBuffer.size();
                std::cout << "Saving " << frameCount << " frames..." << std::endl;

                for (size_t i = 0; i < frameCount; ++i)
                {
                    auto imageData = circularBuffer.get(frameCount - 1 - i); // Start from oldest frame
                    cv::Mat image(512, 512, CV_8UC1, imageData.data());

                    std::ostringstream oss;
                    oss << "frame_" << std::setw(5) << std::setfill('0') << i << ".png";
                    std::string filename = oss.str();

                    std::filesystem::path fullPath = outputDir / filename;

                    cv::imwrite(fullPath.string(), image);
                    std::cout << "Saved frame " << (i + 1) << "/" << frameCount << ": " << filename << "\r" << std::flush;
                }

                std::cout << "\nAll frames saved in the 'output' directory." << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small sleep to reduce CPU usage
    }
}

void sample(EGrabber<CallbackOnDemand> &grabber, const GrabberParams &params, CircularBuffer &circularBuffer, SharedResources &shared)
{
    std::thread processingThread(processingThreadTask,
                                 std::ref(shared.done), std::ref(shared.paused),
                                 std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
                                 std::ref(shared.framesToProcess), std::ref(circularBuffer),
                                 params.width, params.height);
    std::thread displayThread(displayThreadTask,
                              std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
                              std::ref(shared.displayNeedsUpdate), std::ref(shared.framesToDisplay),
                              std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
                              params.width, params.height, params.bufferCount, std::ref(shared));
    std::thread keyboardThread(keyboardHandlingThread,
                               std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
                               std::ref(shared.displayNeedsUpdate), std::ref(grabber),
                               std::ref(circularBuffer), params.bufferCount, std::ref(shared));

    grabber.start();
    size_t frameCount = 0;
    uint64_t lastFrameId = 0;
    uint64_t duplicateCount = 0;
    while (!shared.done)
    {
        if (shared.paused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cv::waitKey(1);
            continue;
        }

        ScopedBuffer buffer(grabber);
        uint8_t *imagePointer = buffer.getInfo<uint8_t *>(gc::BUFFER_INFO_BASE);
        uint64_t frameId = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_FRAMEID);
        uint64_t timestamp = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_TIMESTAMP);
        bool isIncomplete = buffer.getInfo<bool>(gc::BUFFER_INFO_IS_INCOMPLETE);
        size_t sizeFilled = buffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE_FILLED);

        if (!isIncomplete)
        {
            if (frameId <= lastFrameId)
            {
                ++duplicateCount;
                std::cout << "Duplicate frame detected: FrameID=" << frameId << ", Timestamp=" << timestamp << std::endl;
            }
            else
            {
                circularBuffer.push(imagePointer);
                {
                    std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                    std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                    shared.framesToProcess.push(frameCount);
                    shared.framesToDisplay.push(frameCount);
                }
                shared.displayQueueCondition.notify_one();
                shared.processingQueueCondition.notify_one();
                frameCount++;
            }
            lastFrameId = frameId;
        }
        else
        {
            std::cout << "Incomplete frame received: FrameID=" << frameId << std::endl;
        }
    }

    grabber.stop();

    processingThread.join();
    displayThread.join();
    keyboardThread.join();
}

int main()
{
    try
    {

        EGenTL genTL;
        EGrabberDiscovery egrabberDiscovery(genTL);
        egrabberDiscovery.discover();
        EGrabber<CallbackOnDemand> grabber(egrabberDiscovery.cameras(0));
        // grabber.runScript("config.js");

        configure(grabber);
        GrabberParams params = initializeGrabber(grabber);
        initializeBackgroundFrame(params);

        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        SharedResources shared;

        sample(grabber, params, circularBuffer, shared);
    }
    catch (const std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}