#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <json.hpp>

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

struct ContourResult
{
    std::vector<std::vector<cv::Point>> contours;
    double findTime;
};

struct QualifiedResult
{
    ContourResult contourResult;
    cv::Mat originalImage;
    std::chrono::system_clock::time_point timestamp;
};

struct SharedResources
{
    std::atomic<bool> done{false};
    std::atomic<bool> paused{false};
    std::atomic<int> currentFrameIndex{-1};
    std::atomic<bool> displayNeedsUpdate{false};
    std::atomic<size_t> latestCameraFrame{0};
    std::atomic<size_t> frameRateCount{0};
    std::queue<size_t> framesToProcess;
    std::queue<size_t> framesToDisplay;
    std::mutex displayQueueMutex;
    std::mutex processingQueueMutex;
    std::condition_variable displayQueueCondition;
    std::condition_variable processingQueueCondition;
    std::vector<std::tuple<double, double>> circularities;
    std::mutex circularitiesMutex;
    std::atomic<bool> newScatterDataAvailable{false};
    std::condition_variable scatterDataCondition;
    cv::Mat backgroundFrame;
    cv::Mat blurredBackground;
    std::mutex backgroundFrameMutex;
    cv::Rect roi;
    std::mutex roiMutex;
    std::vector<QualifiedResult> qualifiedResults;
    std::mutex qualifiedResultsMutex;
    std::vector<QualifiedResult> qualifiedResultsBuffer1;
    std::vector<QualifiedResult> qualifiedResultsBuffer2;
    std::atomic<bool> usingBuffer1{true};
    std::condition_variable savingCondition;
    std::atomic<bool> savingInProgress{false};
    std::atomic<size_t> totalSavedResults{0};
    std::chrono::steady_clock::time_point lastSaveTime;
};

// Function declarations
ImageParams initializeImageParams(const std::string &directory);
void loadImages(const std::string &directory, CircularBuffer &cameraBuffer, bool reverseOrder = false);
void initializeMockBackgroundFrame(SharedResources &shared, const ImageParams &params, const CircularBuffer &cameraBuffer);
void processFrame(const std::vector<uint8_t> &imageData, size_t width, size_t height,
                  SharedResources &shared, cv::Mat &outputImage, bool isProcessingThread);
ContourResult findContours(const cv::Mat &processedImage);
std::tuple<double, double> calculateMetrics(const std::vector<cv::Point> &contour);
void simulateCameraThread(std::atomic<bool> &done, std::atomic<bool> &paused,
                          CircularBuffer &cameraBuffer, SharedResources &shared,
                          const ImageParams &params);
void processingThreadTask(std::atomic<bool> &done, std::atomic<bool> &paused,
                          std::mutex &processingQueueMutex,
                          std::condition_variable &processingQueueCondition,
                          std::queue<size_t> &framesToProcess,
                          const CircularBuffer &circularBuffer,
                          size_t width, size_t height, SharedResources &shared);
void onTrackbar(int pos, void *userdata);
void updateScatterPlot(cv::Mat &plot, const std::vector<std::tuple<double, double>> &circularities);
void displayThreadTask(const std::atomic<bool> &done, const std::atomic<bool> &paused,
                       const std::atomic<int> &currentFrameIndex,
                       std::atomic<bool> &displayNeedsUpdate,
                       std::queue<size_t> &framesToDisplay,
                       std::mutex &displayQueueMutex,
                       const CircularBuffer &circularBuffer,
                       size_t width, size_t height, size_t bufferCount,
                       SharedResources &shared);
void keyboardHandlingThread(std::atomic<bool> &done, std::atomic<bool> &paused,
                            std::atomic<int> &currentFrameIndex,
                            std::atomic<bool> &displayNeedsUpdate,
                            const CircularBuffer &circularBuffer,
                            size_t bufferCount, size_t width, size_t height,
                            SharedResources &shared);
void mockSample(const ImageParams &params, CircularBuffer &cameraBuffer,
                CircularBuffer &circularBuffer, SharedResources &shared);
void saveQualifiedResultsToDisk(const std::vector<QualifiedResult> &results, const std::string &directory);
void resultSavingThread(SharedResources &shared, const std::string &saveDirectory);
void convertSavedImagesToStandardFormat(const std::string &binaryImageFile, const std::string &outputDirectory);
int mock_grabber_main();
json readConfig(const std::string &filename);
bool updateConfig(const std::string &filename, const std::string &key, const json &value);
void temp_mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, SharedResources &shared);