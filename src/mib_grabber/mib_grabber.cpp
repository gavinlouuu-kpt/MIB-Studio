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
#include <CircularBuffer/CircularBuffer.h>
#include <tuple>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <image_processing/image_processing.h>
#include <menu_system/menu_system.h>
#include <nlohmann/json.hpp>
// #include <mib_grabber/mib_grabber.h>
// #include <../egrabber_config.h>

#define M_PI 3.14159265358979323846 // pi

cv::Mat backgroundFrame;

using namespace Euresys;

// struct GrabberParams
// {
//     size_t width;
//     size_t height;
//     uint64_t pixelFormat;
//     size_t imageSize;
//     size_t bufferCount;
// };

// void configure(EGrabber<CallbackOnDemand> &grabber)
// {
//     grabber.setInteger<RemoteModule>("Width", 512);
//     grabber.setInteger<RemoteModule>("Height", 96);
//     grabber.setInteger<RemoteModule>("AcquisitionFrameRate", 4700);
//     // ... (other configuration settings)
// }

static int lastUsedCameraIndex = -1;

void configure_js(std::string config_path)
{
    // Euresys::EGenTL gentl;
    // Euresys::EGrabber<> grabber(gentl);
    // if (config_path.substr(config_path.size() - 3) != ".js")
    // {
    //     throw std::runtime_error("Config path must end with .js");
    // }
    // grabber.runScript(config_path);
    // std::cout << "Config script executed successfully" << std::endl;
    try
    {
        Euresys::EGenTL gentl;
        Euresys::EGrabberDiscovery discovery(gentl);

        // Verify file extension
        if (config_path.substr(config_path.size() - 3) != ".js")
        {
            throw std::runtime_error("Config path must end with .js");
        }

        // Discover available cameras
        discovery.discover();

        if (discovery.cameraCount() == 0)
        {
            throw std::runtime_error("No cameras detected");
        }

        // If no camera was previously selected, use the first one
        if (lastUsedCameraIndex < 0 || lastUsedCameraIndex >= discovery.cameraCount())
        {
            lastUsedCameraIndex = 0;
        }

        // Initialize grabber with the last used camera
        Euresys::EGrabber<> grabber(discovery.cameras(lastUsedCameraIndex));

        // Run the configuration script
        grabber.runScript(config_path);
        std::cout << "Config script executed successfully on camera " << lastUsedCameraIndex << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Configuration error: " << e.what() << std::endl;
    }
}

ImageParams initializeGrabber(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.reallocBuffers(3);
    grabber.start(1);
    ScopedBuffer firstBuffer(grabber);

    ImageParams params;
    params.width = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    params.height = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    params.pixelFormat = firstBuffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    params.imageSize = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    params.bufferCount = 5000; // You can adjust this as needed

    grabber.stop();
    return params;
}

void initializeBackgroundFrame(SharedResources &shared, const ImageParams &params)
{
    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
    shared.backgroundFrame = cv::Mat(static_cast<int>(params.height), static_cast<int>(params.width), CV_8UC1, cv::Scalar(255));
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
}

void triggerOut(EGrabber<CallbackOnDemand> &grabber, SharedResources &shared)
{
    grabber.setString<InterfaceModule>("LineSelector", "TTLIO12");
    grabber.setString<InterfaceModule>("LineMode", "Output");
    grabber.setString<InterfaceModule>("LineSource", shared.triggerOut ? "High" : "Low");
}

void triggerThread(EGrabber<CallbackOnDemand> &grabber, SharedResources &shared)
{
    while (!shared.done)
    {
        triggerOut(grabber, shared);
        // wait do not sleep
    }
}

