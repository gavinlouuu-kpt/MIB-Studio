# TODO
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