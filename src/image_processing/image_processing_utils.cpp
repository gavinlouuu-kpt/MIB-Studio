#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <future>
#include <vector>
#include "menu_system/menu_system.h"

void createDefaultConfigIfMissing(const std::filesystem::path &configPath)
{
    if (!std::filesystem::exists(configPath))
    {
        std::ofstream file(configPath);
        file << "// Decrease the resolution before increasing the frame rate\n\n"
             << "// Make sure to remove offset before changing the resolution\n\n"
             << " var g = grabbers[0];\n"
             << " g.InterfacePort.set(\"LineSelector\", \"TTLIO12\");\n"
             << " g.InterfacePort.set(\"LineMode\", \"Output\");\n"
             << " g.InterfacePort.set(\"LineSource\", \"Low\");\n"
             << " g.RemotePort.set(\"Width\", 512);\n"
             << " g.RemotePort.set(\"Height\", 96);\n"
             << " g.RemotePort.set(\"OffsetY\", 500);\n"
             << " g.RemotePort.set(\"OffsetX\", 704);\n"
             << " g.RemotePort.set(\"ExposureTime\", 2);\n"
             << " g.RemotePort.set(\"AcquisitionFrameRate\", 5000);\n\n"
             << "\n\n"
             << "// Decrease the frame rate before upscaling to 1920x1080\n\n"
             << "// var g = grabbers[0];\n"
             << "// g.InterfacePort.set(\"LineSelector\", \"TTLIO12\");\n"
             << "// g.InterfacePort.set(\"LineMode\", \"Output\");\n"
             << "// g.InterfacePort.set(\"LineSource\", \"Low\");\n"
             << "// g.RemotePort.set(\"AcquisitionFrameRate\", 25);\n"
             << "// g.RemotePort.set(\"ExposureTime\", 20);\n"
             << "// g.RemotePort.set(\"OffsetY\", 0);\n"
             << "// g.RemotePort.set(\"OffsetX\", 0);\n"
             << "// g.RemotePort.set(\"Width\", 1920);\n"
             << "// g.RemotePort.set(\"Height\", 1080);\n";
        file.close();
    }
}

std::string selectSaveDirectory(const std::string &configPath)
{
    // Create output directory if it doesn't exist
    std::filesystem::path outputDir("output");
    if (!std::filesystem::exists(outputDir))
    {
        std::filesystem::create_directory(outputDir);
    }

    // Read current save directory from config
    json config = readConfig(configPath);
    std::string saveDir = config["save_directory"];

    std::cout << "Current save directory: " << saveDir << std::endl;
    std::cout << "Choose save directory option:\n";
    std::cout << "1: Use current directory\n";
    std::cout << "2: Enter new directory\n";
    std::cout << "3: Use testing directory (will overwrite existing)\n";
    std::cout << "Choice: ";

    int choice;
    std::cin >> choice;

    if (choice == 2)
    {
        std::cout << "Enter new save directory name: ";
        std::cin >> saveDir;

        // Update the config file with the new base directory
        updateConfig(configPath, "save_directory", saveDir);
    }
    else if (choice == 3)
    {
        saveDir = "testing";
        // Remove existing testing directory if it exists
        std::filesystem::path testPath = outputDir / saveDir;
        if (std::filesystem::exists(testPath))
        {
            std::filesystem::remove_all(testPath);
        }
        // Update the config file with the testing directory
        updateConfig(configPath, "save_directory", saveDir);
    }

    // Create full path within output directory
    std::filesystem::path fullPath = outputDir / saveDir;
    std::string basePath = fullPath.string();

    // Automatically increment the directory name if it already exists
    int suffix = 1;
    while (std::filesystem::exists(fullPath))
    {
        fullPath = outputDir / (saveDir + "_" + std::to_string(suffix));
        suffix++;
    }

    // Create the subdirectory
    std::filesystem::create_directories(fullPath);

    std::cout << "Using save directory: " << fullPath.string() << std::endl;

    return fullPath.string();
}

