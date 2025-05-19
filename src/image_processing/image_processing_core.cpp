#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include <cmath>
#include <algorithm> // For std::sort and std::nth_element

ThreadLocalMats initializeThreadMats(int height, int width, SharedResources &shared)
{
    std::lock_guard<std::mutex> lock(shared.processingConfigMutex);
    ThreadLocalMats mats;
    mats.blurred_target = cv::Mat(height, width, CV_8UC1);
    mats.enhanced = cv::Mat(height, width, CV_8UC1);
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

    // Get ROI from the background
    // Note: The background is already blurred and contrast-enhanced with the same parameters
    cv::Mat blurred_bg = shared.blurredBackground(roi);

    // Process only ROI area
    auto roiArea = inputImage(roi);

    // Apply Gaussian blur to reduce noise - same as applied to background
    cv::GaussianBlur(roiArea, mats.blurred_target(roi),
                     cv::Size(shared.processingConfig.gaussian_blur_size,
                              shared.processingConfig.gaussian_blur_size),
                     0);

    // Apply simple contrast enhancement if enabled
    if (shared.processingConfig.enable_contrast_enhancement)
    {
        // Use the formula: new_pixel = alpha * old_pixel + beta
        // alpha > 1 increases contrast, beta increases brightness
        mats.blurred_target(roi).convertTo(mats.enhanced(roi), -1,
                                           shared.processingConfig.contrast_alpha,
                                           shared.processingConfig.contrast_beta);

        // Perform background subtraction with the enhanced image
        cv::subtract(mats.enhanced(roi), blurred_bg, mats.bg_sub(roi));
    }
    else
    {
        // Simple background subtraction without contrast enhancement
        cv::subtract(mats.blurred_target(roi), blurred_bg, mats.bg_sub(roi));
    }

    // Apply threshold to create binary image
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

double calculateRingRatio(const std::vector<cv::Point>& innerContour, const std::vector<cv::Point>& outerContour)
{
    // Calculate areas of both contours
    double innerArea = cv::contourArea(innerContour);
    double outerArea = cv::contourArea(outerContour);
    
    // Prevent division by zero
    if (outerArea <= 0) {
        return 0.0;
    }
    
    // Calculate the ratio of inner area to outer area and return sqrt of it
    return  std::sqrt(outerArea-innerArea);
}

std::tuple<std::vector<std::vector<cv::Point>>, bool, std::vector<std::vector<cv::Point>>, std::vector<int>> findContours(const cv::Mat &processedImage)
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
    std::vector<int> parentIndices; // Store parent indices for inner contours

    // Process hierarchy to find inner contours
    for (size_t i = 0; i < filteredHierarchy.size(); i++)
    {
        // h[3] > -1 means this contour has a parent (it's an inner contour)
        if (filteredHierarchy[i][3] > -1)
        {
            hasNestedContours = true;
            innerContours.push_back(filteredContours[i]);
            
            // Store the index of the parent contour in the filtered contours array
            int parentIdx = filteredHierarchy[i][3];
            // Need to map from original hierarchy index to filtered index
            int filteredParentIdx = -1;
            for (size_t j = 0; j < filteredContours.size(); j++) {
                if (j == parentIdx) {
                    filteredParentIdx = j;
                    break;
                }
            }
            parentIndices.push_back(filteredParentIdx);
        }
    }

    return std::make_tuple(filteredContours, hasNestedContours, innerContours, parentIndices);
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
                                  const ProcessingConfig &config, const uint8_t processedColor,
                                  const cv::Mat &originalImage)
{
    // Initialize result with default values
    // isValid is now false by default, will be set to true if criteria are met
    FilterResult result = {false, false, false, false, 0, 0.0, 0.0, 0.0, 0.0, BrightnessQuantiles()};

    // Get ROI from processed image
    cv::Mat roiImage = processedImage(roi);

    // First, find contours in the entire processed image
    auto [contours, hasNestedContours, innerContours, parentIndices] = findContours(processedImage);

    // Update inner contour information
    result.innerContourCount = static_cast<int>(innerContours.size());
    result.hasSingleInnerContour = (innerContours.size() == 1);

    // Calculate brightness quantiles if the original image is provided
    if (!originalImage.empty()) {
        result.brightness = calculateBrightnessQuantiles(originalImage, processedImage);
    }

    // If we require a single inner contour and don't have exactly one, return early
    if (config.require_single_inner_contour && !result.hasSingleInnerContour)
    {
        // For simplicity, we only process objects with exactly one inner contour
        return result;
    }

    // Border check using contours instead of raw pixels
    if (config.enable_border_check)
    {
        const int borderThreshold = 2; // Pixels from border to consider as "touching"

        // If we have inner contours, check if the inner contour touches the border
        if (!innerContours.empty())
        {
            // We only care about the first inner contour (we've already checked for single inner contour above)
            const auto &innerContour = innerContours[0];

            // Check if any point of the inner contour is too close to the border
            for (const auto &point : innerContour)
            {
                // Convert point to ROI coordinates
                int x = point.x - roi.x;
                int y = point.y - roi.y;

                // Check if point is within the ROI (safety check)
                if (x >= 0 && x < roi.width && y >= 0 && y < roi.height)
                {
                    // Check if point is too close to any border
                    if (x < borderThreshold || x >= roi.width - borderThreshold ||
                        y < borderThreshold || y >= roi.height - borderThreshold)
                    {
                        result.touchesBorder = true;
                        break;
                    }
                }
                else
                {
                    // Point is outside ROI, definitely touching border
                    result.touchesBorder = true;
                    break;
                }
            }
        }
        else if (!contours.empty())
        {
            // If no inner contours, check outer contours
            for (const auto &contour : contours)
            {
                // Check if any point of the contour is too close to the border
                for (const auto &point : contour)
                {
                    // Convert point to ROI coordinates
                    int x = point.x - roi.x;
                    int y = point.y - roi.y;

                    // Check if point is within the ROI (safety check)
                    if (x >= 0 && x < roi.width && y >= 0 && y < roi.height)
                    {
                        // Check if point is too close to any border
                        if (x < borderThreshold || x >= roi.width - borderThreshold ||
                            y < borderThreshold || y >= roi.height - borderThreshold)
                        {
                            result.touchesBorder = true;
                            break;
                        }
                    }
                    else
                    {
                        // Point is outside ROI, definitely touching border
                        result.touchesBorder = true;
                        break;
                    }
                }

                if (result.touchesBorder)
                {
                    break; // No need to check other contours
                }
            }
        }
    }

    // Only proceed with contour analysis if no border pixels were found or border check is disabled
    if (!result.touchesBorder || !config.enable_border_check)
    {
        // If we have a single inner contour, use it for metrics
        if (result.hasSingleInnerContour)
        {
            // We have exactly one inner contour - use it for metrics
            // Calculate contour area
            double contourArea = cv::contourArea(innerContours[0]);

            // Calculate convex hull
            std::vector<cv::Point> hull;
            cv::convexHull(innerContours[0], hull);
            double hullArea = cv::contourArea(hull);

            // Calculate area ratio (R = Ahull/Acontour)
            result.areaRatio = hullArea / contourArea;

            auto [deformability, area] = calculateMetrics(innerContours[0]);
            result.deformability = deformability;
            result.area = area;

            // Calculate ring ratio using the parent contour information
            if (result.hasSingleInnerContour && parentIndices.size() > 0)
            {
                int parentIdx = parentIndices[0];
                if (parentIdx >= 0 && parentIdx < contours.size())
                {
                    // Calculate the ring ratio using the inner contour and its parent outer contour
                    result.ringRatio = calculateRingRatio(innerContours[0], contours[parentIdx]);
                }
            }

            // Check area range only if that check is enabled
            if (!config.enable_area_range_check ||
                (area >= config.area_threshold_min && area <= config.area_threshold_max))
            {
                result.inRange = true;
                // Set isValid to true for frames with exactly one inner contour
                result.isValid = true;
            }
        }
        // If no inner contours but we have contours, use the largest one
        else if (!contours.empty() && !config.require_single_inner_contour)
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
        // Red for border touching (highest priority rejection reason)
        return cv::Scalar(0, 0, 255); // BGR: Red
    }
    else if (result.hasSingleInnerContour && isValid)
    {
        // Bright Green for valid frames with exactly one inner contour (highest priority for keeping)
        // These are the frames we want to keep - using the single inner contour for metrics
        return cv::Scalar(0, 255, 0); // BGR: Bright Green
    }
    else if (isValid)
    {
        // Yellow for valid frames without inner contours (lower priority)
        // These are acceptable but not ideal - we prefer frames with exactly one inner contour
        return cv::Scalar(0, 255, 255); // BGR: Yellow
    }
    else
    {
        // Gray for invalid frames without inner contours (lowest priority)
        return cv::Scalar(128, 128, 128); // BGR: Gray
    }
}

