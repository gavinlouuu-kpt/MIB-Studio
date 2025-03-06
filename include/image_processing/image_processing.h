#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include "CircularBuffer/CircularBuffer.h"

#define M_PI 3.14159265358979323846 // pi

using json = nlohmann::json;

class CircularBuffer;

struct ImageParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

struct QualifiedResult
{
    // ContourResult contourResult;
    int64_t timestamp;
    double areaRatio;
    double area;
    double deformability;

    cv::Mat originalImage;
};

struct ProcessingConfig
{
    ProcessingConfig(
        int gaussian = 3,
        int threshold = 10,
        int kernel = 3,
        int iterations = 1,
        int min_area = 100,
        int max_area = 600,
        bool check_borders = true,
        bool check_multiple_contours = true,
        bool check_area_range = true,
        bool require_single_inner = true,
        bool enable_contrast = true,
        double alpha = 1.2,
        int beta = 10) : gaussian_blur_size(gaussian),
                         bg_subtract_threshold(threshold),
                         morph_kernel_size(kernel),
                         morph_iterations(iterations),
                         area_threshold_min(min_area),
                         area_threshold_max(max_area),
                         enable_border_check(check_borders),
                         enable_area_range_check(check_area_range),
                         require_single_inner_contour(require_single_inner),
                         enable_contrast_enhancement(enable_contrast),
                         contrast_alpha(alpha),
                         contrast_beta(beta) {}

    int gaussian_blur_size;
    int bg_subtract_threshold;
    int morph_kernel_size;
    int morph_iterations;
    int area_threshold_min;
    int area_threshold_max;

    // Filter flags
    bool enable_border_check;
    bool enable_multiple_contours_check;
    bool enable_area_range_check;
    bool require_single_inner_contour; // Require exactly one inner contour

    // Contrast enhancement parameters
    bool enable_contrast_enhancement; // Toggle for contrast enhancement
    double contrast_alpha;            // Contrast multiplier (1.0 = no change, >1.0 = increase contrast)
    int contrast_beta;                // Brightness adjustment (0 = no change, >0 = increase brightness)
};

struct ThreadLocalMats
{
    cv::Mat original;
    cv::Mat blurred_target;
    cv::Mat enhanced;
    cv::Mat bg_sub;
    cv::Mat binary;
    cv::Mat dilate1;
    cv::Mat erode1;
    cv::Mat erode2;
    cv::Mat kernel;
    bool initialized = false;
};

struct FilterResult
{
    bool isValid;
    bool touchesBorder;
    bool hasSingleInnerContour; // Specifically indicates exactly one inner contour
    bool inRange;
    int innerContourCount; // Number of inner contours detected
    double deformability;
    double area;
    double areaRatio;
};

struct SharedResources
{

    std::atomic<bool> done{false};
    std::atomic<bool> paused{false};
    std::function<void(int)> keyboardCallback;
    std::mutex keyboardMutex;
    std::atomic<bool> overlayMode{false};
    std::atomic<int> currentFrameIndex{-1};
    std::atomic<bool> displayNeedsUpdate{false};
    std::atomic<int> currentBatchNumber{0};

    std::atomic<size_t> latestCameraFrame{0}; // for simulated camera
    std::atomic<size_t> frameRateCount{0};    // for simulated camera
    std::queue<size_t> framesToProcess;
    std::queue<size_t> framesToDisplay;
    std::mutex displayQueueMutex;
    std::mutex processingQueueMutex;
    std::condition_variable displayQueueCondition;
    std::condition_variable processingQueueCondition;
    // std::vector<std::tuple<double, double>> deformabilities;
    // std::mutex deformabilitiesMutex;
    std::atomic<bool> newScatterDataAvailable{false};
    std::condition_variable scatterDataCondition;
    cv::Mat backgroundFrame;
    cv::Mat blurredBackground;
    std::mutex backgroundFrameMutex;
    cv::Rect roi;
    std::mutex roiMutex;

