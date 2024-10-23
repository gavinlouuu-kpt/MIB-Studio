#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"

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

void processFrame(const cv::Mat &inputImage, SharedResources &shared,
                  cv::Mat &outputImage, bool isProcessingThread)
{
    // Choose the appropriate set of thread-local variables
    ThreadLocalMats &mats = isProcessingThread ? processingThreadMats : displayThreadMats;

    // Initialize thread_local variables only once for each thread
    if (!mats.initialized)
    {
        int height = inputImage.rows;
        int width = inputImage.cols;
        mats.blurred_target = cv::Mat(height, width, CV_8UC1);
        mats.bg_sub = cv::Mat(height, width, CV_8UC1);
        mats.binary = cv::Mat(height, width, CV_8UC1);
        mats.dilate1 = cv::Mat(height, width, CV_8UC1);
        mats.erode1 = cv::Mat(height, width, CV_8UC1);
        mats.erode2 = cv::Mat(height, width, CV_8UC1);
        mats.kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        mats.initialized = true;
    }

    // Direct access without locks
    cv::Rect roi = shared.roi;

    // Ensure ROI is within image bounds
    roi &= cv::Rect(0, 0, inputImage.cols, inputImage.rows);

    // Direct access to background
    cv::Mat blurred_bg = shared.blurredBackground(roi);

    // Optimize morphological operations
    static const int iterations = 1;

    // Process only ROI area
    auto roiArea = inputImage(roi);
    cv::GaussianBlur(roiArea, mats.blurred_target(roi), cv::Size(3, 3), 0);
    cv::subtract(blurred_bg, mats.blurred_target(roi), mats.bg_sub(roi));
    cv::threshold(mats.bg_sub(roi), mats.binary(roi), 10, 255, cv::THRESH_BINARY);

    // Combine operations to reduce memory transfers
    cv::morphologyEx(mats.binary(roi), mats.dilate1(roi), cv::MORPH_CLOSE, mats.kernel,
                     cv::Point(-1, -1), iterations);
    cv::morphologyEx(mats.dilate1(roi), outputImage(roi), cv::MORPH_OPEN, mats.kernel,
                     cv::Point(-1, -1), iterations);

    // Use more efficient mask creation
    if (roi.width != inputImage.cols || roi.height != inputImage.rows)
    {
        cv::Mat mask(inputImage.rows, inputImage.cols, CV_8UC1, cv::Scalar(0));
        mask(roi) = 255;
        outputImage.setTo(0, mask == 0);
    }
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
    double deformability = 1.0 - circularity;
    return std::make_tuple(deformability, area);
}
