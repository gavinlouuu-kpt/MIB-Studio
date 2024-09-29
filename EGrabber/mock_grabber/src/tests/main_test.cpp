#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <string>

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