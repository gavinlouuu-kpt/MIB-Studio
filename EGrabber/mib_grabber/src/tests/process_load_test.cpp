#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>   // Add this line for std::accumulate
#include <algorithm> // Add this line for std::minmax_element
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <cmath>

struct ImageParams
{
    size_t width;
    size_t height;
    size_t imageSize;
};

ImageParams getImageParams(const cv::Mat &image)
{
    ImageParams params;
    params.width = image.cols;
    params.height = image.rows;
    params.imageSize = image.total() * image.elemSize();
    return params;
}

static cv::Mat background, blurred_bg;
static cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

void processFrame(const cv::Mat &original, cv::Mat &output)
{
    cv::Mat blurred, bg_sub, binary, dilate1, erode1, erode2, dilate2, edges;

    if (background.empty())
    {
        background = cv::Mat(original.size(), CV_8UC1, cv::Scalar(255));
        cv::GaussianBlur(background, blurred_bg, cv::Size(3, 3), 0);
    }

    cv::GaussianBlur(original, blurred, cv::Size(3, 3), 0);
    cv::subtract(blurred_bg, blurred, bg_sub);
    cv::threshold(bg_sub, binary, 10, 255, cv::THRESH_BINARY);

    cv::dilate(binary, dilate1, kernel, cv::Point(-1, -1), 2);
    cv::erode(dilate1, erode1, kernel, cv::Point(-1, -1), 2);
    cv::erode(erode1, erode2, kernel, cv::Point(-1, -1), 1);
    cv::dilate(erode2, dilate2, kernel, cv::Point(-1, -1), 1);

    cv::Canny(erode2, edges, 50, 150);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    output = edges.clone();
}

void processImagesFromFolder(const std::string &folderPath)
{
    std::vector<long long> processingTimes;
    std::vector<std::string> imagePaths;

    // Collect image paths
    for (const auto &entry : std::filesystem::directory_iterator(folderPath))
    {
        if (entry.path().extension() == ".tiff" || entry.path().extension() == ".jpg")
        {
            imagePaths.push_back(entry.path().string());
        }
    }

    const int numImages = imagePaths.size();
    processingTimes.reserve(numImages);

    // Set background image (assuming the first image is the background)
    if (!imagePaths.empty())
    {
        background = cv::imread(imagePaths[0], cv::IMREAD_GRAYSCALE);
        cv::GaussianBlur(background, blurred_bg, cv::Size(3, 3), 0);
    }

    for (size_t i = 1; i < imagePaths.size(); ++i)
    {
        cv::Mat original = cv::imread(imagePaths[i], cv::IMREAD_GRAYSCALE);
        if (original.empty())
        {
            std::cerr << "Failed to load image: " << imagePaths[i] << std::endl;
            continue;
        }

        ImageParams params = getImageParams(original);
        cv::Mat processed(params.height, params.width, CV_8UC1);

        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();
        processFrame(original, processed);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        processingTimes.push_back(duration.count());
    }

    if (processingTimes.empty())
    {
        std::cout << "No images were processed." << std::endl;
        return;
    }

    // Calculate basic statistics
    double sum = std::accumulate(processingTimes.begin(), processingTimes.end(), 0.0);
    double mean = sum / processingTimes.size();

    auto [min_it, max_it] = std::minmax_element(processingTimes.begin(), processingTimes.end());
    long long min = *min_it;
    long long max = *max_it;

    // Print results
    std::cout << "Processed " << numImages << " images" << std::endl;
    std::cout << "Processing time statistics:" << std::endl;
    std::cout << "Min: " << min << " us" << std::endl;
    std::cout << "Max: " << max << " us" << std::endl;
    std::cout << "Mean: " << mean << " us" << std::endl;
}

int main()
{
    try
    {
        std::string folderPath = "D:/CMake_egrabber/VS_EGrabber/EGrabber/test_images_cgy/Slight under focus_512x96";
        processImagesFromFolder(folderPath);
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
