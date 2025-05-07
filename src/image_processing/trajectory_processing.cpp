#include "image_processing/image_processing.h"
#include <iostream>
#include <limits>

// ObjectTrack method implementations
cv::Point ObjectTrack::predictNextPosition() const {
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

bool ObjectTrack::willLeaveFrame(const cv::Size& frameSize) {
    if (positions.empty()) return false;
    
    // Get the predicted next position
    cv::Point predictedPos = predictNextPosition();
    
    // Only check if the object will cross the right edge with a 50-pixel margin
    bool willLeave = (predictedPos.x >= frameSize.width - 50);
    
    // Update the flag
    predictedToLeaveFrame = willLeave;
    
    return willLeave;
}

cv::Point ObjectTrack::getAverageVelocity() const {
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

// TrajectoryData method implementations
void TrajectoryData::checkForObjectsLeavingFrame(const cv::Size& frameSize, 
                                               const std::vector<std::vector<cv::Point>>& allContours,
                                               int currentFrameIndex) {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& track : tracks) {
        // Skip objects that already have a contour saved
        if (!track.contour.empty()) {
            continue;
        }
        
        // Check if this object is predicted to leave frame (right edge only)
        if (track.willLeaveFrame(frameSize)) {
            std::cout << "TRAJECTORY: Track " << track.id << " approaching right edge of frame" << std::endl;
            
            // Try to find and store the contour for this track
            if (!track.positions.empty() && !allContours.empty()) {
                cv::Point currentPos = track.positions.back();
                double minDist = std::numeric_limits<double>::max();
                int closestContourIdx = -1;
                
                // Find closest contour
                for (size_t i = 0; i < allContours.size(); i++) {
                    if (allContours[i].empty()) continue;
                    
                    // Calculate center of contour's bounding box 
                    cv::Rect bbox = cv::boundingRect(allContours[i]);
                    cv::Point contourCenter(bbox.x + bbox.width/2, bbox.y + bbox.height/2);
                    double dist = cv::norm(contourCenter - currentPos);
                    
                    if (dist < minDist) {
                        minDist = dist;
                        closestContourIdx = i;
                    }
                }
                
                // If we found a contour close enough
                if (closestContourIdx >= 0 && minDist < 50.0) { // Within 50 pixel threshold
                    track.contour = allContours[closestContourIdx];
                    std::cout << "TRAJECTORY: Captured contour for track " << track.id 
                              << " at frame " << currentFrameIndex << std::endl;
                }
            }
        }
    }
}

void TrajectoryData::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    tracks.clear();
    lastFrameIndex = -1;  // Make sure this is reset to -1
    nextObjectId = 0;
    debugMessages.clear();
    lateDetections.clear(); // Clear late detections
    std::cout << "TRAJECTORY: Tracking reset, lastFrameIndex = " << lastFrameIndex << std::endl;
}

std::string TrajectoryData::getLastDebugMessage() {
    std::lock_guard<std::mutex> lock(mutex);
    if (debugMessages.empty()) {
        return "No debug information available";
    }
    return debugMessages.back();
}

void TrajectoryData::addDebugMessage(const std::string& message) {
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

void TrajectoryData::updateTrack(int frameIndex, 
                               const std::vector<std::vector<cv::Point>>& contours,
                               const std::vector<std::vector<cv::Point>>& innerContours,
                               const std::vector<int>& parentIndices,
                               const cv::Size& frameSize) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Clear late detections from previous frames
    lateDetections.clear();
    
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
            // Calculate bounding box first
            cv::Rect bbox = cv::boundingRect(contours[i]);
            boundingBoxes.push_back(bbox);
            
            // Use center of bounding box as centroid
            cv::Point centroid(bbox.x + bbox.width/2, bbox.y + bbox.height/2);
            centroids.push_back(centroid);
            
            // Calculate area
            double area = cv::contourArea(contours[i]);
            areas.push_back(area);
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
                // Check if the new detection is beyond 50% of x axis
                // This indicates a target that should have been recognized earlier
                if (centroids[i].x > frameSize.width / 2) {
                    std::cout << "TRAJECTORY: WARNING - Potential late detection at (" 
                             << centroids[i].x << "," << centroids[i].y 
                             << ") beyond 50% of x-axis (width: " << frameSize.width
                             << "). Not creating new track." << std::endl;
                    
                    // Store the late detection for visualization
                    lateDetections.push_back(centroids[i]);
                    
                    // Skip creating a new track for this detection
                    continue;
                }
                
                // Only create tracks for detections in the left half of the frame
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