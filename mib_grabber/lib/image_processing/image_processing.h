#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>

#define M_PI 3.14159265358979323846 // pi

class CircularBuffer;

struct ImageParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
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
};

struct ContourResult
{
    std::vector<std::vector<cv::Point>> contours;
    double findTime;
};

// Function declarations
ImageParams initializeImageParams(const std::string &directory);
void loadImages(const std::string &directory, CircularBuffer &cameraBuffer, bool reverseOrder = false);
void initializeMockBackgroundFrame(SharedResources &shared, const ImageParams &params);
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