#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>   // Add this line for std::accumulate
#include <algorithm> // Add this line for std::minmax_element
#include <EGrabber.h>
#include <opencv2/opencv.hpp>

using namespace Euresys;

struct GrabberParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

void configure(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.setInteger<RemoteModule>("Width", 256);
    grabber.setInteger<RemoteModule>("Height", 64);
    grabber.setInteger<RemoteModule>("AcquisitionFrameRate", 4700);
    // Add other configuration settings as needed
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

cv::Mat processFrame(const cv::Mat &original)
{
    cv::Mat blurred, bg_sub, binary, dilate1, erode1, erode2, dilate2;

    // Create a simple background (you may want to capture a real background frame)
    cv::Mat background(original.size(), CV_8UC1, cv::Scalar(255));
    cv::GaussianBlur(background, background, cv::Size(3, 3), 0);

    // Blur the target image
    cv::GaussianBlur(original, blurred, cv::Size(3, 3), 0);

    // Background subtraction
    cv::subtract(background, blurred, bg_sub);

    // Apply threshold
    cv::threshold(bg_sub, binary, 10, 255, cv::THRESH_BINARY);

    // Create kernel for morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    // Erode and dilate to remove noise
    cv::dilate(binary, dilate1, kernel, cv::Point(-1, -1), 2);
    cv::erode(dilate1, erode1, kernel, cv::Point(-1, -1), 2);
    cv::erode(erode1, erode2, kernel, cv::Point(-1, -1), 1);
    cv::dilate(erode2, dilate2, kernel, cv::Point(-1, -1), 1);

    return dilate2;
}

void sample(EGrabber<CallbackOnDemand> &grabber, const GrabberParams &params)
{
    grabber.start();

    const int numFrames = 5000;
    std::vector<long long> processingTimes;
    processingTimes.reserve(numFrames);

    for (int i = 0; i < numFrames; ++i)
    {
        // Capture a single frame
        ScopedBuffer buffer(grabber);
        uint8_t *imagePointer = buffer.getInfo<uint8_t *>(gc::BUFFER_INFO_BASE);

        // Create OpenCV Mat from the image data
        cv::Mat original(params.height, params.width, CV_8UC1, imagePointer);

        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();

        cv::Mat processed = processFrame(original);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        processingTimes.push_back(duration.count());
    }

    grabber.stop();

    // Calculate statistics
    auto minmax = std::minmax_element(processingTimes.begin(), processingTimes.end());
    long long minTime = *minmax.first;
    long long maxTime = *minmax.second;
    long long totalTime = std::accumulate(processingTimes.begin(), processingTimes.end(), 0LL);
    double meanTime = static_cast<double>(totalTime) / numFrames;

    std::cout << "Processing time statistics for " << numFrames << " frames:" << std::endl;
    std::cout << "  Minimum: " << minTime << " microseconds" << std::endl;
    std::cout << "  Maximum: " << maxTime << " microseconds" << std::endl;
    std::cout << "  Mean: " << meanTime << " microseconds" << std::endl;
}

int main()
{
    try
    {
        EGenTL genTL;
        EGrabberDiscovery egrabberDiscovery(genTL);
        egrabberDiscovery.discover();
        EGrabber<CallbackOnDemand> grabber(egrabberDiscovery.cameras(0));

        configure(grabber);
        GrabberParams params = initializeGrabber(grabber);
        sample(grabber, params);
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