ImageParams initializeImageParams(const std::string &directory)
{
    ImageParams params;
    params.bufferCount = 5000;

    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".tiff" || entry.path().extension() == ".tif" ||
            entry.path().extension() == ".png" || entry.path().extension() == ".jpg" ||
            entry.path().extension() == ".jpeg")
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
        if (entry.path().extension() == ".tiff" || entry.path().extension() == ".tif" ||
            entry.path().extension() == ".png" || entry.path().extension() == ".jpg" ||
            entry.path().extension() == ".jpeg")
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
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                     cv::Size(shared.processingConfig.gaussian_blur_size,
                              shared.processingConfig.gaussian_blur_size),
                     0);

    // Apply simple contrast enhancement to the background if enabled
    if (shared.processingConfig.enable_contrast_enhancement)
    {
        // Use the formula: new_pixel = alpha * old_pixel + beta
        // alpha > 1 increases contrast, beta increases brightness
        shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                           shared.processingConfig.contrast_alpha,
                                           shared.processingConfig.contrast_beta);

        std::cout << "Background frame initialized with contrast enhancement applied." << std::endl;
    }
    else
    {
        std::cout << "Background frame initialized from loaded image at index: " << selectedIndex << std::endl;
    }

    // Record the timestamp when background was automatically initialized
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::lock_guard<std::mutex> timeLock(shared.backgroundCaptureTimeMutex);

    // Format time to only show hours:minutes:seconds
    std::tm timeInfo;
    localtime_s(&timeInfo, &time_t_now);
    char buffer[9]; // HH:MM:SS + null terminator
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
    shared.backgroundCaptureTime = std::string(buffer) + " (auto)"; // Indicate this was automatic initialization
}

void saveQualifiedResultsToDisk(const std::vector<QualifiedResult> &results, const std::string &directory, const SharedResources &shared)
{
    std::string batchDir = directory + "/batch_" + std::to_string(shared.currentBatchNumber);
    std::filesystem::create_directories(batchDir);

    std::string csvFilePath = batchDir + "/batch_data.csv";
    std::ofstream csvFile(csvFilePath);

    std::ofstream imageFile(batchDir + "/images.bin", std::ios::binary);
    
    // Save condition from configuration
    json condition_config = readConfig("config.json");
    std::string condition = condition_config["save_directory"];

    // Write CSV header
    csvFile << "Condition, Timestamp_us,Deformability,Area\n";

    // Add this block to save both background images
    if (!results.empty())
    {
        // Save clean background
        cv::imwrite(batchDir + "/background_clean.tiff", shared.backgroundFrame);

        // Save ROI coordinates to CSV
        std::ofstream roiFile(batchDir + "/roi.csv");
        roiFile << "x,y,width,height\n";
        roiFile << shared.roi.x << ","
                << shared.roi.y << ","
                << shared.roi.width << ","
                << shared.roi.height;
        roiFile.close();

        // Save processing configuration
        json config = readConfig("config.json");
        std::ofstream configFile(batchDir + "/processing_config.json");
        configFile << std::setw(4) << config["image_processing"] << std::endl;
        configFile.close();
    }

    for (const auto &result : results)
    {
        // Write data to CSV
        csvFile << condition << ","
                << result.timestamp << ","
                << result.deformability << ","
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

        std::string outputPath = outputDirectory + "/image_" + std::to_string(imageCount++) + ".tiff";
        cv::imwrite(outputPath, image);
    }

    std::cout << "Converted " << imageCount << " images to TIFF format in " << outputDirectory << std::endl;
}

