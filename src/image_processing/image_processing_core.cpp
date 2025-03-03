#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include <cmath>

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
    cv::subtract(mats.blurred_target(roi), blurred_bg, mats.bg_sub(roi));
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

std::tuple<std::vector<std::vector<cv::Point>>, bool, std::vector<std::vector<cv::Point>>> findContours(const cv::Mat &processedImage)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(processedImage, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Filter out small noise contours
    std::vector<std::vector<cv::Point>> filteredContours;
    std::vector<cv::Vec4i> filteredHierarchy;

    // Minimum area threshold to filter out noise (adjust as needed)
    const double minNoiseArea = 10.0;

    for (size_t i = 0; i < contours.size(); i++)
    {
        double area = cv::contourArea(contours[i]);
        if (area >= minNoiseArea)
        {
            filteredContours.push_back(contours[i]);
            if (i < hierarchy.size())
            {
                filteredHierarchy.push_back(hierarchy[i]);
            }
        }
    }

    // Check if there are nested contours by examining the hierarchy
    bool hasNestedContours = false;
    std::vector<std::vector<cv::Point>> innerContours;

    // Process hierarchy to find inner contours
    for (size_t i = 0; i < filteredHierarchy.size(); i++)
    {
        // h[3] > -1 means this contour has a parent (it's an inner contour)
        if (filteredHierarchy[i][3] > -1)
        {
            hasNestedContours = true;
            innerContours.push_back(filteredContours[i]);
        }
    }

    return std::make_tuple(filteredContours, hasNestedContours, innerContours);
}

std::tuple<double, double> calculateMetrics(const std::vector<cv::Point> &contour)
{
    cv::Moments m = cv::moments(contour);
    double area = m.m00;
    double perimeter = cv::arcLength(contour, true);
    // Updated formula: sqrt(4 * pi * area) / perimeter
    double circularity = (perimeter > 0) ? std::sqrt(4 * M_PI * area) / perimeter : 0.0; // DO NOT CHANGE THIS FORMULA
    double deformability = 1.0 - circularity;
    return std::make_tuple(deformability, area);
}
