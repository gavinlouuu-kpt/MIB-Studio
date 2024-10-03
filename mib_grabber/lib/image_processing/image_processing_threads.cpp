#include "image_processing.h"
#include "CircularBuffer.h"
#include <chrono>
#include <iostream>
#include <conio.h>
#include <filesystem>

namespace fs = std::filesystem;
void simulateCameraThread(std::atomic<bool> &done, std::atomic<bool> &paused,
                          CircularBuffer &cameraBuffer, SharedResources &shared,
                          const ImageParams &params)
{
    using clock = std::chrono::high_resolution_clock;

    size_t currentIndex = 0;
    size_t totalFrames = cameraBuffer.size();
    auto lastFrameTime = clock::now();
    auto fpsStartTime = clock::now();
    size_t frameCount = 0;
    const int targetFPS = 1000;
    const std::chrono::nanoseconds frameInterval(1000000000 / targetFPS);

    while (!done)
    {
        auto now = clock::now();
        if (!paused && (now - lastFrameTime) >= frameInterval)
        {
            const uint8_t *imageData = cameraBuffer.getPointer(currentIndex);
            if (imageData != nullptr)
            {
                shared.latestCameraFrame.store(currentIndex, std::memory_order_release);
                currentIndex = (currentIndex + 1) % totalFrames;
                lastFrameTime = now;
                if (++frameCount % 5000 == 0)
                {
                    // Optional: Print frame processing information
                }
            }
            else
            {
                std::cout << "Invalid frame at index: " << currentIndex << std::endl;
            }
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - fpsStartTime).count() >= 5)
        {
            double fps = frameCount / std::chrono::duration<double>(now - fpsStartTime).count();
            std::cout << "Current FPS: " << fps << std::endl;
            frameCount = 0;
            fpsStartTime = now;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void processingThreadTask(std::atomic<bool> &done, std::atomic<bool> &paused,
                          std::mutex &processingQueueMutex,
                          std::condition_variable &processingQueueCondition,
                          std::queue<size_t> &framesToProcess,
                          const CircularBuffer &circularBuffer,
                          size_t width, size_t height, SharedResources &shared)
{
    auto lastPrintTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    double totalProcessingTime = 0.0;
    double totalFindTime = 0.0;

    // cv::namedWindow("debug-preprocess", cv::WINDOW_NORMAL);
    // cv::resizeWindow("debug-preprocess", width, height);

    // cv::namedWindow("debug-postprocess", cv::WINDOW_NORMAL);
    // cv::resizeWindow("debug-postprocess", width, height);

    // Pre-allocate memory for images
    cv::Mat image(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);

    while (!done)
    {
        std::unique_lock<std::mutex> lock(processingQueueMutex);
        processingQueueCondition.wait(lock, [&]()
                                      { return !framesToProcess.empty() || done || paused; });

        if (done)
            break;

        if (!framesToProcess.empty() && !paused)
        {
            // size_t frame = framesToProcess.front(); //retrieve content of queue
            framesToProcess.pop();
            lock.unlock();

            auto imageData = circularBuffer.get(0);
            image = cv::Mat(height, width, CV_8UC1, imageData.data());
            // cv::imshow("debug-preprocess", image);
            // cv::waitKey(1);

            // Measure processing time
            auto startTime = std::chrono::high_resolution_clock::now();

            // Preprocess Image using the optimized processFrame function
            processFrame(imageData, width, height, shared, processedImage, true); // true indicates it's the processing thread

            // cv::imshow("debug-postprocess", processedImage);
            // cv::waitKey(1);

            // Find contour
            ContourResult contourResult = findContours(processedImage); // Assuming findContours is modified to accept pre-allocated contours
            std::vector<std::vector<cv::Point>> contours = contourResult.contours;
            double contourFindTime = contourResult.findTime;

            // Calculate deformability and circularity for each contour
            {
                std::lock_guard<std::mutex> circularitiesLock(shared.circularitiesMutex);
                for (const auto &contour : contours)
                {
                    if (contour.size() >= 10)
                    {
                        auto [circularity, area] = calculateMetrics(contour);
                        shared.circularities.emplace_back(circularity, area);
                        shared.newScatterDataAvailable = true;
                        shared.scatterDataCondition.notify_one();
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            double processingTime = duration.count(); // measure in microseconds
            totalProcessingTime += processingTime;
            totalFindTime += contourFindTime;

            ++frameCount;

            // Check if 5 seconds have passed
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastPrintTime).count();
            if (elapsedTime >= 5)
            {
                double averageProcessingTime = totalProcessingTime / frameCount;
                double averageFindTime = totalFindTime / frameCount;
                std::cout << "Average processing time over last " << frameCount << " frames: "
                          << averageProcessingTime << " us" << std::endl;
                std::cout << "Average contour find time: " << averageFindTime << " us" << std::endl;

                // Reset counters and update last print time
                frameCount = 0;
                totalProcessingTime = 0.0;
                totalFindTime = 0.0;
                lastPrintTime = currentTime;
            }
        }
        else
        {
            lock.unlock();
        }
    }
}

void displayThreadTask(const std::atomic<bool> &done, const std::atomic<bool> &paused,
                       const std::atomic<int> &currentFrameIndex,
                       std::atomic<bool> &displayNeedsUpdate,
                       std::queue<size_t> &framesToDisplay,
                       std::mutex &displayQueueMutex,
                       const CircularBuffer &circularBuffer,
                       size_t width, size_t height, size_t bufferCount,
                       SharedResources &shared)
{
    const std::chrono::duration<double> frameDuration(1.0 / 25.0); // 25 FPS
    auto nextFrameTime = std::chrono::steady_clock::now();

    // Pre-allocate memory for images
    cv::Mat image(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);

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
                    // size_t frame = framesToDisplay.front(); //retrieve queue content
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    image = cv::Mat(height, width, CV_8UC1, imageData.data());
                    cv::imshow("Live Feed", image);

                    // Preprocess Image using the optimized processFrame function
                    processFrame(imageData, width, height, shared, processedImage, false); // false indicates it's the display thread

                    cv::imshow("Processed Feed", processedImage);

                    // Update scatter plot using the global circularities vector
                    std::unique_lock<std::mutex> lock(shared.circularitiesMutex);
                    shared.scatterDataCondition.wait_for(lock, std::chrono::milliseconds(1),
                                                         [&shared]
                                                         { return shared.newScatterDataAvailable.load(); });

                    if (shared.newScatterDataAvailable)
                    {
                        // std::cout << "Updating scatter plot with " << shared.circularities.size() << " points" << std::endl;
                        updateScatterPlot(scatterPlot, shared.circularities);
                        cv::imshow("Scatter Plot", scatterPlot);
                        shared.newScatterDataAvailable = false;
                    }

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
                        image = cv::Mat(height, width, CV_8UC1, imageData.data());
                        cv::imshow("Live Feed", image);

                        // Process and display the binary image using processFrame
                        processFrame(imageData, width, height, shared, processedImage, false);
                        // Find contour during pause
                        ContourResult contourResult = findContours(processedImage);
                        std::vector<std::vector<cv::Point>> contours = contourResult.contours;

                        for (const auto &contour : contours) // debug purpose
                        {
                            if (contour.size() >= 5)
                            { // Ensure there are enough points to calculate circularity
                                auto [circularity, area] = calculateMetrics(contour);
                                std::cout << "Contour metrics: Circularity = " << circularity << ", Area = " << area << std::endl;
                            }
                        }
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

void onTrackbar(int pos, void *userdata)
{
    auto *shared = static_cast<SharedResources *>(userdata);
    shared->currentFrameIndex = pos;
    shared->displayNeedsUpdate = true;
}

void updateScatterPlot(cv::Mat &plot, const std::vector<std::tuple<double, double>> &circularities)
{
    // std::lock_guard<std::mutex> lock(mutex);

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

void keyboardHandlingThread(std::atomic<bool> &done, std::atomic<bool> &paused,
                            std::atomic<int> &currentFrameIndex,
                            std::atomic<bool> &displayNeedsUpdate,
                            const CircularBuffer &circularBuffer,
                            size_t bufferCount, size_t width, size_t height,
                            SharedResources &shared)
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
                    // uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                    // uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                    // std::cout << "EGrabber Frame Rate: " << fr << " FPS" << std::endl;
                    // std::cout << "EGrabber Data Rate: " << dr << " MB/s" << std::endl;
                    // grabber.stop();
                    currentFrameIndex = 0;
                    auto BackgroundImageData = circularBuffer.get(currentFrameIndex);
                    {
                        std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
                        shared.backgroundFrame = cv::Mat(height, width, CV_8UC1, BackgroundImageData.data()).clone();
                        cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
                    }

                    std::cout << "Background frame captured and blurred." << std::endl;

                    std::cout << "Paused" << std::endl;
                    displayNeedsUpdate = true;
                }
                else
                {
                    // grabber.start();
                    std::cout << "Resumed" << std::endl;
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

void mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, SharedResources &shared)
{
    std::thread processingThread(processingThreadTask,
                                 std::ref(shared.done), std::ref(shared.paused),
                                 std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
                                 std::ref(shared.framesToProcess), std::ref(circularBuffer),
                                 params.width, params.height, std::ref(shared));

    std::thread displayThread(displayThreadTask,
                              std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
                              std::ref(shared.displayNeedsUpdate), std::ref(shared.framesToDisplay),
                              std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
                              params.width, params.height, params.bufferCount, std::ref(shared));

    std::thread keyboardThread(keyboardHandlingThread,
                               std::ref(shared.done), std::ref(shared.paused), std::ref(shared.currentFrameIndex),
                               std::ref(shared.displayNeedsUpdate),
                               std::ref(circularBuffer), params.bufferCount, params.width, params.height, std::ref(shared));

    std::thread cameraSimThread(simulateCameraThread,
                                std::ref(shared.done), std::ref(shared.paused),
                                std::ref(cameraBuffer), std::ref(shared), std::ref(params));

    size_t lastProcessedFrame = 0;
    while (!shared.done)
    {
        if (shared.paused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        size_t latestFrame = shared.latestCameraFrame.load(std::memory_order_acquire);
        if (latestFrame != lastProcessedFrame)
        {
            const uint8_t *imageData = cameraBuffer.getPointer(latestFrame);
            if (imageData != nullptr)
            {
                circularBuffer.push(imageData);

                {
                    std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                    std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                    shared.framesToProcess.push(latestFrame);
                    shared.framesToDisplay.push(latestFrame);
                }
                shared.displayQueueCondition.notify_one();
                shared.processingQueueCondition.notify_one();

                lastProcessedFrame = latestFrame;

                // Optional: Print frame grabbing information
                static size_t frameCount = 0;
                if (++frameCount % 100 == 0) // Print every 100 frames
                {
                    // std::cout << "Program grabbed frame " << frameCount << " (Camera frame: " << latestFrame << ")" << std::endl;
                }
            }
        }
    }

    processingThread.join();
    displayThread.join();
    keyboardThread.join();
    cameraSimThread.join();
}