json readConfig(const std::string &filename)
{
    json config;

    if (!std::filesystem::exists(filename))
    {
        // Create default config with a more structured approach
        json image_processing = {
            {"gaussian_blur_size", 3},
            {"bg_subtract_threshold", 10},
            {"morph_kernel_size", 3},
            {"morph_iterations", 1},
            {"area_threshold_min", 100},
            {"area_threshold_max", 600},
            {"filters", {{"enable_border_check", true}, {"enable_multiple_contours_check", true}, {"enable_area_range_check", true}, {"require_single_inner_contour", true}}},
            {"contrast_enhancement", {{"enable_contrast", true}, {"alpha", 1.2}, {"beta", 10}}}};

        config = {
            {"save_directory", "updated_results"},
            {"buffer_threshold", 1000},
            {"target_fps", 5000},
            {"scatter_plot_enabled", false},
            {"image_processing", image_processing}};

        // Write default config to file
        std::ofstream configFile(filename);
        configFile << std::setw(4) << config << std::endl;
        std::cout << "Created default config file: " << filename << std::endl;
    }
    else
    {
        // Read existing config file and ensure all required fields exist
        std::ifstream configFile(filename);
        configFile >> config;

        // Ensure image_processing section exists with all required fields
        if (!config.contains("image_processing"))
        {
            config["image_processing"] = json::object();
        }

        auto &img_config = config["image_processing"];

        // Set defaults for any missing fields
        if (!img_config.contains("gaussian_blur_size"))
            img_config["gaussian_blur_size"] = 3;
        if (!img_config.contains("bg_subtract_threshold"))
            img_config["bg_subtract_threshold"] = 10;
        if (!img_config.contains("morph_kernel_size"))
            img_config["morph_kernel_size"] = 3;
        if (!img_config.contains("morph_iterations"))
            img_config["morph_iterations"] = 1;
        if (!img_config.contains("area_threshold_min"))
            img_config["area_threshold_min"] = 100;
        if (!img_config.contains("area_threshold_max"))
            img_config["area_threshold_max"] = 600;

        // Ensure filters section exists
        if (!img_config.contains("filters"))
        {
            img_config["filters"] = json::object();
        }

        auto &filters = img_config["filters"];

        // Set defaults for filter flags
        if (!filters.contains("enable_border_check"))
            filters["enable_border_check"] = true;
        if (!filters.contains("enable_multiple_contours_check"))
            filters["enable_multiple_contours_check"] = true;
        if (!filters.contains("enable_area_range_check"))
            filters["enable_area_range_check"] = true;
        if (!filters.contains("require_single_inner_contour"))
            filters["require_single_inner_contour"] = true;

        // Ensure contrast_enhancement section exists
        if (!img_config.contains("contrast_enhancement"))
        {
            img_config["contrast_enhancement"] = json::object();
        }

        auto &contrast = img_config["contrast_enhancement"];

        // Set defaults for contrast enhancement parameters
        if (!contrast.contains("enable_contrast"))
            contrast["enable_contrast"] = true;
        if (!contrast.contains("alpha"))
            contrast["alpha"] = 1.2;
        if (!contrast.contains("beta"))
            contrast["beta"] = 10;

        // Write back the complete config to ensure file has all fields
        std::ofstream outFile(filename);
        outFile << std::setw(4) << config << std::endl;
    }

    return config;
}

ProcessingConfig getProcessingConfig(const json &config)
{
    const auto &img_config = config["image_processing"];
    const auto &filters = img_config.contains("filters") ? img_config["filters"] : json::object();
    const auto &contrast = img_config.contains("contrast_enhancement") ? img_config["contrast_enhancement"] : json::object();

    // Get filter flags with defaults if not present
    bool enable_border_check = filters.contains("enable_border_check") ? filters["enable_border_check"].get<bool>() : true;
    bool enable_multiple_contours_check = filters.contains("enable_multiple_contours_check") ? filters["enable_multiple_contours_check"].get<bool>() : true;
    bool enable_area_range_check = filters.contains("enable_area_range_check") ? filters["enable_area_range_check"].get<bool>() : true;
    bool require_single_inner_contour = filters.contains("require_single_inner_contour") ? filters["require_single_inner_contour"].get<bool>() : true;

    // Get contrast enhancement parameters with defaults if not present
    bool enable_contrast = contrast.contains("enable_contrast") ? contrast["enable_contrast"].get<bool>() : true;
    double alpha = contrast.contains("alpha") ? contrast["alpha"].get<double>() : 1.2;
    int beta = contrast.contains("beta") ? contrast["beta"].get<int>() : 10;

    return ProcessingConfig{
        img_config["gaussian_blur_size"],
        img_config["bg_subtract_threshold"],
        img_config["morph_kernel_size"],
        img_config["morph_iterations"],
        img_config["area_threshold_min"],
        img_config["area_threshold_max"],
        enable_border_check,
        enable_multiple_contours_check,
        enable_area_range_check,
        require_single_inner_contour,
        enable_contrast,
        alpha,
        beta};
}

