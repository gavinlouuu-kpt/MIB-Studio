#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include <chrono>
#include <iostream>
#include <conio.h>
#include <filesystem>
#include <fstream>   // Add this for file operations
#include <string>    // Add this for std::string
#include <stdexcept> // Add this for std::runtime_error
#include <nlohmann/json.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

using json = nlohmann::json;

namespace fs = std::filesystem;
void simulateCameraThread(
    CircularBuffer &cameraBuffer, SharedResources &shared,
    const ImageParams &params)
{
    using clock = std::chrono::high_resolution_clock;

    size_t currentIndex = 0;
    size_t totalFrames = cameraBuffer.size();
    auto lastFrameTime = clock::now();
    auto fpsStartTime = clock::now();
    size_t frameCount = 0;
    const int targetFPS = 5000;
    const std::chrono::nanoseconds frameInterval(1000000000 / targetFPS);

    while (!shared.done)
    {
        auto now = clock::now();
        if (!shared.paused && (now - lastFrameTime) >= frameInterval)
        {
            const uint8_t *imageData = cameraBuffer.getPointer(currentIndex);
            if (imageData != nullptr)
            {
                shared.latestCameraFrame.store(currentIndex, std::memory_order_release);
                currentIndex = (currentIndex + 1) % totalFrames;
                lastFrameTime = now;
                if (++frameCount % 5000 == 0)
                {
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
            shared.currentFPS.store(fps, std::memory_order_release);
            frameCount = 0;
            fpsStartTime = now;
        }
        shared.updated = true;
    }
    std::cout << "Camera thread interrupted." << std::endl;
}

void metricDisplayThread(SharedResources &shared)
{
    using namespace ftxui;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 1ms to allow other things to be printed first

    auto render_processing_metrics = [&]()
    {
        return window(text("Processing Metrics"), vbox({
                                                      hbox({text("Instant Processing Time: "),
                                                            text(std::to_string(shared.instantProcessingTime.load()) + " us")}),
                                                      hbox({text("Avg Processing Time (5 sec): "),
                                                            text(std::to_string(shared.averageProcessingTime.load()) + " us")}),
                                                      hbox({text("Max Processing Time: "),
                                                            text(std::to_string(shared.maxProcessingTime.load()) + " us")}),
                                                      hbox({text("Min Processing Time: "),
                                                            text(std::to_string(shared.minProcessingTime.load()) + " us")}),
                                                      hbox({text("Processing Queue Size: "),
                                                            text(std::to_string(shared.framesToProcess.size()) + " frames")}),
                                                      hbox({text("Circularities Size: "),
                                                            text(std::to_string(shared.circularities.size()) + " sets")}),
                                                  }));
    };

    auto render_camera_metrics = [&]()
    {
        return window(text("Camera Metrics"), vbox({
                                                  hbox({text("Current FPS: "),
                                                        text(std::to_string(shared.currentFPS.load()))}),
                                                  hbox({text("Images in Queue: "),
                                                        text(std::to_string(shared.imagesInQueue.load()))}),
                                              }));
    };

    auto render_roi = [&]()
    {
        return window(text("ROI"), vbox({
                                       hbox({text("X: "),
                                             text(std::to_string(shared.roi.x))}),
                                       hbox({text("Y: "),
                                             text(std::to_string(shared.roi.y))}),
                                       hbox({text("Width: "),
                                             text(std::to_string(shared.roi.width))}),
                                       hbox({text("Height: "),
                                             text(std::to_string(shared.roi.height))}),
                                   }));
    };

    auto render_status = [&]()
    {
        return window(text("Status"), vbox({
                                          hbox({text("Paused: "),
                                                text(shared.paused.load() ? "Yes" : "No")}),
                                          hbox({text("Current Frame Index: "),
                                                text(std::to_string(shared.currentFrameIndex.load()))}),
                                          hbox({text("Saving Speed: "),
                                                text(std::to_string(shared.diskSaveTime.load()) + " ms")}),

                                      }));
    };

    auto render_keyboard_instructions = [&]()
    {
        return window(text("Keyboard Instructions"), vbox({
                                                         text("ESC: Exit"),
                                                         text("Space: Pause/Resume"),
                                                         text("Space: Background frame"),
                                                         text("A: Move to older frame"),
                                                         text("D: Move to newer frame"),
                                                         text("Q: Clear circularities"),
                                                         text("S: Save current frame"),
                                                     }));
    };

    std::string reset_position;
    while (!shared.done)
    {
        if (shared.updated)
        {
            auto document = hbox({
                render_processing_metrics(),
                render_camera_metrics(),
                render_roi(),
                render_status(),
                render_keyboard_instructions(),
            });

            auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
            Render(screen, document);
            std::cout << reset_position;
            screen.Print();
            reset_position = screen.ResetPosition();
            shared.updated = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void processingThreadTask(
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

    // Pre-allocate memory for images
    cv::Mat image(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);

    // const size_t BUFFER_THRESHOLD = config["buffer_threshold"];

    const size_t SAVE_THRESHOLD = 1000;   // Adjust as needed
    const size_t BUFFER_THRESHOLD = 1000; // Adjust as needed

    while (!shared.done)
    {
        std::unique_lock<std::mutex> lock(processingQueueMutex);
        processingQueueCondition.wait(lock, [&]()
                                      { return !framesToProcess.empty() || shared.done || shared.paused; });

        if (shared.done)
            break;

        if (!framesToProcess.empty() && !shared.paused)
        {
            // size_t frame = framesToProcess.front(); //retrieve content of queue
            framesToProcess.pop();
            lock.unlock();

            auto imageData = circularBuffer.get(0);
            image = cv::Mat(height, width, CV_8UC1, imageData.data());

            // Measure processing time
            auto startTime = std::chrono::high_resolution_clock::now();

            // Preprocess Image using the optimized processFrame function
            processFrame(imageData, width, height, shared, processedImage, true); // true indicates it's the processing thread

            // Find contour
            ContourResult contourResult = findContours(processedImage); // Assuming findContours is modified to accept pre-allocated contours
            std::vector<std::vector<cv::Point>> contours = contourResult.contours;
            double contourFindTime = contourResult.findTime;

            // Calculate deformability and circularity for each contour
            {
                std::lock_guard<std::mutex> circularitiesLock(shared.circularitiesMutex);
                // bool qualifiedContourFound = false;
                for (const auto &contour : contours)
                {
                    if (contour.size() >= 10)
                    {
                        // Check if contour touches the edge of ROI
                        bool touchesBorder = false;
                        for (const auto &point : contour)
                        {
                            if (point.x == shared.roi.x || point.x == shared.roi.x + shared.roi.width - 1 ||
                                point.y == shared.roi.y || point.y == shared.roi.y + shared.roi.height - 1)
                            {
                                touchesBorder = true;
                                break;
                            }
                        }
                        // Skip if it touches the border
                        if (touchesBorder)
                        {
                            continue;
                        }
                        auto [circularity, area] = calculateMetrics(contour);
                        shared.circularities.emplace_back(circularity, area);
                        shared.newScatterDataAvailable = true;
                        shared.scatterDataCondition.notify_one();
                        // qualifiedContourFound = true;
                        QualifiedResult qualifiedResult;
                        qualifiedResult.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        qualifiedResult.circularity = circularity;
                        qualifiedResult.area = area;
                        // qualifiedResult.contourResult = contourResult;
                        qualifiedResult.originalImage = image.clone();

                        std::lock_guard<std::mutex> qualifiedResultsLock(shared.qualifiedResultsMutex);
                        auto &currentBuffer = shared.usingBuffer1 ? shared.qualifiedResultsBuffer1 : shared.qualifiedResultsBuffer2;
                        currentBuffer.push_back(std::move(qualifiedResult));

                        // Check if we need to switch buffers
                        if (currentBuffer.size() >= BUFFER_THRESHOLD && !shared.savingInProgress)
                        {
                            shared.usingBuffer1 = !shared.usingBuffer1;
                            shared.savingInProgress = true;
                            shared.savingCondition.notify_one();
                        }
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            double processingTime = duration.count(); // measure in microseconds
            shared.instantProcessingTime = processingTime;
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
                // std::cout << "Average processing time over last " << frameCount << " frames: "
                //           << averageProcessingTime << " us" << std::endl;
                // std::cout << "Average contour find time: " << averageFindTime << " us" << std::endl;

                // // // Debugging statement
                // std::cout << "Qualified result pushed. Buffer 1 size: "
                //           << shared.qualifiedResultsBuffer1.size()
                //           << ", Buffer 2 size: "
                //           << shared.qualifiedResultsBuffer2.size()
                //           << " / " << BUFFER_THRESHOLD << std::endl;

                // Reset counters and update last print time
                frameCount = 0;
                totalProcessingTime = 0.0;
                totalFindTime = 0.0;
                lastPrintTime = currentTime;
            }

            // Update metrics
            shared.totalFramesProcessed++;
            shared.averageProcessingTime = (shared.averageProcessingTime * (shared.totalFramesProcessed - 1) + processingTime) / shared.totalFramesProcessed;
            // shared.averageFindTime = (shared.averageFindTime * (shared.totalFramesProcessed - 1) + contourFindTime) / shared.totalFramesProcessed;
            shared.qualifiedResultCount = shared.qualifiedResultCount.load();
            shared.updated = true;
        }
        else
        {
            lock.unlock();
        }
    }
    std::cout << "Processing thread interrupted." << std::endl;
}

void displayThreadTask(
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

    // Pre-allocate memory for images
    cv::Mat image(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);
    cv::Mat displayImage(height * 2, width, CV_8UC3);

    cv::namedWindow("Combined Feed", cv::WINDOW_NORMAL);
    cv::resizeWindow("Combined Feed", width, height * 2);

    cv::namedWindow("Scatter Plot", cv::WINDOW_NORMAL);
    cv::resizeWindow("Scatter Plot", 400, 400);

    int trackbarPos = 0;
    cv::createTrackbar("Frame", "Combined Feed", &trackbarPos, bufferCount - 1, onTrackbar, &shared);

    // Variable for scatter plot
    cv::Mat scatterPlot(400, 400, CV_8UC3, cv::Scalar(255, 255, 255));

    // Function to handle mouse events for ROI selection
    auto onMouse = [](int event, int x, int y, int flags, void *userdata)
    {
        static cv::Point startPoint;
        auto *sharedResources = static_cast<SharedResources *>(userdata);

        if (event == cv::EVENT_LBUTTONDOWN)
        {
            startPoint = cv::Point(x, y);
        }
        else if (event == cv::EVENT_LBUTTONUP)
        {
            cv::Point endPoint(x, y);
            // Calculate the distance between start and end points
            double distance = cv::norm(startPoint - endPoint);

            // Only update ROI if the distance is greater than a threshold (e.g., 5 pixels)
            if (distance > 5)
            {
                cv::Rect newRoi(startPoint, endPoint);
                std::lock_guard<std::mutex> lock(sharedResources->roiMutex);
                sharedResources->roi = newRoi;
                sharedResources->displayNeedsUpdate = true;
            }
        }
    };

    cv::setMouseCallback("Combined Feed", onMouse, &shared);

    while (!shared.done)
    {
        if (!shared.paused)
        {
            std::unique_lock<std::mutex> lock(displayQueueMutex);
            auto now = std::chrono::steady_clock::now();
            if (now >= nextFrameTime)
            {
                if (!framesToDisplay.empty())
                {
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    image = cv::Mat(height, width, CV_8UC1, imageData.data());

                    {
                        std::lock_guard<std::mutex> roiLock(shared.roiMutex);
                        cv::rectangle(image, shared.roi, cv::Scalar(0, 255, 0), 2);
                    }

                    // Preprocess Image using the optimized processFrame function
                    processFrame(imageData, width, height, shared, processedImage, false); // false indicates it's the display thread

                    // Concatenate live feed and processed feed
                    cv::Mat topHalf, bottomHalf;
                    cv::cvtColor(image, topHalf, cv::COLOR_GRAY2BGR);
                    cv::cvtColor(processedImage, bottomHalf, cv::COLOR_GRAY2BGR);
                    topHalf.copyTo(displayImage(cv::Rect(0, 0, width, height)));
                    bottomHalf.copyTo(displayImage(cv::Rect(0, height, width, height)));

                    cv::imshow("Combined Feed", displayImage);

                    // Update scatter plot using the global circularities vector
                    std::unique_lock<std::mutex> lock(shared.circularitiesMutex);
                    shared.scatterDataCondition.wait_for(lock, std::chrono::milliseconds(1),
                                                         [&shared]
                                                         { return shared.newScatterDataAvailable.load(); });

                    if (shared.newScatterDataAvailable)
                    {
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
            if (shared.displayNeedsUpdate)
            {
                int index = shared.currentFrameIndex;
                if (index >= 0 && index < circularBuffer.size())
                {
                    auto imageData = circularBuffer.get(index);

                    if (!imageData.empty())
                    {
                        image = cv::Mat(height, width, CV_8UC1, imageData.data());

                        // Process and display the binary image using processFrame
                        processFrame(imageData, width, height, shared, processedImage, false);

                        // Concatenate live feed and processed feed
                        cv::Mat topHalf, bottomHalf;
                        cv::cvtColor(image, topHalf, cv::COLOR_GRAY2BGR);
                        cv::cvtColor(processedImage, bottomHalf, cv::COLOR_GRAY2BGR);
                        topHalf.copyTo(displayImage(cv::Rect(0, 0, width, height)));
                        bottomHalf.copyTo(displayImage(cv::Rect(0, height, width, height)));

                        cv::imshow("Combined Feed", displayImage);

                        cv::setTrackbarPos("Frame", "Combined Feed", index);
                        // std::cout << "Displaying frame: " << index << std::endl;
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
                shared.displayNeedsUpdate = false;
            }
            cv::waitKey(1); // Process GUI events
        }
    }
    cv::destroyAllWindows();
    std::cout << "Display thread interrupted." << std::endl;
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

void keyboardHandlingThread(
    const CircularBuffer &circularBuffer,
    size_t bufferCount, size_t width, size_t height,
    SharedResources &shared)
{
    while (!shared.done)
    {
        if (_kbhit())
        {
            int ch = _getch();
            if (ch == 27)
            { // ESC key
                // std::cout << "ESC pressed. Stopping capture..." << std::endl;
                shared.done = true;
                shared.displayQueueCondition.notify_all();
                shared.processingQueueCondition.notify_all();
                shared.savingCondition.notify_all();
            }
            else if (ch == 32)
            { // Space bar
                shared.paused = !shared.paused;
                if (shared.paused)
                {
                    // uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                    // uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                    // std::cout << "EGrabber Frame Rate: " << fr << " FPS" << std::endl;
                    // std::cout << "EGrabber Data Rate: " << dr << " MB/s" << std::endl;
                    // grabber.stop();
                    shared.currentFrameIndex = 0;
                    auto BackgroundImageData = circularBuffer.get(shared.currentFrameIndex);
                    {
                        std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
                        shared.backgroundFrame = cv::Mat(height, width, CV_8UC1, BackgroundImageData.data()).clone();
                        cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
                    }

                    // std::cout << "Background frame captured and blurred." << std::endl;

                    // std::cout << "Paused" << std::endl;
                    shared.displayNeedsUpdate = true;
                }
                else
                {
                    // grabber.start();
                    // std::cout << "Resumed" << std::endl;
                }
            }
            else if (ch == 97)
            { // 'a' key - move to older frame
                if (shared.currentFrameIndex < circularBuffer.size() - 1)
                {
                    shared.currentFrameIndex++;
                    // std::cout << "a key pressed\nMoved to older frame. Current Frame Index: " << shared.currentFrameIndex << std::endl;
                    shared.displayNeedsUpdate = true;
                }
                else
                {
                    // std::cout << "Already at oldest frame." << std::endl;
                }
            }
            else if (ch == 100)
            { // 'd' key - move to newer frame
                if (shared.currentFrameIndex > 0)
                {
                    shared.currentFrameIndex--;
                    // std::cout << "d key pressed\nMoved to newer frame. Current Frame Index: " << shared.currentFrameIndex << std::endl;
                    shared.displayNeedsUpdate = true;
                }
                else
                {
                    // std::cout << "Already at newest frame." << std::endl;
                }
            }
            else if (ch == 113)
            { // 'q' key
                std::lock_guard<std::mutex> lock(shared.circularitiesMutex);
                shared.circularities.clear();
                // std::cout << "Circularities vector cleared." << std::endl;
            }
            else if (ch == 115)
            { // 's' key - save the current frame
                std::filesystem::path outputDir = "output";
                if (!std::filesystem::exists(outputDir))
                {
                    std::filesystem::create_directory(outputDir);
                }

                size_t frameCount = circularBuffer.size();
                // std::cout << "Saving " << frameCount << " frames..." << std::endl;

                for (size_t i = 0; i < frameCount; ++i)
                {
                    auto imageData = circularBuffer.get(frameCount - 1 - i); // Start from oldest frame
                    cv::Mat image(512, 512, CV_8UC1, imageData.data());

                    std::ostringstream oss;
                    oss << "frame_" << std::setw(5) << std::setfill('0') << i << ".png";
                    std::string filename = oss.str();

                    std::filesystem::path fullPath = outputDir / filename;

                    cv::imwrite(fullPath.string(), image);
                    // std::cout << "Saved frame " << (i + 1) << "/" << frameCount << ": " << filename << "\r" << std::flush;
                }

                std::cout << "\nAll frames saved in the 'output' directory." << std::endl;
            }
        }
        shared.updated = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small sleep to reduce CPU usage
    }
    std::cout << "Keyboard handling thread interrupted." << std::endl;
}

void resultSavingThread(SharedResources &shared, const std::string &saveDirectory)
{
    while (!shared.done)
    {
        std::vector<QualifiedResult> bufferToSave;
        {
            std::unique_lock<std::mutex> lock(shared.qualifiedResultsMutex);
            shared.savingCondition.wait(lock, [&shared]()
                                        { return shared.savingInProgress || shared.done; });

            if (shared.done)
                break;

            // Swap the full buffer with our local vector
            if (shared.usingBuffer1)
            {
                bufferToSave.swap(shared.qualifiedResultsBuffer2);
            }
            else
            {
                bufferToSave.swap(shared.qualifiedResultsBuffer1);
            }
        }

        // Save the buffer to disk
        if (!bufferToSave.empty())
        {
            auto start = std::chrono::steady_clock::now();
            saveQualifiedResultsToDisk(bufferToSave, saveDirectory, shared);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            shared.diskSaveTime = duration.count();

            shared.totalSavedResults += bufferToSave.size();
            shared.lastSaveTime = end;

            // std::cout << "Saved " << bufferToSave.size() << " results to disk. "
            //           << "Total saved: " << shared.totalSavedResults
            //           << ". Time taken: " << duration.count() << " ms" << std::endl;
        }

        // Mark saving as complete
        {
            std::lock_guard<std::mutex> lock(shared.qualifiedResultsMutex);
            shared.savingInProgress = false;
        }
        shared.updated = true;
    }
    std::cout << "Result saving thread interrupted." << std::endl;
}

void commonSampleLogic(SharedResources &shared, const std::string &SAVE_DIRECTORY,
                       std::function<std::vector<std::thread>(SharedResources &, const std::string &)> setupThreads)
{
    shared.done = false;
    shared.paused = false;
    shared.currentFrameIndex = -1;
    shared.displayNeedsUpdate = true;
    shared.circularities.clear();
    shared.qualifiedResults.clear();
    shared.totalSavedResults = 0;

    // saving UI block
    json config = readConfig("config.json");
    std::string saveDir = config["save_directory"];

    std::cout << "Current save directory: " << saveDir << std::endl;
    std::cout << "Do you want to use this directory or enter a new one? (1: use current / 2: enter new): ";
    int choice;
    std::cin >> choice;

    if (choice == 2)
    {
        std::cout << "Enter new save directory name: ";
        std::cin >> saveDir;

        // Update the config file with the new base directory
        updateConfig("config.json", "save_directory", saveDir);
    }

    // Automatically increment the directory name if it already exists
    std::string baseName = saveDir;
    int suffix = 1;
    while (std::filesystem::exists(saveDir))
    {
        saveDir = baseName + "_" + std::to_string(suffix);
        suffix++;
    }

    // Ensure the directory exists
    std::filesystem::create_directories(saveDir);

    std::cout << "Using save directory: " << saveDir << std::endl;

    // Call the setup function passed as parameter
    std::vector<std::thread> threads = setupThreads(shared, saveDir);

    // Wait for completion
    shared.displayQueueCondition.notify_all();
    shared.processingQueueCondition.notify_all();
    shared.savingCondition.notify_all();
    std::cout << "Joining threads..." << std::endl;
    for (auto &thread : threads)
    {
        thread.join();
    }
}

void setupCommonThreads(SharedResources &shared, const std::string &saveDir,
                        const CircularBuffer &circularBuffer, const ImageParams &params,
                        std::vector<std::thread> &threads)
{
    threads.emplace_back(processingThreadTask,
                         std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
                         std::ref(shared.framesToProcess), std::ref(circularBuffer),
                         params.width, params.height, std::ref(shared));

    threads.emplace_back(displayThreadTask, std::ref(shared.framesToDisplay),
                         std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
                         params.width, params.height, params.bufferCount, std::ref(shared));

    threads.emplace_back(keyboardHandlingThread,
                         std::ref(circularBuffer), params.bufferCount, params.width, params.height, std::ref(shared));

    threads.emplace_back(resultSavingThread, std::ref(shared), saveDir);
    threads.emplace_back(metricDisplayThread, std::ref(shared));
}

void temp_mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, SharedResources &shared)
{
    commonSampleLogic(shared, "default_save_directory", [&](SharedResources &shared, const std::string &saveDir)
                      {
                          std::vector<std::thread> threads;
                          setupCommonThreads(shared, saveDir, circularBuffer, params, threads);

                          threads.emplace_back(simulateCameraThread,
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
                                    //   static size_t frameCount = 0;
                                    //   if (++frameCount % 100 == 0) // Print every 100 frames
                                    //   {
                                    //       // std::cout << "Program grabbed frame " << frameCount << " (Camera frame: " << latestFrame << ")" << std::endl;
                                    //   }
                                  }
                              }
                          }
                          return threads; });
}