#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <conio.h>
#include <chrono>
#include <iomanip>
#include "CircularBuffer.h"
#include <tuple>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "image_processing.h"

int main()
{
    try
    {

        bool loadInReverse = true;
        const std::string imageDirectory = "C:\\Users\\gavin\\code\\test_image";

        ImageParams params = initializeImageParams(imageDirectory);
        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);

        loadImages(imageDirectory, cameraBuffer, loadInReverse);

        SharedResources shared;
        initializeBackgroundFrame(shared, params);

        sample(params, cameraBuffer, circularBuffer, shared);
    }
    catch (const std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}