#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <numeric>
// #include "matplotlibcpp.h"

namespace fs = std::filesystem;
// namespace plt = matplotlibcpp;

// Global variables for image processing
cv::Mat image, background, blurred_bg, blurred, bg_sub, binary, dilate1, erode1, erode2, dilate2, edges;
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
std::vector<std::vector<cv::Point>> contours;

std::pair<double, double> process_image(const std::string &image_path, const std::string &background_path)
{
    image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
    background = cv::imread(background_path, cv::IMREAD_GRAYSCALE);
    cv::GaussianBlur(background, blurred_bg, cv::Size(3, 3), 0);

    cv::TickMeter tm1;
    tm1.start();

    cv::GaussianBlur(image, blurred, cv::Size(3, 3), 0);

    if (blurred.size() != blurred_bg.size())
    {
        std::cerr << "Error: Image sizes do not match!" << std::endl;
        return {0.0, 0.0};
    }

    cv::subtract(blurred_bg, blurred, bg_sub);

    cv::threshold(bg_sub, binary, 10, 255, cv::THRESH_BINARY);

    cv::dilate(binary, dilate1, kernel, cv::Point(-1, -1), 2);
    cv::erode(dilate1, erode1, kernel, cv::Point(-1, -1), 2);
    cv::erode(erode1, erode2, kernel, cv::Point(-1, -1), 1);
    cv::dilate(erode2, dilate2, kernel, cv::Point(-1, -1), 1);

    cv::TickMeter tm3;
    tm3.start();
    cv::Canny(erode2, edges, 50, 150);
    tm3.stop();
    double dilation_time_ms = tm3.getTimeMilli();

    tm1.stop();
    double pre_processing_time_ms = tm1.getTimeMilli();

    cv::TickMeter tm2;
    tm2.start();
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    tm2.stop();
    double find_contours_time_ms = tm2.getTimeMilli();

    std::cout << "Dilation time: " << dilation_time_ms << " ms" << std::endl;
    std::cout << "Pre-processing time: " << pre_processing_time_ms << " ms" << std::endl;
    std::cout << "Find contours time: " << find_contours_time_ms << " ms" << std::endl;

    // cv::imshow("edges", edges);
    // cv::waitKey(0);
    // cv::destroyAllWindows();

    return {pre_processing_time_ms, find_contours_time_ms};
}

int main()
{
    std::string directory = "D:/CMake_egrabber/VS_EGrabber/EGrabber/test_images_cgy/uf_512x96";
    std::string background_path = directory + "/background.tiff";

    std::vector<double> pre_processing_times;
    std::vector<double> find_contours_times;

    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (entry.path().extension() == ".tiff" && entry.path().filename() != "background.tiff")
        {
            std::string image_path = entry.path().string();
            std::cout << "Processing: " << entry.path().filename() << std::endl;

            auto [pre_processing_time, find_contours_time] = process_image(image_path, background_path);

            pre_processing_times.push_back(pre_processing_time);
            find_contours_times.push_back(find_contours_time);
        }
    }

    // Calculate and print average times
    double avg_pre_processing = std::accumulate(pre_processing_times.begin(), pre_processing_times.end(), 0.0) / pre_processing_times.size();
    double avg_find_contours = std::accumulate(find_contours_times.begin(), find_contours_times.end(), 0.0) / find_contours_times.size();

    std::cout << "\nAverage pre-processing time: " << avg_pre_processing << " ms" << std::endl;
    std::cout << "Average find contours time: " << avg_find_contours << " ms" << std::endl;
    std::cout << "Total average time: " << avg_pre_processing + avg_find_contours << " ms" << std::endl;

    return 0;
}