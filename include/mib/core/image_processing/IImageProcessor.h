#pragma once

#include <opencv2/core.hpp>
#include "image_processing/image_processing.h"

// Temporary interface to abstract processing while refactoring.
// Will be implemented by an adapter around existing free functions.
struct IImageProcessor {
    virtual void processFrame(const cv::Mat& inputImage,
                              SharedResources& shared,
                              cv::Mat& outputImage,
                              ThreadLocalMats& mats) = 0;
    virtual ~IImageProcessor() = default;
};


