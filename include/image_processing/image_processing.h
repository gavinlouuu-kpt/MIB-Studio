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

// New structure for tracking object trajectories
struct ObjectTrack {
    int id;                                 // Unique identifier for the object
    std::vector<cv::Point> positions;       // List of positions (centroids) across frames
    std::vector<int> frameIndices;          // Corresponding frame indices for each position
    std::vector<cv::Rect> boundingBoxes;    // Bounding boxes across frames
    std::vector<double> areas;              // Areas across frames
    
    // Constructor with initial position
    ObjectTrack(int objectId, const cv::Point& position, int frameIndex, 
                const cv::Rect& bbox, double area) 
        : id(objectId) {
        positions.push_back(position);
        frameIndices.push_back(frameIndex);
        boundingBoxes.push_back(bbox);
        areas.push_back(area);
    }
    
    // Predict next position based on constant velocity assumption
    cv::Point predictNextPosition() const {
        if (positions.size() < 2) {
            return positions.empty() ? cv::Point(0, 0) : positions.back();
        }
        
        // For better prediction, use the actual frame indices to calculate velocity
        int lastIdx = positions.size() - 1;
        
        // Calculate frame interval
        int frameInterval = frameIndices[lastIdx] - frameIndices[lastIdx - 1];
        if (frameInterval <= 0) frameInterval = 1; // Ensure positive interval
        
        // Calculate velocity based on the last two points
        cv::Point velocity(
            (positions[lastIdx].x - positions[lastIdx - 1].x) / frameInterval, 
            (positions[lastIdx].y - positions[lastIdx - 1].y) / frameInterval
        );
        
        // Scale velocity by the expected frame interval (assuming next frame = last frame + 1)
        int expectedFrameInterval = 1;
        
        // Return prediction based on the last position plus scaled velocity
        return cv::Point(
            positions[lastIdx].x + velocity.x * expectedFrameInterval,
            positions[lastIdx].y + velocity.y * expectedFrameInterval
        );
    }
    
    // Get average velocity (pixels per frame)
    cv::Point getAverageVelocity() const {
        if (positions.size() < 2) {
            return cv::Point(0, 0);
        }
        
        // Use first and last positions to get total displacement
        cv::Point totalDisplacement = positions.back() - positions.front();
        
        // Calculate frames elapsed
        int framesElapsed = frameIndices.back() - frameIndices.front();
        if (framesElapsed == 0) return cv::Point(0, 0);
        
        // Return average velocity
        return cv::Point(
            totalDisplacement.x / framesElapsed,
            totalDisplacement.y / framesElapsed
        );
    }
};

struct TrajectoryData {
    std::vector<ObjectTrack> tracks;         // All object tracks
    int lastFrameIndex = -1;                 // Last processed frame index
    int nextObjectId = 0;                    // Counter for generating unique object IDs
    std::mutex mutex;                        // Mutex for thread safety
    std::vector<std::string> debugMessages;  // Store debug messages about trajectory processing
    
    // Configuration parameters
    double maxMatchingDistance = 250.0;     // Maximum distance for matching objects between frames
    double minMovementThreshold = 100.0;     // Minimum movement required to consider a valid match
    
