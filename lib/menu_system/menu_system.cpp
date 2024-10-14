#include <iostream>
#include <string>
#include <limits>
#include "image_processing.h"
#include "CircularBuffer.h"
#include "menu_system.h"

namespace MenuSystem
{

    void clearInputBuffer()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void displayMenu()
    {
        std::cout << "\n=== Cell Analysis Menu ===\n";
        std::cout << "1. Run Mock Sample\n";
        std::cout << "2. Run Live Sample\n";
        std::cout << "3. Convert Saved Images\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter your choice: ";
    }

    void runMockSample()
    {
        std::string imageDirectory;
        std::cout << "Enter the image directory path: ";
        std::getline(std::cin, imageDirectory);

        try
        {
            ImageParams params = initializeImageParams(imageDirectory);
            CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);
            CircularBuffer circularBuffer(params.bufferCount, params.imageSize);

            loadImages(imageDirectory, cameraBuffer, true);

            SharedResources shared;
            initializeMockBackgroundFrame(shared, params, cameraBuffer);
            shared.roi = cv::Rect(0, 0, params.width, params.height);

            mockSample(params, cameraBuffer, circularBuffer, shared);

            std::cout << "Mock sampling completed.\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void runLiveSample()
    {
        // Placeholder for live sampling implementation
        std::cout << "Live sampling not implemented yet.\n";
    }

    void convertSavedImages()
    {
        std::string binaryImageFile, outputDirectory;

        std::cout << "Enter the path to the binary image file: ";
        std::getline(std::cin, binaryImageFile);

        std::cout << "Enter the output directory for converted images: ";
        std::getline(std::cin, outputDirectory);

        try
        {
            convertSavedImagesToStandardFormat(binaryImageFile, outputDirectory);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    int runMenu()
    {
        int choice;
        do
        {
            displayMenu();
            std::cin >> choice;
            clearInputBuffer();

            switch (choice)
            {
            case 1:
                runMockSample();
                break;
            case 2:
                runLiveSample();
                break;
            case 3:
                convertSavedImages();
                break;
            case 4:
                std::cout << "Exiting program.\n";
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
            }
        } while (true);
    }

} // namespace MenuSystem
