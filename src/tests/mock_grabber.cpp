#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <string>
#include <filesystem>

int main()
{
    try
    {
        bool loadInReverse = true;
        std::string imageDirectory;

        // Prompt user for image directory
        std::cout << "Enter the image directory path: ";
        std::getline(std::cin, imageDirectory);

        // Validate the directory
        if (!std::filesystem::exists(imageDirectory) || !std::filesystem::is_directory(imageDirectory))
        {
            throw std::runtime_error("Invalid directory path");
        }

        ImageParams params = initializeImageParams(imageDirectory);
        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);

        loadImages(imageDirectory, cameraBuffer, loadInReverse);

        SharedResources shared;
        initializeMockBackgroundFrame(shared, params);

        mockSample(params, cameraBuffer, circularBuffer, shared);
    }
    catch (const std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}