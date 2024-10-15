#pragma once

#include <iostream>
#include <filesystem>
#include <EGrabber.h>
#include <opencv2/opencv.hpp>
#include <FormatConverter.h>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <conio.h>
#include <chrono>
#include <iomanip>
#include <CircularBuffer/CircularBuffer.h>
#include <tuple>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <image_processing/image_processing.h>
#include <menu_system/menu_system.h>
#include <nlohmann/json.hpp>

#define M_PI 3.14159265358979323846 // pi

using namespace Euresys;

struct GrabberParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

void configure(EGrabber<CallbackOnDemand> &grabber);
void configure_js();
GrabberParams initializeGrabber(EGrabber<CallbackOnDemand> &grabber);
void initializeBackgroundFrame(SharedResources &shared, const GrabberParams &params);
void sample(EGrabber<CallbackOnDemand> &grabber, const GrabberParams &params, CircularBuffer &circularBuffer, SharedResources &shared);