// Function to update the background with current contrast settings
void updateBackgroundWithCurrentSettings(SharedResources &shared)
{
    if (shared.backgroundFrame.empty())
    {
        return; // No background to update
    }

    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);

    // Re-apply Gaussian blur to the original background frame
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                     cv::Size(shared.processingConfig.gaussian_blur_size,
                              shared.processingConfig.gaussian_blur_size),
                     0);

    // Apply simple contrast enhancement to the background if enabled
    if (shared.processingConfig.enable_contrast_enhancement)
    {
        // Use the formula: new_pixel = alpha * old_pixel + beta
        // alpha > 1 increases contrast, beta increases brightness
        shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                           shared.processingConfig.contrast_alpha,
                                           shared.processingConfig.contrast_beta);
    }
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

        // Update the specified key
        if (key.find('.') != std::string::npos)
        {
            // Handle nested keys (e.g., "image_processing.gaussian_blur_size")
            std::istringstream ss(key);
            std::string token;
            std::vector<std::string> keys;
            while (std::getline(ss, token, '.'))
            {
                keys.push_back(token);
            }

            // Navigate to the nested object
            json *current = &config;
            for (size_t i = 0; i < keys.size() - 1; i++)
            {
                if (!current->contains(keys[i]))
                {
                    (*current)[keys[i]] = json::object();
                }
                current = &(*current)[keys[i]];
            }

            // Update the value
            (*current)[keys.back()] = value;
        }
        else
        {
            // Simple key
            config[key] = value;
        }

        // Write updated config back to file
        std::ofstream output_file(filename);
        if (!output_file.is_open())
        {
            std::cerr << "Unable to write to config file: " << filename << std::endl;
            return false;
        }
        output_file << std::setw(4) << config << std::endl;
        output_file.close();

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating config: " << e.what() << std::endl;
        return false;
    }
}

