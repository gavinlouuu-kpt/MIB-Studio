# TODO
## 2024/10/31
- Duplicate frame detection works, repeated saving is because of more than one contour found
- Param hot reload completed with json in display mode. Need to leave and re enter to see the effect

## 2024/10/24
### Target: Get real time cell contour
- Duplicate frame detection
- Calculate DI (gating from noise, debris, shake*)
- (can tune from json) Easy to tune threshold params 
- (ok - use 'b' when paused) Background acquisition uncouple from pause
- XY offset to align field of view from 1920 x 1080 to 512 x 96 (EGrabber script to offset)
- (ok for now - direct read from bin, need to integrate back to current functions) Faster image conversion for review
- (not essential) larger saving batch (saving speed is not the bottleneck)
- (ok - use 'r' to start saving) Do not auto-save with deformabilities
- change scatter plot to qt
- create a setup mode




## 2024/10/16
- Show keyboard instructions
- Metrics to monitor
- Processing time
  - average
  - max
  - min
- Current Camera FPS
- Number of images in queue
- Paused
  - Current Frame Index
  - show keyboard instructions under paused condition 
- ROI
  - Image size
  - Size (x and y)
  - Position
- Circularities vector size 
  - min
  - max
  - mean
- Saving time
-       std::cout << "Saved " << bufferToSave.size() << " results to disk. "
                      << "Total saved: " << shared.totalSavedResults
                      << ". Time taken: " << duration.count() << " ms" << std::endl;
## 2024/10/14
### Features:
- Save cell image, circularities
  - Scatter plot is real time (not full extent of the data record)
- Save entire experiment ~20GB of data ~100,000 images
