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
    grabber.setInteger<RemoteModule>("Width", 512);
    grabber.setInteger<RemoteModule>("Height", 96);
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

static cv::Mat background, blurred_bg;
static cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

void processFrame(const cv::Mat &original, cv::Mat &output)
{
    if (background.empty())
    {
        background = cv::Mat(original.size(), CV_8UC1, cv::Scalar(255));
        cv::GaussianBlur(background, blurred_bg, cv::Size(3, 3), 0);
    }

    cv::GaussianBlur(original, output, cv::Size(3, 3), 0);
    cv::subtract(blurred_bg, output, output);
    cv::threshold(output, output, 10, 255, cv::THRESH_BINARY);

    // Perform closing operation (dilate then erode)
    cv::morphologyEx(output, output, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
    cv::morphologyEx(output, output, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(output, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
}

void sample(EGrabber<CallbackOnDemand> &grabber, const GrabberParams &params)
{
    // cv::ocl::setUseOpenCL(true);

    grabber.start();

    const int numFrames = 5000;
    std::vector<long long> processingTimes;
    processingTimes.reserve(numFrames);

    cv::Mat original(params.height, params.width, CV_8UC1);
    cv::Mat processed(params.height, params.width, CV_8UC1);

    for (int i = 0; i < numFrames; ++i)
    {
        // Capture a single frame
        ScopedBuffer buffer(grabber);
        uint8_t *imagePointer = buffer.getInfo<uint8_t *>(gc::BUFFER_INFO_BASE);

        // Create OpenCV Mat from the image data
        std::memcpy(original.data, imagePointer, params.width * params.height);

        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();

        processFrame(original, processed);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        processingTimes.push_back(duration.count());
    }

    grabber.stop();

    // Sort the processing times
    std::sort(processingTimes.begin(), processingTimes.end());

    // Calculate the number of values to exclude (5% from each end)
    int excludeCount = static_cast<int>(numFrames * 0.05);

    // Calculate statistics excluding extreme values
    double sum = 0;
    long long min = processingTimes[excludeCount];
    long long max = processingTimes[numFrames - excludeCount - 1];

    for (int i = excludeCount; i < numFrames - excludeCount; ++i)
    {
        sum += processingTimes[i];
    }

    int validCount = numFrames - 2 * excludeCount;
    double mean = sum / validCount;

    double variance = 0;
    for (int i = excludeCount; i < numFrames - excludeCount; ++i)
    {
        double diff = processingTimes[i] - mean;
        variance += diff * diff;
    }
    variance /= validCount;
    double stdDev = std::sqrt(variance);

    // Print results
    std::cout << "Processing time statistics (excluding extreme 10% values):" << std::endl;
    std::cout << "Min: " << min << " us" << std::endl;
    std::cout << "Max: " << max << " us" << std::endl;
    std::cout << "Mean: " << mean << " us" << std::endl;
    std::cout << "Standard Deviation: " << stdDev << " us" << std::endl;
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
