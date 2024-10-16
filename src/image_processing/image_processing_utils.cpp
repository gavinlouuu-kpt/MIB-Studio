#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <future>
#include <vector>

ImageParams initializeImageParams(const std::string &directory)
{
    ImageParams params;
    params.bufferCount = 5000;

    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".tiff" || entry.path().extension() == ".tif")
        {
            cv::Mat image = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
            if (!image.empty())
            {
                params.width = image.cols;
                params.height = image.rows;
                params.pixelFormat = image.type();
                params.imageSize = image.total() * image.elemSize();
                return params;
            }
        }
    }

    throw std::runtime_error("No valid TIFF images found in the directory");
}

void loadImages(const std::string &directory, CircularBuffer &cameraBuffer, bool reverseOrder)
{
    std::vector<std::filesystem::path> imagePaths;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".tiff" || entry.path().extension() == ".tif")
        {
            imagePaths.push_back(entry.path());
        }
    }

    std::sort(imagePaths.begin(), imagePaths.end());

    if (reverseOrder)
    {
        std::reverse(imagePaths.begin(), imagePaths.end());
    }

    for (const auto &path : imagePaths)
    {
        cv::Mat image = cv::imread(path.string(), cv::IMREAD_GRAYSCALE);
        if (!image.empty())
        {
            cameraBuffer.push(image.data);
        }
    }

    std::cout << "Loaded " << cameraBuffer.size() << " images into camera buffer." << std::endl;
}

void initializeMockBackgroundFrame(SharedResources &shared, const ImageParams &params, const CircularBuffer &cameraBuffer)
{
    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);

    // Select an image from the middle of the buffer as the background
    size_t selectedIndex = 0;
    std::vector<uchar> imageData = cameraBuffer.get(selectedIndex);

    // Create a cv::Mat from the image data
    cv::Mat selectedImage(static_cast<int>(params.height), static_cast<int>(params.width), CV_8UC1, imageData.data());

    // Clone the selected image to create the background frame
    shared.backgroundFrame = selectedImage.clone();

    // Apply Gaussian blur to the background frame
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);

    std::cout << "Background frame initialized from loaded image at index: " << selectedIndex << std::endl;
}

void saveQualifiedResultsToDisk(const std::vector<QualifiedResult> &results, const std::string &directory, const SharedResources &shared)
{
    static int batchNumber = 0;
    std::string batchDir = directory + "/batch_" + std::to_string(batchNumber++);
    std::filesystem::create_directories(batchDir);

    std::string csvFilePath = batchDir + "/batch_data.csv";
    std::ofstream csvFile(csvFilePath);

    std::ofstream imageFile(batchDir + "/images.bin", std::ios::binary);

    // Write CSV header
    csvFile << "Timestamp_us,Circularity,Area\n";

    // Add this block to handle the ROI image
    if (!results.empty())
    {
        cv::Mat roiImage = results[0].originalImage.clone();
        cv::rectangle(roiImage, shared.roi, cv::Scalar(255, 255, 255), 2); // Draw white rectangle
        cv::imwrite(batchDir + "/roi_image.png", roiImage);
    }

    for (const auto &result : results)
    {
        // Write data to CSV
        csvFile << result.timestamp << ","
                << result.circularity << ","
                << result.area << "\n";

        // Save image metadata and data (unchanged)
        int rows = result.originalImage.rows;
        int cols = result.originalImage.cols;
        int type = result.originalImage.type();
        imageFile.write(reinterpret_cast<const char *>(&rows), sizeof(int));
        imageFile.write(reinterpret_cast<const char *>(&cols), sizeof(int));
        imageFile.write(reinterpret_cast<const char *>(&type), sizeof(int));

        if (result.originalImage.isContinuous())
        {
            imageFile.write(reinterpret_cast<const char *>(result.originalImage.data), result.originalImage.total() * result.originalImage.elemSize());
        }
        else
        {
            for (int r = 0; r < rows; ++r)
            {
                imageFile.write(reinterpret_cast<const char *>(result.originalImage.ptr(r)), cols * result.originalImage.elemSize());
            }
        }
    }
    csvFile.close();
    imageFile.close();

    // std::cout << "Saved " << results.size() << " results to " << batchDir << std::endl;
}

void convertSavedImagesToStandardFormat(const std::string &binaryImageFile, const std::string &outputDirectory)
{
    std::ifstream imageFile(binaryImageFile, std::ios::binary);
    std::filesystem::create_directories(outputDirectory);

    int imageCount = 0;
    while (imageFile.good())
    {
        int rows, cols, type;
        imageFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
        imageFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
        imageFile.read(reinterpret_cast<char *>(&type), sizeof(int));

        if (imageFile.eof())
            break;

        cv::Mat image(rows, cols, type);
        imageFile.read(reinterpret_cast<char *>(image.data), rows * cols * image.elemSize());

        std::string outputPath = outputDirectory + "/image_" + std::to_string(imageCount++) + ".png";
        cv::imwrite(outputPath, image);
    }

    std::cout << "Converted " << imageCount << " images to PNG format in " << outputDirectory << std::endl;
}

json readConfig(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open config file: " + filename);
    }
    json config;
    file >> config;
    return config;
}

bool updateConfig(const std::string &filename, const std::string &key, const json &value)
{
    try
    {
        // Read existing config
        std::ifstream input_file(filename);
        if (!input_file.is_open())
        {
            std::cerr << "Unable to open config file: " << filename << std::endl;
            return false;
        }
        json config;
        input_file >> config;
        input_file.close();

        // Update the value
        config[key] = value;

        // Write updated config back to file
        std::ofstream output_file(filename);
        if (!output_file.is_open())
        {
            std::cerr << "Unable to open config file for writing: " << filename << std::endl;
            return false;
        }
        output_file << std::setw(4) << config << std::endl;
        output_file.close();

        std::cout << "Config updated successfully. Key: " << key << ", New value: " << value << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating config: " << e.what() << std::endl;
        return false;
    }
}