BrightnessQuantiles calculateBrightnessQuantiles(const cv::Mat &originalImage, const cv::Mat &mask)
{
    BrightnessQuantiles result;
    
    // Convert original image to grayscale if it's not already
    cv::Mat grayImage;
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, grayImage, cv::COLOR_BGR2GRAY);
    } else {
        grayImage = originalImage.clone();
    }
    
    // Extract brightness values from the masked area
    std::vector<uchar> brightness;
    brightness.reserve(grayImage.rows * grayImage.cols / 4); // Approximate size
    
    for (int y = 0; y < grayImage.rows; y++) {
        for (int x = 0; x < grayImage.cols; x++) {
            // Check if this pixel is part of the mask
            if (mask.at<uchar>(y, x) > 0) {
                brightness.push_back(grayImage.at<uchar>(y, x));
            }
        }
    }
    
    // If no masked pixels were found, return zeros
    if (brightness.empty()) {
        return result;
    }
    
    // Sort the values to compute quantiles
    std::sort(brightness.begin(), brightness.end());
    
    // Calculate quantile positions
    size_t n = brightness.size();
    size_t q1_pos = n / 4;
    size_t q2_pos = n / 2;
    size_t q3_pos = (3 * n) / 4;
    size_t q4_pos = n - 1;
    
    // Extract quantile values
    result.q1 = brightness[q1_pos];
    result.q2 = brightness[q2_pos];
    result.q3 = brightness[q3_pos];
    result.q4 = brightness[q4_pos];
    
    return result;
}