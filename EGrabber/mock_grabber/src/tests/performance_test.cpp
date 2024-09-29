#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <chrono>

int main(int argc, char *argv[])
{
    /*
    Run the code with an arg:
    ./build/debug/performance_test.exe "C:\Users\gavin\code\test_image"
    */
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image_directory>" << std::endl;
        return 1;
    }

    std::string directory = argv[1];

    try
    {
        // Initialize image parameters
        ImageParams params = initializeImageParams(directory);

        // Create circular buffers
        CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);
        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);

        // Load images into the camera buffer
        loadImages(directory, cameraBuffer, false);

        // Initialize shared resources
        SharedResources shared;
        initializeBackgroundFrame(shared, params);

        // Process each image and measure time
        cv::Mat processedImage(params.height, params.width, CV_8UC1);
        double totalProcessingTime = 0.0;
        int processedFrames = 0;

        for (size_t i = 0; i < cameraBuffer.size(); ++i)
        {
            auto imageData = cameraBuffer.get(i);

            auto start = std::chrono::high_resolution_clock::now();

            // Process the image
            processFrame(imageData, params.width, params.height, shared, processedImage, true);

            // Find contours
            ContourResult contourResult = findContours(processedImage);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            totalProcessingTime += duration.count();
            processedFrames++;

            // Optional: Print progress
            if (processedFrames % 100 == 0)
            {
                std::cout << "Processed " << processedFrames << " frames..." << std::endl;
            }
        }

        // Calculate and print average processing time
        double averageProcessingTime = totalProcessingTime / processedFrames;
        std::cout << "Average processing time per frame: " << averageProcessingTime << " microseconds" << std::endl;
        std::cout << "Frames processed per second: " << 1000000.0 / averageProcessingTime << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}