    // Reset all tracking data
    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        tracks.clear();
        lastFrameIndex = -1;  // Make sure this is reset to -1
        nextObjectId = 0;
        debugMessages.clear();
        std::cout << "TRAJECTORY: Tracking reset, lastFrameIndex = " << lastFrameIndex << std::endl;
    }
    
    // Get last debug message
    std::string getLastDebugMessage() {
        std::lock_guard<std::mutex> lock(mutex);
        if (debugMessages.empty()) {
            return "No debug information available";
        }
        return debugMessages.back();
    }
    
    // Add a debug message
    void addDebugMessage(const std::string& message) {
        // Log to console instead of storing in memory
        std::cout << "TRAJECTORY: " << message << std::endl;
        
        // Still store a few recent messages for on-screen display
        std::lock_guard<std::mutex> lock(mutex);
        debugMessages.push_back(message);
        // Keep only the last 5 messages
        if (debugMessages.size() > 5) {
            debugMessages.erase(debugMessages.begin());
        }
    }
    
    // Add a new position to an existing track or create a new track
    void updateTrack(int frameIndex, 
                    const std::vector<std::vector<cv::Point>>& contours,
                    const std::vector<std::vector<cv::Point>>& innerContours,
                    const std::vector<int>& parentIndices) {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Debug the current state before processing
        std::cout << "TRAJECTORY: updateTrack called with frameIndex = " << frameIndex 
                  << ", lastFrameIndex = " << lastFrameIndex << std::endl;
        
        // Skip if we're processing the same frame again
        if (frameIndex == lastFrameIndex) {
            std::cout << "TRAJECTORY: Frame " << frameIndex << " - Skipped (same as last processed frame)" << std::endl;
            return;
        }
        
        // If lastFrameIndex is invalid or unreasonably large, reset it
        if (lastFrameIndex < 0 || lastFrameIndex > 5000) {
            std::cout << "TRAJECTORY: Reset lastFrameIndex from " << lastFrameIndex << " to -1 (invalid value detected)" << std::endl;
            lastFrameIndex = -1;
        }
        
        // Now check if we're going backward (but only if lastFrameIndex is valid)
        if (lastFrameIndex > 0 && frameIndex < lastFrameIndex) {
            std::cout << "TRAJECTORY: Frame " << frameIndex << " - Skipped (going backward from " 
                      << lastFrameIndex << ")" << std::endl;
            return;
        }
        
        // Calculate centroids and bounding boxes for all valid contours
        std::vector<cv::Point> centroids;
        std::vector<cv::Rect> boundingBoxes;
        std::vector<double> areas;
        
        for (size_t i = 0; i < contours.size(); i++) {
            // Check if this is a parent contour with an inner contour
            bool hasInnerContour = false;
            for (size_t j = 0; j < parentIndices.size(); j++) {
                if (static_cast<int>(i) == parentIndices[j]) {
                    hasInnerContour = true;
                    break;
                }
            }
            
            if (hasInnerContour) {
                // Calculate moments to get centroid
                cv::Moments m = cv::moments(contours[i]);
                if (m.m00 > 0) {
                    cv::Point centroid(m.m10 / m.m00, m.m01 / m.m00);
                    centroids.push_back(centroid);
                    
                    // Calculate bounding box
                    cv::Rect bbox = cv::boundingRect(contours[i]);
                    boundingBoxes.push_back(bbox);
                    
                    // Calculate area
                    double area = cv::contourArea(contours[i]);
                    areas.push_back(area);
                }
            }
        }
        
        // Record number of detected objects
        std::cout << "TRAJECTORY: Frame " << frameIndex << " - Detected " 
                  << centroids.size() << " objects" << std::endl;
        
        // If this is the first frame or we reset
        if (tracks.empty()) {
            // Create new tracks for each detected object
            for (size_t i = 0; i < centroids.size(); i++) {
                tracks.emplace_back(nextObjectId++, centroids[i], frameIndex, 
                                   boundingBoxes[i], areas[i]);
            }
            std::cout << "TRAJECTORY: Created " << centroids.size() << " new tracks (first frame)" << std::endl;
        } else {
            // Match existing tracks with new detections
            std::vector<bool> assignedDetections(centroids.size(), false);
            int matchCount = 0;
            
            // For each existing track
            for (auto& track : tracks) {
                // Skip tracks that haven't been updated recently
                if (track.frameIndices.back() < lastFrameIndex - 5) {
                    continue;
                }
                
                // Predict where this track should be
                cv::Point predictedPos = track.predictNextPosition();
                
                // Find the closest detection
                int bestMatch = -1;
                double minDistance = maxMatchingDistance; // Use class variable instead of hard-coded value
                
                for (size_t i = 0; i < centroids.size(); i++) {
                    if (!assignedDetections[i]) {
                        double distance = cv::norm(predictedPos - centroids[i]);
                        
                        // Calculate how far the object has moved from its last position
                        double movementDistance = 0.0;
                        if (!track.positions.empty()) {
                            movementDistance = cv::norm(track.positions.back() - centroids[i]);
                        }
                        
                        std::cout << "TRAJECTORY: Matching - Track " << track.id 
                                  << " (last at " << track.positions.back().x << "," << track.positions.back().y 
                                  << ", predicted at " << predictedPos.x << "," << predictedPos.y
                                  << ") to object at (" << centroids[i].x << "," << centroids[i].y 
                                  << ") distance = " << distance 
                                  << ", movement = " << movementDistance << std::endl;
                        
                        // Only consider matches if object has moved enough
                        if (movementDistance >= minMovementThreshold) {
                            if (distance < minDistance) {
                                minDistance = distance;
                                bestMatch = i;
                            }
                        } else {
                            std::cout << "TRAJECTORY: Discarding match - movement " << movementDistance 
                                      << " is below threshold " << minMovementThreshold << std::endl;
                        }
                    }
                }
                
                // If we found a match
                if (bestMatch >= 0) {
                    // Update the track
                    track.positions.push_back(centroids[bestMatch]);
                    track.frameIndices.push_back(frameIndex);
                    track.boundingBoxes.push_back(boundingBoxes[bestMatch]);
                    track.areas.push_back(areas[bestMatch]);
                    assignedDetections[bestMatch] = true;
                    matchCount++;
                    std::cout << "TRAJECTORY: SUCCESS - Matched track " << track.id 
                              << " to object at (" << centroids[bestMatch].x << "," << centroids[bestMatch].y 
                              << ") with distance " << minDistance << std::endl;
                } else {
                    std::cout << "TRAJECTORY: FAILED - No match found for track " << track.id 
                              << " (predicted at " << predictedPos.x << "," << predictedPos.y
                              << ")" << std::endl;
                }
            }
            
            // Count new tracks created
            int newTrackCount = 0;
            
            // Create new tracks for unassigned detections
            for (size_t i = 0; i < centroids.size(); i++) {
                if (!assignedDetections[i]) {
                    tracks.emplace_back(nextObjectId++, centroids[i], frameIndex, 
                                       boundingBoxes[i], areas[i]);
                    newTrackCount++;
                }
            }
            
            std::cout << "TRAJECTORY: Frame " << frameIndex << " - Matched " << matchCount 
                      << " existing tracks, created " << newTrackCount << " new tracks" << std::endl;
        }
        
        // Update the last frame index with the current frame
        std::cout << "TRAJECTORY: Updated lastFrameIndex from " << lastFrameIndex << " to " << frameIndex << std::endl;
        lastFrameIndex = frameIndex;
    }
};

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
    double ringRatio; // Ratio of inner contour area to outer contour area

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
    double ringRatio; // Ratio of inner contour area to outer contour area
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
    std::atomic<size_t> recordedItemsCount{0}; // Counter for items recorded during 'running' state
    std::atomic<bool> clearHistogramData{false}; // Flag to clear histogram data
    std::atomic<double> averageRingRatio{0.0}; // Average ring ratio for dashboard display
    
    // Trajectory tracking data
    TrajectoryData trajectoryData;
    std::atomic<bool> showTrajectories{true}; // Enable/disable trajectory visualization

    // Thread shutdown tracking
    std::atomic<int> activeThreadCount{0};
    std::atomic<int> threadsReadyToJoin{0};
    std::mutex threadShutdownMutex;
    std::condition_variable threadShutdownCondition;

    // Valid frames sharing between processing thread and display thread
    struct ValidFrameData {
        cv::Mat originalImage;
        cv::Mat processedImage;
        FilterResult result;
        size_t frameIndex;
        int64_t timestamp;
    };
    std::deque<ValidFrameData> validFramesQueue;
    std::mutex validFramesMutex;
    std::condition_variable validFramesCondition;
    std::atomic<bool> newValidFrameAvailable{false};

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
    std::string backgroundCaptureTime;     // Timestamp when background was captured
    std::mutex backgroundCaptureTimeMutex; // Mutex for the background capture time
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
    std::atomic<double> frameRingRatios;
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
    // std::atomic<bool> triggerOut{false};
    std::atomic<bool> processTrigger{false};
};

