#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"

ThreadLocalMats initializeThreadMats(int height, int width, SharedResources &shared)
{
    std::lock_guard<std::mutex> lock(shared.processingConfigMutex);
    ThreadLocalMats mats;
    mats.blurred_target = cv::Mat(height, width, CV_8UC1);
    mats.bg_sub = cv::Mat(height, width, CV_8UC1);
    mats.binary = cv::Mat(height, width, CV_8UC1);
    mats.dilate1 = cv::Mat(height, width, CV_8UC1);
    mats.erode1 = cv::Mat(height, width, CV_8UC1);
    mats.erode2 = cv::Mat(height, width, CV_8UC1);
    mats.kernel = cv::getStructuringElement(cv::MORPH_CROSS,
                                            cv::Size(shared.processingConfig.morph_kernel_size,
                                                     shared.processingConfig.morph_kernel_size));
    mats.initialized = true;
    return mats;
}

void processFrame(const cv::Mat &inputImage, SharedResources &shared,
                  cv::Mat &outputImage, ThreadLocalMats &mats)
{
    std::lock_guard<std::mutex> lock(shared.processingConfigMutex);
    cv::Rect roi = shared.roi;
    // Ensure ROI is within image bounds
    roi &= cv::Rect(0, 0, inputImage.cols, inputImage.rows);
    // Direct access to background
    cv::Mat blurred_bg = shared.blurredBackground(roi);
    // Process only ROI area
    auto roiArea = inputImage(roi);
    cv::GaussianBlur(roiArea, mats.blurred_target(roi),
                     cv::Size(shared.processingConfig.gaussian_blur_size,
                              shared.processingConfig.gaussian_blur_size),
                     0);
    cv::subtract(blurred_bg, mats.blurred_target(roi), mats.bg_sub(roi));
    cv::threshold(mats.bg_sub(roi), mats.binary(roi),
                  shared.processingConfig.bg_subtract_threshold, 255, cv::THRESH_BINARY);
    // Combine operations to reduce memory transfers
    cv::morphologyEx(mats.binary(roi), mats.dilate1(roi), cv::MORPH_CLOSE, mats.kernel,
                     cv::Point(-1, -1), shared.processingConfig.morph_iterations);
    cv::morphologyEx(mats.dilate1(roi), outputImage(roi), cv::MORPH_OPEN, mats.kernel,
                     cv::Point(-1, -1), shared.processingConfig.morph_iterations);

    if (roi.width != inputImage.cols || roi.height != inputImage.rows)
    {
        cv::Mat mask(inputImage.rows, inputImage.cols, CV_8UC1, cv::Scalar(0));
        mask(roi) = 255;
        outputImage.setTo(0, mask == 0);
    }
}

std::vector<std::vector<cv::Point>> findContours(const cv::Mat &processedImage)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processedImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    return contours;
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
