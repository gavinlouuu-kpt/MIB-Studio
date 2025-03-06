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

FilterResult filterProcessedImage(const cv::Mat &processedImage, const cv::Rect &roi,
                                  const ProcessingConfig &config, const uint8_t processedColor)
{
    FilterResult result = {false, false, false, false, false, 0.0, 0.0, 0.0};

    // Get ROI from processed image
    cv::Mat roiImage = processedImage(roi);

    // Check borders only if border check is enabled
    if (config.enable_border_check)
    {
        const int borderThreshold = 2;

        // Check left and right borders
        for (int y = 0; y < roiImage.rows && !result.touchesBorder; y++)
        {
            // Check left border region
            for (int x = 0; x < borderThreshold; x++)
            {
                if (roiImage.at<uint8_t>(y, x) == processedColor)
                {
                    result.touchesBorder = true;
                    break;
                }
            }

            // Check right border region
            for (int x = roiImage.cols - borderThreshold; x < roiImage.cols && !result.touchesBorder; x++)
            {
                if (roiImage.at<uint8_t>(y, x) == processedColor)
                {
                    result.touchesBorder = true;
                    break;
                }
            }
        }

        // Check top and bottom borders
        for (int x = 0; x < roiImage.cols && !result.touchesBorder; x++)
        {
            // Check top border region
            for (int y = 0; y < borderThreshold; y++)
            {
                if (roiImage.at<uint8_t>(y, x) == processedColor)
                {
                    result.touchesBorder = true;
                    break;
                }
            }

            // Check bottom border region
            for (int y = roiImage.rows - borderThreshold; y < roiImage.rows && !result.touchesBorder; y++)
            {
                if (roiImage.at<uint8_t>(y, x) == processedColor)
                {
                    result.touchesBorder = true;
                    break;
                }
            }
        }
    }

    // Only proceed with contour detection if no border pixels were found or border check is disabled
    if (!result.touchesBorder || !config.enable_border_check)
    {
        auto [contours, hasNestedContours, innerContours] = findContours(processedImage);

        // Set hasNestedContours flag if nested contours check is enabled
        if (config.enable_nested_contours_check)
        {
            result.hasNestedContours = hasNestedContours;
        }

        // Find significant contours (those that are large enough to be considered for analysis)
        std::vector<std::vector<cv::Point>> significantContours;
        std::vector<double> contourAreas;

        // Minimum area ratio compared to the largest contour to be considered significant
        const double significanceThreshold = 0.2; // 20% of the largest contour's area

        // Find the largest contour and its area
        double largestArea = 0.0;
        for (const auto &contour : contours)
        {
            double area = cv::contourArea(contour);
            largestArea = std::max(largestArea, area);
        }

        // Filter contours based on significance
        for (const auto &contour : contours)
        {
            double area = cv::contourArea(contour);
            if (area >= largestArea * significanceThreshold)
            {
                significantContours.push_back(contour);
                contourAreas.push_back(area);
            }
        }

        // Check for multiple contours only if that check is enabled
        if (config.enable_multiple_contours_check && significantContours.size() > 1)
        {
            // Only set hasMultipleContours if we have multiple significant outer contours
            // or multiple significant inner contours

            // Count significant inner contours
            int significantInnerCount = 0;
            for (const auto &innerContour : innerContours)
            {
                double area = cv::contourArea(innerContour);
                if (area >= largestArea * significanceThreshold)
                {
                    significantInnerCount++;
                }
            }

            // If we have multiple significant inner contours, reject
            if (significantInnerCount > 1)
            {
                result.hasMultipleContours = true;
                return result;
            }

            // If we have multiple significant outer contours without inner contours, reject
            if (!hasNestedContours && significantContours.size() > 1)
            {
                result.hasMultipleContours = true;
                return result;
            }
        }

        // If we have inner contours, use the largest one for metrics
        if (hasNestedContours && !innerContours.empty())
        {
            // Find the largest inner contour
            size_t largestInnerIdx = 0;
            double largestInnerArea = 0.0;

            for (size_t i = 0; i < innerContours.size(); i++)
            {
                double area = cv::contourArea(innerContours[i]);
                if (area > largestInnerArea)
                {
                    largestInnerArea = area;
                    largestInnerIdx = i;
                }
            }

            // Calculate contour area
            double contourArea = cv::contourArea(innerContours[largestInnerIdx]);

            // Calculate convex hull
            std::vector<cv::Point> hull;
            cv::convexHull(innerContours[largestInnerIdx], hull);
            double hullArea = cv::contourArea(hull);

            // Calculate area ratio (R = Ahull/Acontour)
            result.areaRatio = hullArea / contourArea;

            auto [deformability, area] = calculateMetrics(innerContours[largestInnerIdx]);
            result.deformability = deformability;
            result.area = area;

            // Check area range only if that check is enabled
            if (!config.enable_area_range_check ||
                (area >= config.area_threshold_min && area <= config.area_threshold_max))
            {
                result.inRange = true;
                result.isValid = true;
            }
        }
        // If no nested contours but we have contours, use the largest one
        else if (!contours.empty())
        {
            // Find the largest contour
            size_t largestIdx = 0;
            double largestOuterArea = 0.0;

            for (size_t i = 0; i < contours.size(); i++)
            {
                double area = cv::contourArea(contours[i]);
                if (area > largestOuterArea)
                {
                    largestOuterArea = area;
                    largestIdx = i;
                }
            }

            // Calculate contour area
            double contourArea = cv::contourArea(contours[largestIdx]);

            // Calculate convex hull
            std::vector<cv::Point> hull;
            cv::convexHull(contours[largestIdx], hull);
            double hullArea = cv::contourArea(hull);

            // Calculate area ratio (R = Ahull/Acontour)
            result.areaRatio = hullArea / contourArea;

            auto [deformability, area] = calculateMetrics(contours[largestIdx]);
            result.deformability = deformability;
            result.area = area;

            // Check area range only if that check is enabled
            if (!config.enable_area_range_check ||
                (area >= config.area_threshold_min && area <= config.area_threshold_max))
            {
                result.inRange = true;
                result.isValid = true;
            }
        }
    }

    return result;
}

cv::Scalar determineOverlayColor(const FilterResult &result, bool isValid)
{
    // Hierarchical condition checking based solely on FilterResult
    if (result.touchesBorder)
    {
        // Red for border touching (highest priority)
        return cv::Scalar(0, 0, 255); // BGR: Red
    }
    else if (result.hasMultipleContours)
    {
        // Blue for multiple significant contours (rejected frames)
        return cv::Scalar(255, 0, 0); // BGR: Blue
    }
    else if (result.hasNestedContours && isValid)
    {
        // Green for using inner contour (valid frame with inner contour)
        return cv::Scalar(0, 255, 0); // BGR: Green
    }
    else if (result.hasNestedContours)
    {
        // Purple for nested contours detected but not used
        return cv::Scalar(255, 0, 255); // BGR: Purple
    }
    else if (isValid)
    {
        // Green for valid frames (that passed all filters)
        return cv::Scalar(0, 255, 0); // BGR: Green
    }
    else
    {
        // Yellow for frames filtered out due to being out of range
        return cv::Scalar(0, 255, 255); // BGR: Yellow
    }
}