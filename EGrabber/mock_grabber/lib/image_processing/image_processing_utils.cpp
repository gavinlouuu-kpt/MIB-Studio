#include "image_processing.h"
#include "CircularBuffer.h"
#include <filesystem>
#include <iostream>

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

void initializeBackgroundFrame(SharedResources &shared, const ImageParams &params)
{
    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
    shared.backgroundFrame = cv::Mat(static_cast<int>(params.height), static_cast<int>(params.width), CV_8UC1, cv::Scalar(255));
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
}