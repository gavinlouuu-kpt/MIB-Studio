#pragma once

#include <EGrabber.h>
#include <boost/circular_buffer.hpp>
#include "image_processing/image_processing.h"

// void configure(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber);
void configure_js(std::string config_path);
ImageParams initializeGrabber(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber);
void initializeBackgroundFrame(SharedResources &shared, const ImageParams &params);
void temp_sample(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber, const ImageParams &params, boost::circular_buffer<std::vector<uint8_t>> &circularBuffer, boost::circular_buffer<std::vector<uint8_t>> &processingBuffer, SharedResources &shared);
void hybrid_sample(Euresys::EGrabber<Euresys::CallbackOnDemand> &grabber, const ImageParams &params, boost::circular_buffer<std::vector<uint8_t>> &cameraBuffer, boost::circular_buffer<std::vector<uint8_t>> &circularBuffer, boost::circular_buffer<std::vector<uint8_t>> &processingBuffer, SharedResources &shared);
void runHybridSample();
int mib_grabber_main();
