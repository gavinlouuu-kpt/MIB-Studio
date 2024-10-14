#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <string>
#include <filesystem>
#include "menu_system.h"

// int mock_grabber_main()
// {
//     try
//     {
//         bool loadInReverse = true;
//         std::string imageDirectory;

//         // Prompt user for image directory
//         std::cout << "Enter the image directory path: ";
//         std::getline(std::cin, imageDirectory);

//         // Validate the directory
//         if (!std::filesystem::exists(imageDirectory) || !std::filesystem::is_directory(imageDirectory))
//         {
//             throw std::runtime_error("Invalid directory path");
//         }

//         ImageParams params = initializeImageParams(imageDirectory);
//         CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
//         CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);

//         loadImages(imageDirectory, cameraBuffer, loadInReverse);

//         SharedResources shared;
//         const std::string saveDirectory = "qualified_results";
//         initializeMockBackgroundFrame(shared, params, cameraBuffer);
//         shared.roi = cv::Rect(0, 0, params.width, params.height);

//         mockSample(params, cameraBuffer, circularBuffer, shared);
//     }
//     catch (const std::exception &e)
//     {
//         std::cout << "error: " << e.what() << std::endl;
//     }
//     return 0;
// }

int main()
{
    try
    {
        return MenuSystem::runMenu();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}