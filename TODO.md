# TODO
## 2024/10/24
### Target: Get real time cell contour
- Duplicate frame detection
- Calculate DI (gating from noise, debris, shake*)
- Easy to tune threshold params 
- Background acquisition uncouple from pause
- XY offset to align field of view from 1920 x 1080 to 512 x 96 (EGrabber script to offset)
- Faster image conversion for review
- larger saving batch (saving speed is not the bottleneck)
- Do not auto-save with deformabilities
- change scatter plot to qt


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