void temp_sample(EGrabber<CallbackOnDemand> &grabber, const ImageParams &params, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared)
{
    commonSampleLogic(shared, "default_save_directory", [&](SharedResources &shared, const std::string &saveDir)
                      {
                          std::vector<std::thread> threads;
                          setupCommonThreads(shared, saveDir, circularBuffer, processingBuffer, params, threads);
                          // Add trigger thread before starting the grabber
                          threads.emplace_back(triggerThread, std::ref(grabber), std::ref(shared));
                          
                          grabber.start();
                          // egrabber request fps and exposure time load to shared.resources for metric display
                          uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                          uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                          // Get exposure time
                          uint64_t exposureTime = grabber.getInteger<RemoteModule>("ExposureTime");
                          shared.currentFPS = fr;
                          shared.dataRate = dr;
                          shared.exposureTime = exposureTime;
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

                              // Update FPS and other metrics periodically (every 100 frames or so)
                              if (frameCount % 100 == 0)
                              {
                                  uint64_t fr = grabber.getInteger<StreamModule>("StatisticsFrameRate");
                                  uint64_t dr = grabber.getInteger<StreamModule>("StatisticsDataRate");
                                  uint64_t exposureTime = grabber.getInteger<RemoteModule>("ExposureTime");
                                  shared.currentFPS = fr;
                                  shared.dataRate = dr;
                                  shared.exposureTime = exposureTime;
                                  shared.updated = true;
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
                                      //   std::cout << "Duplicate frame detected: FrameID=" << frameId << ", Timestamp=" << timestamp << std::endl;
                                  }
                                  else
                                  {
                                      circularBuffer.push(imagePointer);
                                      processingBuffer.push(imagePointer);
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
                                  //   std::cout << "Incomplete frame received: FrameID=" << frameId << std::endl;
                              }
                          }

                          grabber.stop();

                          return threads; });
}

int mib_grabber_main()
{
    try
    {
        EGenTL genTL;
        EGrabberDiscovery discovery(genTL);

        // Discover available cameras and grabbers
        std::cout << "Scanning for available eGrabbers and cameras..." << std::endl;
        discovery.discover();

        // Display available cameras
        if (discovery.cameraCount() == 0)
        {
            throw std::runtime_error("No cameras detected in the system");
        }

        std::cout << "\nAvailable cameras:" << std::endl;
        for (int i = 0; i < discovery.cameraCount(); ++i)
        {
            EGrabberCameraInfo info = discovery.cameras(i);
            std::cout << i << ": " << info.grabbers[0].deviceModelName << std::endl;
        }

        // Let user select camera
        std::cout << "\nSelect camera (0-" << discovery.cameraCount() - 1 << "): ";
        int selectedCamera;
        std::cin >> selectedCamera;

        if (selectedCamera < 0 || selectedCamera >= discovery.cameraCount())
        {
            throw std::runtime_error("Invalid camera selection");
        }

        lastUsedCameraIndex = selectedCamera;
        // Initialize grabber with selected camera
        EGrabber<CallbackOnDemand> grabber(discovery.cameras(selectedCamera));

        // // Optional: Load configuration script
        // std::string configPath;
        // std::cout << "Enter configuration script path (or press Enter to skip): ";
        // std::cin.ignore();
        // std::getline(std::cin, configPath);

        // if (!configPath.empty())
        // {
        //     try
        //     {
        //         grabber.runScript(configPath);
        //         std::cout << "Configuration script loaded successfully" << std::endl;
        //     }
        //     catch (const std::exception &e)
        //     {
        //         std::cout << "Warning: Failed to load configuration script: " << e.what() << std::endl;
        //     }
        // }

        // Continue with your existing initialization and grabbing logic
        ImageParams params = initializeGrabber(grabber);
        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        CircularBuffer processingBuffer(params.bufferCount, params.imageSize);
        SharedResources shared;
        initializeBackgroundFrame(shared, params);
        shared.roi = cv::Rect(0, 0, params.width, params.height);

        temp_sample(grabber, params, circularBuffer, processingBuffer, shared);
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

// int mib_grabber_main()
// {
//     try
//     {

//         EGenTL genTL;
//         // EGrabberDiscovery egrabberDiscovery(genTL);
//         // egrabberDiscovery.discover();
//         EGrabber<CallbackOnDemand> grabber(genTL);
//         // grabber.runScript("check-config.js"); // figureout how to load config script

//         // configure(grabber);
//         ImageParams params = initializeGrabber(grabber);

//         CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
//         CircularBuffer processingBuffer(params.bufferCount, params.imageSize);
//         SharedResources shared;
//         initializeBackgroundFrame(shared, params);
//         shared.roi = cv::Rect(0, 0, params.width, params.height);

//         temp_sample(grabber, params, circularBuffer, processingBuffer, shared);
//     }
//     catch (const std::exception &e)
//     {
//         std::cout << "error: " << e.what() << std::endl;
//     }
//     return 0;
// }