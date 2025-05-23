#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "mib_grabber/mib_grabber.h"
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
#include <matplot/matplot.h>
#include <thread>
#include <atomic>
#include <deque>  // Add this for std::deque
#include <iomanip> // For std::setprecision
#include <sstream> // For std::stringstream
#include <random>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

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
    
    // Read target FPS from config.json
    json config = readConfig("config.json");
    const int simCameraTargetFPS = config.value("simCameraTargetFPS", 5000); // Default to 5000 if not specified
    const std::chrono::nanoseconds frameInterval(1000000000 / simCameraTargetFPS);

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
                if (++frameCount % simCameraTargetFPS == 0)
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
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }
    
    std::cout << "Camera thread interrupted." << std::endl;
}

void metricDisplayThread(SharedResources &shared)
{
    using namespace ftxui;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 1ms to allow other things to be printed first

    auto calculateProcessingMetrics = [](const CircularBuffer &processingTimes)
    {
        double avgTime = 0.0, maxTime = 0.0, minTime = std::numeric_limits<double>::max();
        double instantTime = 0.0;
        size_t highLatencyCount = 0;            // Count of times above 200us
        const double LATENCY_THRESHOLD = 200.0; // 200us threshold

        size_t count = processingTimes.size();
        if (count > 0)
        {
            // Get latest value for instant time
            instantTime = *reinterpret_cast<const double *>(processingTimes.getPointer(0));

            // Calculate statistics
            for (size_t i = 0; i < count; i++)
            {
                const double *timePtr = reinterpret_cast<const double *>(processingTimes.getPointer(i));
                double time = *timePtr;
                avgTime += time;
                maxTime = std::max(maxTime, time);
                minTime = std::min(minTime, time);
                if (time > LATENCY_THRESHOLD)
                {
                    highLatencyCount++;
                }
            }
            avgTime /= count;
        }

        double highLatencyPercentage = count > 0 ? (highLatencyCount * 100.0) / count : 0.0;
        return std::make_tuple(instantTime, avgTime, maxTime, minTime, highLatencyPercentage);
    };
    auto calculateDeformabilityBufferRate = [](const SharedResources &shared)
    {
        // Calculate the rate of new sets added to the deformability buffer
        static auto lastCheckTime = std::chrono::steady_clock::now();
        static size_t lastBufferCount = 0;

        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastCheckTime).count();

        size_t currentBufferCount = shared.deformabilityBuffer.size();
        size_t addedCount = currentBufferCount - lastBufferCount;

        double rate = duration > 0 ? static_cast<double>(addedCount) / duration : 0.0;

        lastCheckTime = now;
        lastBufferCount = currentBufferCount;

        // Return the rate and the shared recorded items count
        return std::make_tuple(rate, shared.recordedItemsCount.load());
    };

    auto render_processing_metrics = [&]()
    {
        auto [instantTime, avgTime, maxTime, minTime, highLatencyPct] = calculateProcessingMetrics(shared.processingTimes);
        auto [rate, recordedCount] = calculateDeformabilityBufferRate(shared);

        return window(text("Processing Metrics"), vbox({hbox({text("Avg Processing Time: "), text(std::to_string((int)avgTime) + " us")}),
                                                        hbox({text("Max Processing Time: "), text(std::to_string((int)maxTime) + " us")}),
                                                        hbox({text("High Latency (>200us): "), text(std::to_string(highLatencyPct) + "%")}),
                                                        hbox({text("Processing Queue Size: "), text(std::to_string(shared.framesToProcess.size()) + " frames")}),
                                                        hbox({text("Deformability Buffer Size: "), text(std::to_string(shared.deformabilityBuffer.size()) + " sets")}),
                                                        hbox({text("Recorded Items Count: "), text(std::to_string(recordedCount) + " items")}),
                                                        hbox({text("Processed Trigger: "), text(shared.processTrigger.load() ? "Yes" : "No")}),
                                                        hbox({text("Trigger Onset Duration: "), text(std::to_string(shared.triggerOnsetDuration.load()) + " us")}),
                                                        hbox({text("Deformability: "), text(std::to_string(shared.frameDeformabilities.load()))}),
                                                        hbox({text("Area: "), text(std::to_string(shared.frameAreas.load()))}),
                                                        hbox({text("Area Ratio: "), text(std::to_string(shared.frameAreaRatios.load()))}),
                                                        hbox({text("Ring Ratio: "), text(std::to_string(shared.frameRingRatios.load()))}),
                                                        hbox({text("Avg Ring Ratio: "), text(std::to_string(shared.averageRingRatio.load()).substr(0, 6))})}));
    };

    auto render_config_metrics = [&]()
    {
        return window(text("Configuration"), vbox({hbox({text("Current FPS: "),
                                                         text(std::to_string((int)shared.currentFPS.load()))}),
                                                   hbox({text("Data Rate: "),
                                                         text(std::to_string((int)shared.dataRate.load()))}),
                                                   hbox({text("Exposure Time: "),
                                                         text(std::to_string((int)shared.exposureTime.load()))}),
                                                   hbox({text("Binary Threshold: "),
                                                         text(std::to_string(shared.processingConfig.bg_subtract_threshold))}),
                                                   // display if valid display frame
                                                   hbox({text("Valid Display Frame: "),
                                                         text(shared.validDisplayFrame.load() ? "Yes" : "No")}),
                                                   hbox({text("Touched Border: "),
                                                         text(shared.displayFrameTouchedBorder.load() ? "Yes" : "No")}),
                                                   hbox({text("Require Single Inner Contour: "),
                                                         text(shared.processingConfig.require_single_inner_contour ? "Yes" : "No")}),
                                                   hbox({text("Area Min Threshold: "),
                                                         text(std::to_string(shared.processingConfig.area_threshold_min))}),
                                                   hbox({text("Area Max Threshold: "),
                                                         text(std::to_string(shared.processingConfig.area_threshold_max))}),
                                                   // Contrast enhancement parameters
                                                   hbox({text("Contrast Enhancement: "),
                                                         text(shared.processingConfig.enable_contrast_enhancement ? "Enabled" : "Disabled")}),
                                                   hbox({text("Contrast Alpha: "),
                                                         text(std::to_string(shared.processingConfig.contrast_alpha))}),
                                                   hbox({text("Brightness Beta: "),
                                                         text(std::to_string(shared.processingConfig.contrast_beta))})}));
    };

    auto render_status = [&]()
    {
        // Get the background capture time with proper lock
        std::string bgCaptureTime;
        {
            std::lock_guard<std::mutex> lock(shared.backgroundCaptureTimeMutex);
            bgCaptureTime = shared.backgroundCaptureTime.empty() ? "Not set" : shared.backgroundCaptureTime;
        }

        return window(text("Status"), vbox({
                                          hbox({text("Running: "),
                                                text(shared.running.load() ? "Yes" : "No")}),
                                          hbox({text("Paused: "),
                                                text(shared.paused.load() ? "Yes" : "No")}),
                                          hbox({text("Overlay Mode: "),
                                                text(shared.overlayMode.load() ? "Yes" : "No")}),
                                          //   hbox({text("Trigger Out: "),
                                          // text(shared.triggerOut.load() ? "Yes" : "No")}),
                                          hbox({text("Current Frame Index: "),
                                                text(std::to_string(shared.currentFrameIndex.load()))}),
                                          hbox({text("Saving Speed: "),
                                                text(std::to_string((int)shared.diskSaveTime.load()) + " ms")}),
                                          hbox({text("Background Captured: "),
                                                text(bgCaptureTime)}),
                                          hbox({text("Recorded Items: "),
                                                text(std::to_string(shared.recordedItemsCount.load()))}),
                                      }));
    };

    auto render_keyboard_instructions = [&]()
    {
        return window(text("Keyboard Instructions"), vbox({
                                                         text("ESC: Exit program"),
                                                         text("Space: Pause/Resume live feed"),
                                                         text("When Paused:"),
                                                         text("  B: Set current frame as background (logs timestamp)"),
                                                         text("  A: Next frame"),
                                                         text("  D: Previous frame"),
                                                         text("Display Options:"),
                                                         text("  P: Toggle processed image overlay"),
                                                         text("  Q: Clear deformability buffer"),
                                                         text("Data Management:"),
                                                         text("  R: Toggle data recording"),
                                                         text("  S: Save all frames to disk"),
                                                         text("  F: Configure eGrabber settings"),
                                                         text("ROI: Click and drag to select region"),
                                                     }));
    };

    std::string reset_position;
    while (!shared.done)
    {
        if (shared.updated)
        {
            auto document = hbox({
                render_processing_metrics(),
                render_config_metrics(),
                // render_roi(),
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
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }
}

void validFramesDisplayThread(SharedResources &shared, const CircularBuffer &circularBuffer, const ImageParams &imageParams)
{
    const std::string windowName = "Valid Frames";
    
    // Create a window for the valid frames display
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    
    // Get dimensions from image params
    int height = static_cast<int>(imageParams.height);
    int width = static_cast<int>(imageParams.width);
    
    // Flag to track if display needs updating
    bool displayNeedsUpdate = false;
    
    // Pre-create a placeholder image for when no valid frames are available
    cv::Mat noValidFramesImg(height, width, CV_8UC3, cv::Scalar(40, 40, 40));
    cv::putText(noValidFramesImg, "Waiting for valid frames...", 
                cv::Point(width/2 - 150, height/2), 
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(200, 200, 200), 2);
    
    // Local cache of valid frames to avoid locking the mutex during display
    std::deque<SharedResources::ValidFrameData> validFramesCache;
    
    // Frame rate control - maximum 60 FPS
    const std::chrono::microseconds frameInterval(1000000 / 60); // 60 FPS = 16.67ms
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    while (!shared.done)
    {
        // Use condition variable with timeout to wait for new frames
        {
            std::unique_lock<std::mutex> lock(shared.validFramesMutex);
            
            // Check done flag immediately before waiting
            if (shared.done) {
                break;
            }
            
            // Use a shorter timeout to check done flag more frequently
            const auto shortTimeout = std::chrono::milliseconds(10);
            
            // Wait for notification with timeout or until a new frame is available or done
            shared.validFramesCondition.wait_for(lock, shortTimeout, [&shared]() {
                return shared.newValidFrameAvailable || shared.done;
            });
            
            // Check done flag again after waiting
            if (shared.done) {
                break;
            }
            
            // Enforce maximum frame rate
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime);
            if (elapsed < frameInterval && !shared.done) 
            {
                // Continue to next iteration with done check
                continue;
            }
            
            // Process new frames if available
            if (shared.newValidFrameAvailable && !shared.validFramesQueue.empty()) 
            {
                // Copy the queue to our local cache
                validFramesCache = shared.validFramesQueue;
                displayNeedsUpdate = true;
                
                // Reset the flag
                shared.newValidFrameAvailable = false;
            }
        } // Unlock mutex here to minimize lock time
        
        // Check done flag again outside the lock
        if (shared.done) {
            break;
        }
        
        // Update the display if needed
        if (displayNeedsUpdate || validFramesCache.empty())
        {
            // Reset the flag
            displayNeedsUpdate = false;
            
            // Create combined display image
            if (!validFramesCache.empty())
            {
                int totalHeight = height * validFramesCache.size();
                cv::Mat combinedDisplay(totalHeight, width, CV_8UC3);
                
                // Fill the combined display with the valid frames
                for (size_t i = 0; i < validFramesCache.size(); i++)
                {
                    cv::Mat displayRegion = combinedDisplay(cv::Rect(0, i * height, width, height));
                    
                    // Convert original to BGR
                    cv::Mat colorFrame;
                    cv::cvtColor(validFramesCache[i].originalImage, colorFrame, cv::COLOR_GRAY2BGR);
                    
                    // Create overlay from processed image
                    if (shared.overlayMode)
                    {
                        // Create a mask from the processed image
                        cv::Mat mask = (validFramesCache[i].processedImage > 0);
                        
                        // Create a colored overlay
                        cv::Mat overlay = cv::Mat::zeros(colorFrame.size(), CV_8UC3);
                        
                        // Use overlay color based on the filter result
                        cv::Scalar overlayColor = determineOverlayColor(validFramesCache[i].result, true);
                        
                        // Create semi-transparent overlay
                        const double opacity = 0.3;
                        overlay.setTo(overlayColor, mask);
                        cv::addWeighted(colorFrame, 1.0, overlay, opacity, 0, colorFrame);
                    }
                    
                    // Draw ROI rectangle
                    {
                        std::lock_guard<std::mutex> roiLock(shared.roiMutex);
                        cv::rectangle(colorFrame, shared.roi, cv::Scalar(0, 255, 0), 1);
                    }
                    
                    // Add timestamp and metrics text
                    std::stringstream ss;
          
                    
                    ss.str("");
                    ss << "Def: " << std::fixed << std::setprecision(3) << validFramesCache[i].result.deformability;
                    cv::putText(colorFrame, ss.str(), cv::Point(10, 40), 
                                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
                    
                    ss.str("");
                    ss << "Area: " << std::fixed << std::setprecision(1) << validFramesCache[i].result.area;
                    cv::putText(colorFrame, ss.str(), cv::Point(10, 60), 
                                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
                    
                    // Add ring ratio if it's a valid frame with nested contours
                    if (validFramesCache[i].result.isValid && validFramesCache[i].result.hasSingleInnerContour) {
                        ss.str("");
                        ss << "Ring Ratio: " << std::fixed << std::setprecision(3) << validFramesCache[i].result.ringRatio;
                        cv::putText(colorFrame, ss.str(), cv::Point(10, 80), 
                                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
                    }
                    
                    // Copy to the combined display
                    colorFrame.copyTo(displayRegion);
                }
                
                // Show the combined display
                cv::imshow(windowName, combinedDisplay);
            }
            else
            {
                // Show placeholder when no valid frames are available
                cv::imshow(windowName, noValidFramesImg);
            }
            
            // Process UI events with a short timeout to check done flag frequently
            if (cv::waitKey(1) >= 0 || shared.done) {
                break;
            }
            
            // Update last frame time
            lastFrameTime = std::chrono::high_resolution_clock::now();
        }
        
        // Check done flag one more time to ensure responsiveness
        if (shared.done) {
            break;
        }
    }
    
    cv::destroyWindow(windowName);
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }
    
    std::cout << "Valid frames display thread interrupted." << std::endl;
}

void processingThreadTask(
    std::mutex &processingQueueMutex,
    std::condition_variable &processingQueueCondition,
    std::queue<size_t> &framesToProcess,
    const CircularBuffer &processingBuffer,
    size_t width, size_t height, SharedResources &shared)
{
    shared.currentBatchNumber = 0;
    // Pre-allocate memory for images
    cv::Mat inputImage(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    cv::Mat processedImage(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    ThreadLocalMats mats = initializeThreadMats(static_cast<int>(height), static_cast<int>(width), shared);
    const size_t BUFFER_THRESHOLD = 1000; // Adjust as needed
    // const size_t area_threshold = 10;
    const uint8_t processedColor = 255; // grey scaled cell color
    shared.processTrigger = false;
    
    // Initialize frame counter
    size_t frameCounter = 0;

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

            auto startTime = std::chrono::high_resolution_clock::now();
            shared.validProcessingFrame = false;
            auto imageData = processingBuffer.get(0);
            inputImage = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1, imageData.data());

            // Check if ROI is the same as the full image
            if (static_cast<size_t>(shared.roi.width) != width && static_cast<size_t>(shared.roi.height) != height)
            {
                // Preprocess Image using the optimized processFrame function
                processFrame(inputImage, shared, processedImage, mats);
                auto filterResult = filterProcessedImage(processedImage, shared.roi, shared.processingConfig, 255, inputImage);

                // Use the isValid flag directly from filterResult without creating a redundant local variable
                if (filterResult.isValid)
                {
                    shared.processTrigger = true;
                    shared.validProcessingFrame = true;
                    auto plotMetrics = std::make_tuple(filterResult.deformability, filterResult.area);
                    {
                        std::lock_guard<std::mutex> circularitiesLock(shared.deformabilityBufferMutex);
                        shared.deformabilityBuffer.push(reinterpret_cast<const uint8_t *>(&plotMetrics));
                        shared.frameAreaRatios.store(filterResult.areaRatio);
                        shared.frameRingRatios.store(filterResult.ringRatio);

                        shared.newScatterDataAvailable = true;
                        shared.scatterDataCondition.notify_one();
                        
                        // If running is true, increment the recorded items counter
                        if (shared.running) {
                            shared.recordedItemsCount.fetch_add(1, std::memory_order_relaxed);
                        }

                        if (shared.running)
                        {
                            QualifiedResult qualifiedResult;
                            qualifiedResult.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                                                            std::chrono::system_clock::now().time_since_epoch())
                                                            .count();
                            qualifiedResult.areaRatio = filterResult.areaRatio;
                            qualifiedResult.area = filterResult.area;
                            qualifiedResult.deformability = filterResult.deformability;
                            qualifiedResult.ringRatio = filterResult.ringRatio;
                            qualifiedResult.brightness = filterResult.brightness;
                            qualifiedResult.originalImage = inputImage.clone();
                            qualifiedResult.processedImage = processedImage.clone();

                            std::lock_guard<std::mutex> qualifiedResultsLock(shared.qualifiedResultsMutex);
                            auto &currentBuffer = shared.usingBuffer1 ? shared.qualifiedResultsBuffer1
                                                                      : shared.qualifiedResultsBuffer2;
                            currentBuffer.push_back(std::move(qualifiedResult));

                            if (currentBuffer.size() >= BUFFER_THRESHOLD && !shared.savingInProgress)
                            {
                                shared.usingBuffer1 = !shared.usingBuffer1;
                                shared.savingInProgress = true;
                                shared.currentBatchNumber++;
                                shared.savingCondition.notify_one();
                            }
                        }
                    }
                    
                    // Add valid frame to the queue for the validFramesDisplayThread
                    {
                        std::lock_guard<std::mutex> validFramesLock(shared.validFramesMutex);
                        
                        // Create the valid frame data
                        SharedResources::ValidFrameData validFrame;
                        validFrame.originalImage = inputImage.clone();
                        validFrame.processedImage = processedImage.clone();
                        validFrame.result = filterResult;
                        validFrame.frameIndex = frameCounter++;
                        validFrame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch()).count();
                        
                        // Add to the front of the queue (newest first)
                        shared.validFramesQueue.push_front(std::move(validFrame));
                        
                        // Keep only the latest 5 frames
                        const size_t MAX_VALID_FRAMES = 5;
                        while (shared.validFramesQueue.size() > MAX_VALID_FRAMES) {
                            shared.validFramesQueue.pop_back();
                        }
                        
                        // Signal that a new valid frame is available
                        shared.newValidFrameAvailable = true;
                        shared.validFramesCondition.notify_one();
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            double processingTime = static_cast<double>(duration.count());

            // Just store the processing time
            shared.processingTimes.push(reinterpret_cast<const uint8_t *>(&processingTime));
            shared.updated = true;
        }
        else
        {
            lock.unlock();
        }
    }
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
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
    // Read target FPS from config.json
    json config = readConfig("config.json");
    const int displayFPS = config.value("displayFPS", 60); // Default to 5000 if not specified
    const uint8_t processedColor = 255; // grey scaled cell color

    const std::chrono::duration<double> frameDuration(1.0 / displayFPS); // Increase to 60 FPS for smoother response
    auto nextFrameTime = std::chrono::steady_clock::now();
    ThreadLocalMats mats = initializeThreadMats(static_cast<int>(height), static_cast<int>(width), shared);

    // Pre-allocate memory for images
    cv::Mat image(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    cv::Mat processedImage(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    cv::Mat displayImage(static_cast<int>(height), static_cast<int>(width), CV_8UC3);

    cv::namedWindow("Live Feed", cv::WINDOW_AUTOSIZE);
    cv::resizeWindow("Live Feed", static_cast<int>(width), static_cast<int>(height));

    int trackbarPos = 0;
    cv::createTrackbar("Frame", "Live Feed", &trackbarPos, static_cast<int>(bufferCount - 1), onTrackbar, &shared);

    auto updateDisplay = [&](const cv::Mat &originalImage, const cv::Mat &processedImage, const FilterResult &filterResult)
    {
        // Convert original image to color
        cv::cvtColor(originalImage, displayImage, cv::COLOR_GRAY2BGR);

        if (shared.overlayMode)
        {
            // Create a mask from the processed image
            cv::Mat mask = (processedImage > 0);

            // Create a colored overlay image
            cv::Mat overlay = cv::Mat::zeros(displayImage.size(), CV_8UC3);

            const double opacity = 0.3; // Reduced opacity for better blending

            // Use the determineOverlayColor function to get the overlay color directly from filterResult
            cv::Scalar overlayColor = determineOverlayColor(filterResult, filterResult.isValid);

            // Create semi-transparent overlay
            overlay.setTo(overlayColor, mask);
            cv::addWeighted(displayImage, 1.0, overlay, opacity, 0, displayImage);
        }

        // Draw ROI rectangle
        {
            std::lock_guard<std::mutex> roiLock(shared.roiMutex);
            cv::rectangle(displayImage, shared.roi, cv::Scalar(0, 255, 0), 1);
        }

        cv::imshow("Live Feed", displayImage);
    };

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
            double distance = cv::norm(startPoint - endPoint);

            if (distance > 5)
            {
                cv::Rect newRoi(startPoint, endPoint);
                std::lock_guard<std::mutex> lock(sharedResources->roiMutex);
                sharedResources->roi = newRoi;
                sharedResources->displayNeedsUpdate = true;
            }
        }
    };

    cv::setMouseCallback("Live Feed", onMouse, &shared);

    while (!shared.done)
    {
        auto now = std::chrono::steady_clock::now();
        bool shouldUpdate = false;

        if (!shared.paused)
        {
            if (now >= nextFrameTime)
            {
                std::unique_lock<std::mutex> lock(displayQueueMutex);
                if (!framesToDisplay.empty())
                {
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    image = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1, imageData.data());
                    processFrame(image, shared, processedImage, mats);
                    auto filterResult = filterProcessedImage(processedImage, shared.roi, shared.processingConfig, 255, image);

                    // Update shared state variables
                    shared.hasSingleInnerContour = filterResult.hasSingleInnerContour;
                    shared.innerContourCount = filterResult.innerContourCount;
                    shared.usingInnerContour = filterResult.hasSingleInnerContour && filterResult.isValid;
                    shared.displayFrameTouchedBorder = filterResult.touchesBorder;
                    shared.validDisplayFrame = filterResult.isValid;

                    if (filterResult.isValid)
                    {
                        shared.frameDeformabilities.store(filterResult.deformability);
                        shared.frameAreas.store(filterResult.area);
                        shared.frameAreaRatios.store(filterResult.areaRatio);
                        shared.frameRingRatios.store(filterResult.ringRatio);
                    }
                    else
                    {
                        // Store negative values of the metrics to indicate invalid frames
                        shared.frameDeformabilities.store(-filterResult.deformability);
                        shared.frameAreas.store(-filterResult.area);
                        shared.frameAreaRatios.store(-filterResult.areaRatio);
                        shared.frameRingRatios.store(-filterResult.ringRatio);
                    }

                    updateDisplay(image, processedImage, filterResult);
                    shouldUpdate = true;

                    nextFrameTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    if (nextFrameTime < now)
                    {
                        nextFrameTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    }
                }
            }
        }
        else
        {
            if (shared.displayNeedsUpdate)
            {
                int index = shared.currentFrameIndex;
                if (index >= 0 && index < circularBuffer.size())
                {
                    // Reset state variables before processing
                    shared.validDisplayFrame = false;
                    shared.displayFrameTouchedBorder = false;
                    shared.hasSingleInnerContour = false;
                    shared.innerContourCount = 0;
                    shared.usingInnerContour = false;

                    auto imageData = circularBuffer.get(index);
                    if (!imageData.empty())
                    {
                        // Read config to enable hot reloading of image processing parameters
                        json config = readConfig("config.json");
                        ProcessingConfig newConfig = getProcessingConfig(config);

                        // Check if contrast settings have changed
                        bool contrastSettingsChanged =
                            (shared.processingConfig.enable_contrast_enhancement != newConfig.enable_contrast_enhancement) ||
                            (shared.processingConfig.contrast_alpha != newConfig.contrast_alpha) ||
                            (shared.processingConfig.contrast_beta != newConfig.contrast_beta) ||
                            (shared.processingConfig.gaussian_blur_size != newConfig.gaussian_blur_size);

                        shared.processingConfigMutex.lock();
                        shared.processingConfig = newConfig;
                        shared.processingConfigMutex.unlock();

                        // If contrast settings changed, update the background
                        if (contrastSettingsChanged && !shared.backgroundFrame.empty())
                        {
                            updateBackgroundWithCurrentSettings(shared);
                        }

                        image = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1, imageData.data());
                        processFrame(image, shared, processedImage, mats);
                        auto filterResult = filterProcessedImage(processedImage, shared.roi, shared.processingConfig, 255, image);

                        // Update shared state variables
                        shared.hasSingleInnerContour = filterResult.hasSingleInnerContour;
                        shared.innerContourCount = filterResult.innerContourCount;
                        shared.usingInnerContour = filterResult.hasSingleInnerContour && filterResult.isValid;
                        shared.displayFrameTouchedBorder = filterResult.touchesBorder;
                        shared.validDisplayFrame = filterResult.isValid;

                        if (filterResult.isValid)
                        {
                            shared.frameDeformabilities.store(filterResult.deformability);
                            shared.frameAreas.store(filterResult.area);
                            shared.frameAreaRatios.store(filterResult.areaRatio);
                            shared.frameRingRatios.store(filterResult.ringRatio);
                        }
                        else
                        {
                            // Store negative values of the metrics to indicate invalid frames
                            shared.frameDeformabilities.store(-filterResult.deformability);
                            shared.frameAreas.store(-filterResult.area);
                            shared.frameAreaRatios.store(-filterResult.areaRatio);
                            shared.frameRingRatios.store(-filterResult.ringRatio);
                        }

                        updateDisplay(image, processedImage, filterResult);
                        cv::setTrackbarPos("Frame", "Live Feed", index);
                        shouldUpdate = true;
                    }
                }
                shared.displayNeedsUpdate = false;
            }
        }

        // Handle keyboard input more frequently
        for (int i = 0; i < 5; i++)
        { // Check multiple times per frame
            int key = cv::waitKey(1) & 0xFF;
            if (key != -1 && shared.keyboardCallback)
            {
                std::lock_guard<std::mutex> lock(shared.keyboardMutex);
                shared.keyboardCallback(key);
            }

            // If we're not updating the display, add a small sleep
            if (!shouldUpdate)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        // Only call imshow when we actually need to update the display
        if (shouldUpdate)
        {
            cv::imshow("Live Feed", displayImage);
        }
    }

    cv::destroyAllWindows();
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }
    
    std::cout << "Display thread interrupted." << std::endl;
}

void onTrackbar(int pos, void *userdata)
{
    auto *shared = static_cast<SharedResources *>(userdata);
    shared->currentFrameIndex = pos;
    shared->displayNeedsUpdate = true;
}

void updateScatterPlot(SharedResources &shared)
{
    using namespace matplot;

    try
    {
        auto f = figure(true);
        if (!f)
        {
            std::cerr << "Failed to create figure" << std::endl;
            return;
        }
        f->quiet_mode(true);
        auto ax = gca();
        if (!ax)
        {
            std::cerr << "Failed to get current axes" << std::endl;
            return;
        }

        std::vector<double> x, y;
        x.reserve(2000);
        y.reserve(2000);

        const auto updateInterval = std::chrono::milliseconds(5000);
        auto lastUpdateTime = std::chrono::steady_clock::now();

        while (!shared.done)
        {
            try
            {
                auto now = std::chrono::steady_clock::now();
                if (now - lastUpdateTime < updateInterval)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                bool needsUpdate = false;
                {
                    std::lock_guard<std::mutex> lock(shared.deformabilityBufferMutex);
                    if (shared.newScatterDataAvailable && shared.deformabilityBuffer.size() > 0)
                    {
                        x.clear();
                        y.clear();

                        size_t size = shared.deformabilityBuffer.size();
                        if (x.capacity() < size)
                        {
                            x.reserve(size);
                            y.reserve(size);
                        }

                        for (size_t i = 0; i < size; i++)
                        {
                            const auto *metrics = reinterpret_cast<const std::tuple<double, double> *>(
                                shared.deformabilityBuffer.getPointer(i));
                            if (metrics)
                            {
                                x.push_back(std::get<1>(*metrics)); // area
                                y.push_back(std::get<0>(*metrics)); // deformability
                            }
                        }

                        if (!x.empty() && !y.empty())
                        {
                            needsUpdate = true;
                        }
                        shared.newScatterDataAvailable = false;
                    }
                }

                if (needsUpdate)
                {
                    try
                    {
                        ax->clear();

                        // Check for invalid values
                        bool hasInvalidValues = false;
                        for (size_t i = 0; i < x.size(); ++i)
                        {
                            if (!std::isfinite(x[i]) || !std::isfinite(y[i]))
                            {
                                hasInvalidValues = true;
                                break;
                            }
                        }

                        if (!hasInvalidValues)
                        {
                            binscatter(x, y, bin_scatter_style::point_colormap);
                            colormap(ax, palette::parula());

                            ax->xlabel("Area");
                            ax->ylabel("Deformability");
                            ax->title("Deformability vs Area (Density)");

                            f->draw();
                        }
                        else
                        {
                            std::cerr << "Invalid values detected in plot data" << std::endl;
                        }

                        lastUpdateTime = now;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error updating plot: " << e.what() << std::endl;
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error in plot loop: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error in scatter plot thread: " << e.what() << std::endl;
    }

    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }

    std::cout << "Scatter plot thread interrupted." << std::endl;
}

void updateRingRatioHistogram(SharedResources &shared)
{
    using namespace matplot;

    try
    {
        auto f = figure(true);
        if (!f)
        {
            std::cerr << "Failed to create histogram figure" << std::endl;
            return;
        }
        f->quiet_mode(true);
        auto ax = gca();
        if (!ax)
        {
            std::cerr << "Failed to get current axes for histogram" << std::endl;
            return;
        }

        // Vector to store ring ratios
        std::vector<double> ringRatios;
        ringRatios.reserve(2000);
        
        // Vector to collect new data between updates
        std::vector<double> newRingRatios;
        newRingRatios.reserve(500);

        // Keep track of when the histogram was last cleared
        bool histogramWasCleared = false;
        size_t dataPointsSinceLastClear = 0;

        const auto updateInterval = std::chrono::milliseconds(5000);
        auto lastUpdateTime = std::chrono::steady_clock::now();

        while (!shared.done)
        {
            try
            {
                // Wait for new data or timeout
                {
                    std::unique_lock<std::mutex> lock(shared.deformabilityBufferMutex);
                    
                    // Use the same condition variable as scatter plot with timeout
                    shared.scatterDataCondition.wait_for(lock, std::chrono::milliseconds(100), [&shared]() {
                        return shared.newScatterDataAvailable || shared.clearHistogramData || shared.done;
                    });
                    
                    if (shared.done) break;
                    
                    // Clear data if requested
                    if (shared.clearHistogramData)
                    {
                        ringRatios.clear();
                        newRingRatios.clear();
                        histogramWasCleared = true;
                        dataPointsSinceLastClear = 0;
                        shared.clearHistogramData = false;
                        std::cout << "Histogram data cleared" << std::endl;
                        
                        // Reset the average ring ratio when clearing
                        shared.averageRingRatio.store(0.0, std::memory_order_relaxed);
                        
                        // Force an immediate update of the empty histogram
                        lastUpdateTime = std::chrono::steady_clock::now() - updateInterval;
                    }

                    // Collect new data into temporary buffer without updating the plot yet
                    if (shared.newScatterDataAvailable)
                    {
                        // Process qualified results to extract ring ratios
                        {
                            std::lock_guard<std::mutex> qualifiedLock(shared.qualifiedResultsMutex);
                            const auto& currentBuffer = shared.usingBuffer1 ? 
                                shared.qualifiedResultsBuffer1 : shared.qualifiedResultsBuffer2;
                                
                            // Collect valid ring ratios into the temporary buffer
                            for (const auto& result : currentBuffer) {
                                if (result.ringRatio > 0) {
                                    newRingRatios.push_back(result.ringRatio);
                                    dataPointsSinceLastClear++;
                                }
                            }
                        }
                        
                        // Add current frame's ring ratio if valid
                        double currentRingRatio = shared.frameRingRatios.load();
                        if (currentRingRatio > 0) {
                            newRingRatios.push_back(currentRingRatio);
                            dataPointsSinceLastClear++;
                        }
                        
                        shared.newScatterDataAvailable = false;
                    }
                }

                // Check if it's time to update the plot (every 5 seconds)
                auto now = std::chrono::steady_clock::now();
                if (now - lastUpdateTime >= updateInterval)
                {
                    // Add accumulated new data to the main buffer
                    if (!newRingRatios.empty()) {
                        ringRatios.insert(ringRatios.end(), newRingRatios.begin(), newRingRatios.end());
                        newRingRatios.clear();
                        
                        // Only apply the MAX_PERSISTENT_SIZE limit if a significant number of new data points
                        // have been added since the last clear (to avoid keeping old data after clearing)
                        if (!histogramWasCleared || dataPointsSinceLastClear > 10000) {
                            // Keep only the most recent entries
                            const size_t MAX_PERSISTENT_SIZE = 10000;
                            if (ringRatios.size() > MAX_PERSISTENT_SIZE) {
                                ringRatios.erase(
                                    ringRatios.begin(),
                                    ringRatios.begin() + (ringRatios.size() - MAX_PERSISTENT_SIZE)
                                );
                            }
                            histogramWasCleared = false;
                        }
                    }

                    // Update the histogram if there's data
                    if (!ringRatios.empty())
                    {
                        try
                        {
                            ax->clear();

                            // Check for invalid values
                            bool hasInvalidValues = false;
                            for (double ratio : ringRatios)
                            {
                                if (!std::isfinite(ratio))
                                {
                                    hasInvalidValues = true;
                                    break;
                                }
                            }

                            if (!hasInvalidValues)
                            {
                                // Calculate average ring ratio
                                double sum = 0.0;
                                for (double ratio : ringRatios) {
                                    sum += ratio;
                                }
                                double average = sum / ringRatios.size();
                                
                                // Store the average in the shared resource for dashboard display
                                shared.averageRingRatio.store(average, std::memory_order_relaxed);
                                
                                // Create histogram using a fixed number of bins
                                const int NUM_BINS = 25;
                                auto h = hist(ringRatios, NUM_BINS);
                                
                                xlabel("Ring Ratio");
                                ylabel("Frequency");
                                title("Ring Ratio Distribution (" + std::to_string(ringRatios.size()) + " samples, Avg: " + 
                                      std::to_string(average).substr(0, 5) + ")");

                                f->draw();
                            }
                            else
                            {
                                std::cerr << "Invalid values detected in histogram data" << std::endl;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "Error updating histogram: " << e.what() << std::endl;
                        }
                    }
                    else {
                        // If there's no data (after clearing), display an empty histogram
                        ax->clear();
                        xlabel("Ring Ratio");
                        ylabel("Frequency");
                        title("Ring Ratio Distribution (0 samples)");
                        f->draw();
                    }
                    
                    lastUpdateTime = now;
                }
                else
                {
                    // Sleep briefly to avoid busy waiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error in histogram loop: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error in ring ratio histogram thread: " << e.what() << std::endl;
    }

    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
    }

    std::cout << "Ring ratio histogram thread interrupted." << std::endl;
}

void keyboardHandlingThread(
    const CircularBuffer &circularBuffer,
    size_t bufferCount, size_t width, size_t height,
    SharedResources &shared)
{
    auto handleKeypress = [&](int key)
    {
        if (key == 27)
        { // ESC key
            shared.done = true;
            
            // Signal all condition variables to wake up their threads
            shared.validFramesCondition.notify_all();
            shared.displayQueueCondition.notify_all();
            shared.processingQueueCondition.notify_all();
            shared.savingCondition.notify_all();
            shared.scatterDataCondition.notify_all();
            
            // Set new valid frame available to ensure the valid frames thread wakes up
            shared.newValidFrameAvailable = true;
            
            std::cout << "ESC pressed, exiting..." << std::endl;
        }
        else if (key == 32)
        { // Space bar
            shared.paused = !shared.paused;
            if (shared.paused)
            {
                shared.currentFrameIndex = static_cast<int>(circularBuffer.size() - 1);
                shared.displayNeedsUpdate = true;
            }
        }
        else if ((key == 'd' || key == 'D') && shared.paused && shared.currentFrameIndex < static_cast<int>(circularBuffer.size() - 1))
        {
            shared.currentFrameIndex++;
            shared.displayNeedsUpdate = true;
        }
        else if ((key == 'a' || key == 'A') && shared.paused && shared.currentFrameIndex > 0)
        {
            shared.currentFrameIndex--;
            shared.displayNeedsUpdate = true;
        }
        else if (key == 'f' || key == 'F')
        {
            configure_js("egrabberConfig.js");
        }
        else if (key == 'p' || key == 'P')
        {
            shared.overlayMode = !shared.overlayMode;
            shared.displayNeedsUpdate = true;
        }
        else if (key == 'q' || key == 'Q')
        {
            // Clear deformability buffer
            std::lock_guard<std::mutex> lock(shared.deformabilityBufferMutex);
            shared.deformabilityBuffer.clear();
            
            // Set flag to clear histogram data
            shared.clearHistogramData = true;
            std::cout << "Clearing histogram data..." << std::endl;
        }
        else if (key == 'S')
        {
            std::filesystem::path outputDir = "stream_output";
            if (!std::filesystem::exists(outputDir))
            {
                std::filesystem::create_directory(outputDir);
            }
            // Find the next available numbered folder
            int folderNum = 1;
            while (std::filesystem::exists(outputDir / std::to_string(folderNum)))
            {
                folderNum++;
            }

            // Create the numbered subfolder
            std::filesystem::path currentSaveDir = outputDir / std::to_string(folderNum);
            std::filesystem::create_directory(currentSaveDir);

            size_t frameCount = circularBuffer.size();
            // std::cout << "Saving " << frameCount << " frames..." << std::endl;

            for (size_t i = 0; i < frameCount; ++i)
            {
                // Use the same indexing as the trackbar to maintain consistency
                auto imageData = circularBuffer.get(i);
                cv::Mat image(static_cast<int>(height), static_cast<int>(width), CV_8UC1, imageData.data());

                std::ostringstream oss;
                oss << "frame_" << std::setw(5) << std::setfill('0') << i << ".tiff";
                std::string filename = oss.str();

                std::filesystem::path fullPath = currentSaveDir / filename;

                cv::imwrite(fullPath.string(), image);
                // std::cout << "Saved frame " << (i + 1) << "/" << frameCount << ": " << filename << "\r" << std::flush;
            }
        }
        else if ((key == 'b' || key == 'B') && shared.paused)
        {
            auto backgroundImageData = circularBuffer.get(shared.currentFrameIndex);
            {
                std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
                shared.backgroundFrame = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1, backgroundImageData.data()).clone();

                // Apply Gaussian blur to the background frame
                cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                                 cv::Size(shared.processingConfig.gaussian_blur_size,
                                          shared.processingConfig.gaussian_blur_size),
                                 0);

                // Apply simple contrast enhancement to the background if enabled
                if (shared.processingConfig.enable_contrast_enhancement)
                {
                    // Use the formula: new_pixel = alpha * old_pixel + beta
                    // alpha > 1 increases contrast, beta increases brightness
                    shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                                       shared.processingConfig.contrast_alpha,
                                                       shared.processingConfig.contrast_beta);

                    // std::cout << "Background set with contrast enhancement applied." << std::endl;
                }
                else
                {
                    // std::cout << "Background set." << std::endl;
                }

                // Record the timestamp when background was captured
                auto now = std::chrono::system_clock::now();
                auto time_t_now = std::chrono::system_clock::to_time_t(now);
                std::lock_guard<std::mutex> timeLock(shared.backgroundCaptureTimeMutex);

                // Format time to only show hours:minutes:seconds
                std::tm timeInfo;
                localtime_s(&timeInfo, &time_t_now);
                char buffer[9]; // HH:MM:SS + null terminator
                strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
                shared.backgroundCaptureTime = buffer;
            }
            shared.displayNeedsUpdate = true;
            shared.updated = true; // Ensure dashboard gets updated
        }
        else if (key == 'r' || key == 'R')
        {
            // Toggle running state
            shared.running = !shared.running;
        }
        shared.updated = true;
    };

    // Store the callback for use in displayThreadTask
    shared.keyboardCallback = handleKeypress;

    // Handle console input
    while (!shared.done)
    {
        if (_kbhit())
        {
            int ch = _getch();
            handleKeypress(ch);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
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
            shared.diskSaveTime = static_cast<double>(duration.count());

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
    
    // Signal that this thread is ready to be joined
    {
        std::lock_guard<std::mutex> lock(shared.threadShutdownMutex);
        shared.threadsReadyToJoin.fetch_add(1, std::memory_order_release);
        shared.threadShutdownCondition.notify_one();
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
    shared.deformabilityBuffer.clear();
    shared.qualifiedResults.clear();
    shared.totalSavedResults = 0;
    shared.recordedItemsCount = 0; // Initialize recorded items counter
    
    // Reset thread counting
    shared.activeThreadCount = 0;
    shared.threadsReadyToJoin = 0;

    // Initialize the background capture time
    {
        std::lock_guard<std::mutex> lock(shared.backgroundCaptureTimeMutex);
        shared.backgroundCaptureTime = "";
    }

    // Create egrabberConfig.js if it doesn't exist
    createDefaultConfigIfMissing("egrabberConfig.js");

    // Select save directory
    std::string saveDir = selectSaveDirectory("config.json");
    shared.saveDirectory = saveDir;

    // Call the setup function passed as parameter
    std::vector<std::thread> threads = setupThreads(shared, saveDir);
    
    // Store the number of threads we need to wait for
    shared.activeThreadCount = threads.size();

    // Wait for threads to signal they're ready to be joined when done is set to true
    // This will happen when ESC is pressed and done becomes true
    {
        std::unique_lock<std::mutex> lock(shared.threadShutdownMutex);
        
        // Wait for all threads to signal they're exiting
        while (!shared.done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Waiting for all threads to complete..." << std::endl;
        
        // Send signals to all condition variables to wake threads that might be waiting
        shared.displayQueueCondition.notify_all();
        shared.processingQueueCondition.notify_all();
        shared.savingCondition.notify_all();
        shared.validFramesCondition.notify_all();
        shared.scatterDataCondition.notify_all();
        
        // Wait for all threads to be ready to join
        shared.threadShutdownCondition.wait(lock, [&shared]() {
            return shared.threadsReadyToJoin >= shared.activeThreadCount;
        });
        
        std::cout << "All threads are ready to be joined." << std::endl;
    }
    
    // Now safe to join all threads
    std::cout << "Joining threads..." << std::endl;
    for (auto &thread : threads)
    {
        thread.join();
    }
}

void setupCommonThreads(SharedResources &shared, const std::string &saveDir,
                        const CircularBuffer &circularBuffer, const CircularBuffer &processingBuffer, const ImageParams &params,
                        std::vector<std::thread> &threads)
{
    // Create processing thread first and set its priority
    threads.emplace_back(processingThreadTask,
                         std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
                         std::ref(shared.framesToProcess), std::ref(processingBuffer),
                         params.width, params.height, std::ref(shared));
    // Create remaining threads with normal priority
    threads.emplace_back(displayThreadTask, std::ref(shared.framesToDisplay),
                         std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
                         params.width, params.height, params.bufferCount, std::ref(shared));

    threads.emplace_back(keyboardHandlingThread,
                         std::ref(circularBuffer), params.bufferCount, params.width, params.height, std::ref(shared));

    threads.emplace_back(resultSavingThread, std::ref(shared), saveDir);
    threads.emplace_back(metricDisplayThread, std::ref(shared));
    
    // Add the validFramesDisplayThread to show the latest 5 valid frames
    threads.emplace_back(validFramesDisplayThread, std::ref(shared), std::ref(circularBuffer), std::ref(params));

    // Read from json to check if scatterplot and histogram are enabled
    json config = readConfig("config.json");
    bool scatterPlotEnabled = config.value("scatter_plot_enabled", false);
    bool histogramEnabled = config.value("histogram_enabled", true);  // Default to true

    if (scatterPlotEnabled)
    {
        threads.emplace_back(updateScatterPlot, std::ref(shared));
    }
    
    if (histogramEnabled)
    {
        threads.emplace_back(updateRingRatioHistogram, std::ref(shared));
    }
}

void temp_mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared)
{
    commonSampleLogic(shared, "default_save_directory", [&](SharedResources &shared, const std::string &saveDir)
                      {
                          std::vector<std::thread> threads;
                          setupCommonThreads(shared, saveDir, circularBuffer, processingBuffer, params, threads);

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
                                      processingBuffer.push(imageData);
                                      {
                                          std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                                          std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                                          shared.framesToProcess.push(latestFrame);
                                          shared.framesToDisplay.push(latestFrame);
                                      }
                                      shared.displayQueueCondition.notify_one();
                                      shared.processingQueueCondition.notify_one();
                                      lastProcessedFrame = latestFrame;
                                  }
                              }
                          }
                          return threads; });
}