// Function declarations
ImageParams initializeImageParams(const std::string &directory);
void loadImages(const std::string &directory, CircularBuffer &cameraBuffer, bool reverseOrder = false);
void initializeMockBackgroundFrame(SharedResources &shared, const ImageParams &params, const CircularBuffer &cameraBuffer);
void processFrame(const cv::Mat &inputImage, SharedResources &shared,
                  cv::Mat &outputImage, ThreadLocalMats &mats);
std::tuple<std::vector<std::vector<cv::Point>>, bool, std::vector<std::vector<cv::Point>>, std::vector<int>> findContours(const cv::Mat &processedImage);
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

void validFramesDisplayThread(SharedResources &shared, const CircularBuffer &circularBuffer, const ImageParams &imageParams);

ThreadLocalMats initializeThreadMats(int height, int width, SharedResources &shared);

void reviewSavedData();

FilterResult filterProcessedImage(const cv::Mat &processedImage, const cv::Rect &roi,
                                  const ProcessingConfig &config, const uint8_t processedColor = 255);

// Function to determine overlay color based on FilterResult
cv::Scalar determineOverlayColor(const FilterResult &result, bool isValid);

void createDefaultConfigIfMissing(const std::filesystem::path &configPath);
std::string selectSaveDirectory(const std::string &configPath);
