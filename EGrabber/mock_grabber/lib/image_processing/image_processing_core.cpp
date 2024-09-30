#include "image_processing.h"
#include "CircularBuffer.h"

thread_local struct ThreadLocalMats
{
    cv::Mat original;
    cv::Mat blurred_target;
    cv::Mat bg_sub;
    cv::Mat binary;
    cv::Mat dilate1;
    cv::Mat erode1;
    cv::Mat erode2;
    cv::Mat kernel;
    bool initialized = false;
} processingThreadMats, displayThreadMats;

void processFrame(const std::vector<uint8_t> &imageData, size_t width, size_t height,
                  SharedResources &shared, cv::Mat &outputImage, bool isProcessingThread)
{
    ThreadLocalMats &mats = isProcessingThread ? processingThreadMats : displayThreadMats;

    if (!mats.initialized)
    {
        mats.original = cv::Mat(height, width, CV_8UC1);
        mats.blurred_target = cv::Mat(height, width, CV_8UC1);
        mats.bg_sub = cv::Mat(height, width, CV_8UC1);
        mats.binary = cv::Mat(height, width, CV_8UC1);
        mats.dilate1 = cv::Mat(height, width, CV_8UC1);
        mats.erode1 = cv::Mat(height, width, CV_8UC1);
        mats.erode2 = cv::Mat(height, width, CV_8UC1);
        mats.kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        mats.initialized = true;
    }

    mats.original = cv::Mat(height, width, CV_8UC1, const_cast<uint8_t *>(imageData.data()));

    cv::Mat blurred_bg;
    {
        std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
        blurred_bg = shared.blurredBackground;
    }

    cv::GaussianBlur(mats.original, mats.blurred_target, cv::Size(3, 3), 0);
    cv::subtract(blurred_bg, mats.blurred_target, mats.bg_sub);
    cv::threshold(mats.bg_sub, mats.binary, 10, 255, cv::THRESH_BINARY);
    cv::dilate(mats.binary, mats.dilate1, mats.kernel, cv::Point(-1, -1), 2);
    cv::erode(mats.dilate1, mats.erode1, mats.kernel, cv::Point(-1, -1), 2);
    cv::erode(mats.erode1, mats.erode2, mats.kernel, cv::Point(-1, -1), 1);
    cv::dilate(mats.erode2, outputImage, mats.kernel, cv::Point(-1, -1), 1);
}

ContourResult findContours(const cv::Mat &processedImage)
{
    std::vector<std::vector<cv::Point>> contours;
    auto start = std::chrono::high_resolution_clock::now();
    cv::findContours(processedImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    auto end = std::chrono::high_resolution_clock::now();
    double findTime = std::chrono::duration<double, std::micro>(end - start).count();
    return {contours, findTime};
}

std::tuple<double, double> calculateMetrics(const std::vector<cv::Point> &contour)
{
    cv::Moments m = cv::moments(contour);
    double area = m.m00;
    double perimeter = cv::arcLength(contour, true);
    double circularity = (perimeter > 0) ? 4 * M_PI * area / (perimeter * perimeter) : 0.0;
    return std::make_tuple(circularity, area);
}