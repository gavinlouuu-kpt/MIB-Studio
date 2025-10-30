#pragma once

#include <EGrabber.h>
#include "CircularBuffer/CircularBuffer.h"
#include "image_processing/image_processing.h"

struct GrabberParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

// void configure(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber);
void configure_js(std::string config_path);
GrabberParams initializeGrabber(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber);
void initializeBackgroundFrame(SharedResources &shared, const ImageParams &params);
void temp_sample(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber, const ImageParams &params, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared);
void hybrid_sample(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber, const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared);
void runHybridSample();
int mib_grabber_main();
int selectCamera();


