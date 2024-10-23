#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"

thread_local struct ThreadLocalMats
{
    cv::Mat original;
    cv::Mat blurred_target;
    cv::Mat binary;
    cv::Mat kernel;
    bool initialized = false;
} processingThreadMats, displayThreadMats;

void processFrame(const std::vector<uint8_t> &imageData, size_t width, size_t height,
                  SharedResources &shared, cv::Mat &outputImage, bool isProcessingThread)
{
    // Choose the appropriate set of thread-local variables
    ThreadLocalMats &mats = isProcessingThread ? processingThreadMats : displayThreadMats;

    // Initialize thread_local variables only once for each thread
    if (!mats.initialized)
    {
        // Only initialize the Mats we actually need
        mats.original = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
        mats.blurred_target = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
        mats.binary = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
        mats.kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        mats.initialized = true;
    }

    // Create OpenCV Mat from the image data
    mats.original = cv::Mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1, const_cast<uint8_t *>(imageData.data()));

    cv::Rect roi;
    {
        std::lock_guard<std::mutex> lock(shared.roiMutex);
        roi = shared.roi;
    }

    // Ensure ROI is within image bounds
    roi &= cv::Rect(0, 0, static_cast<int>(width), static_cast<int>(height));

    // Check if ROI is the same as the full image
    if (static_cast<size_t>(roi.width) == width && static_cast<size_t>(roi.height) == height)
    {
        mats.original.copyTo(outputImage);
        return;
    }

    // Access the pre-blurred background
    cv::Mat blurred_bg;
    {
        std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
        blurred_bg = shared.blurredBackground(roi);
    }

    // Process only the ROI region
    cv::Mat roiOutput = outputImage(roi);

    // Perform operations directly on ROI
    cv::GaussianBlur(mats.original(roi), mats.blurred_target(roi), cv::Size(3, 3), 0);
    cv::subtract(blurred_bg, mats.blurred_target(roi), mats.binary(roi));
    cv::threshold(mats.binary(roi), roiOutput, 10, 255, cv::THRESH_BINARY);

    // Combine erode and dilate operations
    cv::morphologyEx(roiOutput, roiOutput, cv::MORPH_CLOSE, mats.kernel, cv::Point(-1, -1), 1);
    cv::morphologyEx(roiOutput, roiOutput, cv::MORPH_OPEN, mats.kernel, cv::Point(-1, -1), 1);

    // Clear areas outside ROI
    outputImage.setTo(0);
    roiOutput.copyTo(outputImage(roi));
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