void reviewSavedData()
{
    std::string projectPath = MenuSystem::navigateAndSelectFolder();
    std::vector<std::filesystem::path> batchDirs;
    ProcessingConfig processingConfig;

    // Remove this since we'll load it per batch
    // json config = readConfig("config.json");

    auto loadBatchConfig = [](const std::filesystem::path &batchPath) -> ProcessingConfig
    {
        std::ifstream configFile(batchPath / "processing_config.json");
        if (!configFile.is_open())
        {
            throw std::runtime_error("Failed to load processing config from: " + batchPath.string());
        }
        json config;
        configFile >> config;

        return ProcessingConfig{
            config["gaussian_blur_size"],
            config["bg_subtract_threshold"],
            config["morph_kernel_size"],
            config["morph_iterations"]};
    };

    for (const auto &entry : std::filesystem::directory_iterator(projectPath))
    {
        if (entry.is_directory() && entry.path().filename().string().find("batch_") != std::string::npos)
        {
            batchDirs.push_back(entry.path());
        }
    }

    if (batchDirs.empty())
    {
        std::cout << "No batch directories found in " << projectPath << std::endl;
        return;
    }

    auto initializeBatch = [&processingConfig, &loadBatchConfig](const std::filesystem::path &batchPath, SharedResources &shared) -> cv::Mat
    {
        // Load batch-specific processing config
        processingConfig = loadBatchConfig(batchPath);
        shared.processingConfig = processingConfig;
        // Load background image
        cv::Mat backgroundClean = cv::imread((batchPath / "background_clean.tiff").string(), cv::IMREAD_GRAYSCALE);
        if (backgroundClean.empty())
        {
            throw std::runtime_error("Failed to load background image from: " + batchPath.string());
        }

        // Read ROI from CSV
        std::ifstream roiFile((batchPath / "roi.csv").string());
        std::string roiHeader;
        std::getline(roiFile, roiHeader); // Skip header
        std::string roiData;
        std::getline(roiFile, roiData);
        std::stringstream ss(roiData);
        std::string value;
        std::vector<int> roiValues;
        while (std::getline(ss, value, ','))
        {
            roiValues.push_back(std::stoi(value));
        }

        // Initialize shared resources
        shared.roi = cv::Rect(roiValues[0], roiValues[1], roiValues[2], roiValues[3]);
        cv::GaussianBlur(backgroundClean, shared.blurredBackground, cv::Size(3, 3), 0);

        return backgroundClean;
    };

    std::sort(batchDirs.begin(), batchDirs.end());
    size_t currentBatchIndex = 0;
    size_t currentImageIndex = 0;
    bool showProcessed = false;

    // Initialize resources
    cv::Mat backgroundClean;
    SharedResources shared;

    // Initial batch setup
    backgroundClean = initializeBatch(batchDirs[currentBatchIndex], shared);
    ThreadLocalMats mats = initializeThreadMats(backgroundClean.rows, backgroundClean.cols, shared);

    // Create display window
    cv::namedWindow("Data Review", cv::WINDOW_NORMAL);
    cv::resizeWindow("Data Review", backgroundClean.cols, backgroundClean.rows);

    bool running = true;
    while (running)
    {
        // Load current batch's binary images
        std::ifstream imageFile((batchDirs[currentBatchIndex] / "images.bin").string(), std::ios::binary);
        std::vector<cv::Mat> images;

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
            images.push_back(image.clone());
        }

        // Load CSV data
        std::ifstream csvFile((batchDirs[currentBatchIndex] / "batch_data.csv").string());
        std::string line;
        std::getline(csvFile, line);                                     // Skip header
        std::vector<std::tuple<long long, double, double>> measurements; // timestamp, deformability, area

        while (std::getline(csvFile, line))
        {
            std::stringstream ss(line);
            std::string value;
            std::vector<std::string> values;

            while (std::getline(ss, value, ','))
            {
                values.push_back(value);
            }

            if (values.size() == 3)
            {
                measurements.emplace_back(
                    std::stoll(values[0]),
                    std::stod(values[1]),
                    std::stod(values[2]));
            }
        }

        while (currentImageIndex < images.size() && running)
        {
            // Create display image
            cv::Mat displayImage;
            cv::cvtColor(images[currentImageIndex], displayImage, cv::COLOR_GRAY2BGR);

            if (showProcessed)
            {
                cv::Mat processedImage = cv::Mat(images[currentImageIndex].rows,
                                                 images[currentImageIndex].cols,
                                                 CV_8UC1);
                processFrame(images[currentImageIndex], shared, processedImage, mats);

                cv::Mat processedOverlay;
                cv::cvtColor(processedImage, processedOverlay, cv::COLOR_GRAY2BGR);
                cv::addWeighted(displayImage, 0.7, processedOverlay, 0.3, 0, displayImage);
            }

            // Draw ROI rectangle
            cv::rectangle(displayImage, shared.roi, cv::Scalar(0, 255, 0), 2);

            // Add text overlay with measurements
            if (currentImageIndex < measurements.size())
            {
                auto [timestamp, deformability, area] = measurements[currentImageIndex];
                std::string info = "Batch: " + std::to_string(currentBatchIndex) +
                                   " | Frame: " + std::to_string(currentImageIndex) + "/" + std::to_string(images.size() - 1) +
                                   " | Deformability: " + std::to_string(deformability) +
                                   " | Area: " + std::to_string(area) +
                                   " | Processing: " + (showProcessed ? "ON" : "OFF");
                cv::putText(displayImage, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                            0.7, cv::Scalar(0, 255, 0), 2);
            }

            cv::imshow("Data Review", displayImage);

            // Handle keyboard input
            int key = cv::waitKey(0);
            switch (key)
            {
            case 27: // ESC
                running = false;
                break;
            case ' ': // Spacebar - toggle processing overlay
                showProcessed = !showProcessed;
                break;
            case 'a': // Previous image
                if (currentImageIndex > 0)
                    currentImageIndex--;
                break;
            case 'd': // Next image
                if (currentImageIndex < images.size() - 1)
                    currentImageIndex++;
                break;
            case 'q': // Previous batch
                if (currentBatchIndex > 0)
                {
                    currentBatchIndex--;
                    currentImageIndex = 0;
                    backgroundClean = initializeBatch(batchDirs[currentBatchIndex], shared);
                    mats = initializeThreadMats(backgroundClean.rows, backgroundClean.cols, shared);
                    break;
                }
                break;
            case 'e': // Next batch
                if (currentBatchIndex < batchDirs.size() - 1)
                {
                    currentBatchIndex++;
                    currentImageIndex = 0;
                    backgroundClean = initializeBatch(batchDirs[currentBatchIndex], shared);
                    mats = initializeThreadMats(backgroundClean.rows, backgroundClean.cols, shared);
                    break;
                }
                break;
            }

            // If batch changed, break inner loop to load new batch data
            if (key == 'q' || key == 'e')
                break;
        }
    }

    cv::destroyAllWindows();
}