    std::atomic<bool> running{false};
    std::vector<QualifiedResult> qualifiedResults;
    std::mutex qualifiedResultsMutex;
    std::vector<QualifiedResult> qualifiedResultsBuffer1;
    std::vector<QualifiedResult> qualifiedResultsBuffer2;
    std::atomic<bool> usingBuffer1{true};
    std::condition_variable savingCondition;
    std::atomic<bool> savingInProgress{false};
    std::atomic<size_t> totalSavedResults{0};
    std::chrono::steady_clock::time_point lastSaveTime;
    std::atomic<double> diskSaveTime;
    std::string saveDirectory;
    // metrics
    CircularBuffer processingTimes{1000, sizeof(double)};                          // Buffer to store last 1000 processing times
    CircularBuffer deformabilityBuffer{10000, sizeof(std::tuple<double, double>)}; // Buffer for last 1000 deformability measurements
    std::mutex deformabilityBufferMutex;
    std::atomic<double> currentFPS;
    std::atomic<double> dataRate;
    std::atomic<uint64_t> exposureTime;
    std::atomic<size_t> imagesInQueue;
    std::atomic<size_t> qualifiedResultCount;
    // frameDeformabilities and frameAreas are used for review
    std::atomic<double> frameDeformabilities;
    std::atomic<double> frameAreas;
    std::atomic<double> frameAreaRatios;
    // std::atomic<size_t> totalFramesProcessed;
    std::atomic<bool> updated;
    std::atomic<bool> validProcessingFrame{false};
    std::atomic<bool> validDisplayFrame{false};
    std::atomic<bool> displayFrameTouchedBorder{false};
    std::atomic<bool> hasSingleInnerContour{false};
    std::atomic<int> innerContourCount{0};
    std::atomic<bool> usingInnerContour{false};
    // std::atomic<double> linearProcessingTime;
    std::atomic<int64_t> triggerOnsetDuration{0}; // Store the trigger onset duration in microseconds

    ProcessingConfig processingConfig;
    std::mutex processingConfigMutex;
    std::atomic<bool> triggerOut{false};
    std::atomic<bool> processTrigger{false};
};

// Function declarations
ImageParams initializeImageParams(const std::string &directory);
void loadImages(const std::string &directory, CircularBuffer &cameraBuffer, bool reverseOrder = false);
void initializeMockBackgroundFrame(SharedResources &shared, const ImageParams &params, const CircularBuffer &cameraBuffer);
void processFrame(const cv::Mat &inputImage, SharedResources &shared,
                  cv::Mat &outputImage, ThreadLocalMats &mats);
std::tuple<std::vector<std::vector<cv::Point>>, bool, std::vector<std::vector<cv::Point>>> findContours(const cv::Mat &processedImage);
std::tuple<double, double> calculateMetrics(const std::vector<cv::Point> &contour);

void onTrackbar(int pos, void *userdata);
// void updateScatterPlot(cv::Mat &plot, const std::vector<std::tuple<double, double>> &circularities);

// void saveQualifiedResultsToDisk(const std::vector<QualifiedResult> &results, const std::string &directory);
void saveQualifiedResultsToDisk(const std::vector<QualifiedResult> &results, const std::string &directory, const SharedResources &shared);

void convertSavedImagesToStandardFormat(const std::string &binaryImageFile, const std::string &outputDirectory);
json readConfig(const std::string &filename);
ProcessingConfig getProcessingConfig(const json &config);

bool updateConfig(const std::string &filename, const std::string &key, const json &value);

void updateBackgroundWithCurrentSettings(SharedResources &shared);

void temp_mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared);

void simulateCameraThread(CircularBuffer &cameraBuffer, SharedResources &shared, const ImageParams &params);
void setupCommonThreads(SharedResources &shared, const std::string &saveDir,
                        const CircularBuffer &circularBuffer, const CircularBuffer &processingBuffer, const ImageParams &params,
                        std::vector<std::thread> &threads);
void commonSampleLogic(SharedResources &shared, const std::string &SAVE_DIRECTORY,
                       std::function<std::vector<std::thread>(SharedResources &, const std::string &)> setupThreads);

ThreadLocalMats initializeThreadMats(int height, int width, SharedResources &shared);

void reviewSavedData();

FilterResult filterProcessedImage(const cv::Mat &processedImage, const cv::Rect &roi,
                                  const ProcessingConfig &config, const uint8_t processedColor = 255);

// Function to determine overlay color based on FilterResult
cv::Scalar determineOverlayColor(const FilterResult &result, bool isValid);