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
        file 
                << "var g = grabbers[0];\n"
                << "g.RemotePort.execute(\"AcquisitionStop\");\n"
                << "g.InterfacePort.set(\"LineSelector\", \"TTLIO12\");//Trigger\n"
                << "g.InterfacePort.set(\"LineMode\", \"Output\");\n"
                << "g.InterfacePort.set(\"LineSource\", \"Low\");\n"
                << "g.InterfacePort.set(\"LineSelector\", \"TTLIO11\"); //LED\n"
                << "g.InterfacePort.set(\"LineMode\", \"Output\");\n"
                << "g.InterfacePort.set(\"LineInverter\", true);\n"
                << "g.InterfacePort.set(\"LineSource\", \"Device0Strobe\");\n"
                << "g.RemotePort.set(\"Width\", 512);\n"
                << "g.RemotePort.set(\"Height\", 96);\n"
                << "g.RemotePort.set(\"OffsetY\", 500);\n"
                << "g.RemotePort.set(\"OffsetX\", 704);\n"
                << "g.RemotePort.set(\"ExposureTime\", 3);\n"
                << "g.DevicePort.set(\"CameraControlMethod\", \"RC\");\n"
                << "g.DevicePort.set(\"ExposureRecoveryTime\", \"200\");\n"
                << "g.DevicePort.set(\"CycleMinimumPeriod\", \"200\");\n"
                << "g.DevicePort.set(\"StrobeDelay\", \"-4\");\n"
                << "g.DevicePort.set(\"StrobeDuration\", \"12\");\n"
                << "g.RemotePort.set(\"TriggerMode\", \"On\");\n"
                << "g.RemotePort.set(\"TriggerSource\", \"LinkTrigger0\");\n"
                << "g.RemotePort.execute(\"AcquisitionStart\");\n";

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
    // Read target FPS from config.json
    json config = readConfig("config.json");
    const int simCameraTargetFPS = config.value("simCameraTargetFPS", 5000); // Default to 5000 if not specified
    params.bufferCount = simCameraTargetFPS;

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
    // Save condition from configuration
    json condition_config = readConfig("config.json");
    std::string condition = condition_config["save_directory"];
    
    // Create master file paths
    std::string masterCsvPath = directory + "/" + condition + "_data.csv";
    std::string masterImagesPath = directory + "/" + condition + "_images.bin";
    std::string masterMasksPath = directory + "/" + condition + "_masks.bin";
    std::string masterBackgroundsPath = directory + "/" + condition + "_backgrounds.bin";
    std::string masterROIPath = directory + "/" + condition + "_roi.csv";
    std::string masterConfigPath = directory + "/" + condition + "_processing_config.json";
    
    // Check if master files exist
    bool masterFileExists = std::filesystem::exists(masterCsvPath);
    bool masterROIFileExists = std::filesystem::exists(masterROIPath);
    bool masterConfigFileExists = std::filesystem::exists(masterConfigPath);
    
    // Open master CSV file in append mode
    std::ofstream masterCsvFile(masterCsvPath, std::ios::app);
    
    // Open master images binary file in binary + append mode
    std::ofstream masterImageFile(masterImagesPath, std::ios::binary | std::ios::app);
    
    // Open master masks binary file in binary + append mode
    std::ofstream masterMaskFile(masterMasksPath, std::ios::binary | std::ios::app);
    
    // Open master background images binary file in binary + append mode
    std::ofstream masterBackgroundFile(masterBackgroundsPath, std::ios::binary | std::ios::app);
    
    // Open master ROI CSV file in append mode
    std::ofstream masterROIFile(masterROIPath, std::ios::app);
    
    // Initialize master config
    json masterConfig;
    
    // Load existing master config if it exists
    if (masterConfigFileExists) {
        std::ifstream configIn(masterConfigPath);
        try {
            configIn >> masterConfig;
        } catch (const std::exception& e) {
            // If parsing fails, start with a new empty JSON object
            masterConfig = json::object();
        }
    }

    // Write header to master CSV if it's a new file
    if (!masterFileExists) {
        masterCsvFile << "Batch,Condition,Timestamp_us,Deformability,Area,RingRatio,Brightness_Q1,Brightness_Q2,Brightness_Q3,Brightness_Q4\n";
    }
    
    // Write header to master ROI CSV if it's a new file
    if (!masterROIFileExists) {
        masterROIFile << "Batch,x,y,width,height\n";
    }

    // Save batch data to master files
    if (!results.empty())
    {
        // Save background to master file
        int rows = shared.backgroundFrame.rows;
        int cols = shared.backgroundFrame.cols;
        int type = shared.backgroundFrame.type();
        masterBackgroundFile.write(reinterpret_cast<const char *>(&shared.currentBatchNumber), sizeof(int));
        masterBackgroundFile.write(reinterpret_cast<const char *>(&rows), sizeof(int));
        masterBackgroundFile.write(reinterpret_cast<const char *>(&cols), sizeof(int));
        masterBackgroundFile.write(reinterpret_cast<const char *>(&type), sizeof(int));
        
        if (shared.backgroundFrame.isContinuous()) {
            masterBackgroundFile.write(reinterpret_cast<const char *>(shared.backgroundFrame.data), 
                                      shared.backgroundFrame.total() * shared.backgroundFrame.elemSize());
        } else {
            for (int r = 0; r < rows; ++r) {
                masterBackgroundFile.write(reinterpret_cast<const char *>(shared.backgroundFrame.ptr(r)), 
                                          cols * shared.backgroundFrame.elemSize());
            }
        }
        
        // Save ROI to master file
        masterROIFile << shared.currentBatchNumber << ","
                      << shared.roi.x << ","
                      << shared.roi.y << ","
                      << shared.roi.width << ","
                      << shared.roi.height << "\n";

        // Add batch config to master config
        std::string batchKey = "batch_" + std::to_string(shared.currentBatchNumber);
        json config = readConfig("config.json");
        masterConfig[batchKey] = config["image_processing"];
        
        // Write updated master config file
        std::ofstream masterConfigFile(masterConfigPath);
        masterConfigFile << std::setw(4) << masterConfig << std::endl;
    }

    for (const auto &result : results)
    {
        // Write to master CSV
        masterCsvFile << shared.currentBatchNumber << ","
                      << condition << ","
                      << result.timestamp << ","
                      << result.deformability << ","
                      << result.area << ","
                      << result.ringRatio << ","
                      << result.brightness.q1 << ","
                      << result.brightness.q2 << ","
                      << result.brightness.q3 << ","
                      << result.brightness.q4 << "\n";

        // Write to master images file
        int rows = result.originalImage.rows;
        int cols = result.originalImage.cols;
        int type = result.originalImage.type();
        
        masterImageFile.write(reinterpret_cast<const char *>(&rows), sizeof(int));
        masterImageFile.write(reinterpret_cast<const char *>(&cols), sizeof(int));
        masterImageFile.write(reinterpret_cast<const char *>(&type), sizeof(int));

        if (result.originalImage.isContinuous())
        {
            masterImageFile.write(reinterpret_cast<const char *>(result.originalImage.data), 
                                 result.originalImage.total() * result.originalImage.elemSize());
        }
        else
        {
            for (int r = 0; r < rows; ++r)
            {
                masterImageFile.write(reinterpret_cast<const char *>(result.originalImage.ptr(r)), 
                                     cols * result.originalImage.elemSize());
            }
        }
        
        // Write to master masks file
        rows = result.processedImage.rows;
        cols = result.processedImage.cols;
        type = result.processedImage.type();
        
        masterMaskFile.write(reinterpret_cast<const char *>(&rows), sizeof(int));
        masterMaskFile.write(reinterpret_cast<const char *>(&cols), sizeof(int));
        masterMaskFile.write(reinterpret_cast<const char *>(&type), sizeof(int));

        if (result.processedImage.isContinuous())
        {
            masterMaskFile.write(reinterpret_cast<const char *>(result.processedImage.data), 
                                result.processedImage.total() * result.processedImage.elemSize());
        }
        else
        {
            for (int r = 0; r < rows; ++r)
            {
                masterMaskFile.write(reinterpret_cast<const char *>(result.processedImage.ptr(r)), 
                                    cols * result.processedImage.elemSize());
            }
        }
    }
    
    masterCsvFile.close();
    masterImageFile.close();
    masterMaskFile.close();
    masterBackgroundFile.close();
    masterROIFile.close();

    // std::cout << "Saved " << results.size() << " results to master files in " << directory << std::endl;
}

// New utility function to calculate metrics from saved images and output to CSV
void calculateMetricsFromSavedData(const std::string &inputDirectory, const std::string &outputFilePath)
{
    std::cout << "Calculating metrics from saved data in: " << inputDirectory << std::endl;
    
    // Ensure we have an absolute path for reliable file checks
    std::filesystem::path inputDirPath = std::filesystem::absolute(inputDirectory);
    std::string absInputDir = inputDirPath.string();
    std::cout << "Absolute path: " << absInputDir << std::endl;
    
    // Create overlays directory for saving images with mask overlays
    std::filesystem::path overlaysDir = inputDirPath / "overlays";
    if (!std::filesystem::exists(overlaysDir)) {
        std::filesystem::create_directories(overlaysDir);
        std::cout << "Created directory for overlay images: " << overlaysDir.string() << std::endl;
    }
    
    // Auto-detect the file prefix from the directory contents
    std::string condition = "";
    for (const auto& entry : std::filesystem::directory_iterator(absInputDir)) {
        std::string filename = entry.path().filename().string();
        if (filename.size() > 16 && filename.substr(filename.size() - 16) == "_backgrounds.bin") {
            condition = filename.substr(0, filename.size() - 16);
            std::cout << "Auto-detected file prefix: " << condition << std::endl;
            break;
        }
    }
    
    if (condition.empty()) {
        // Try alternative detection with other common file extensions
        for (const auto& suffix : {"_processing_config.json", "_roi.csv", "_images.bin", "_data.csv"}) {
            for (const auto& entry : std::filesystem::directory_iterator(absInputDir)) {
                std::string filename = entry.path().filename().string();
                if (filename.size() > strlen(suffix) && 
                    filename.substr(filename.size() - strlen(suffix)) == suffix) {
                    condition = filename.substr(0, filename.size() - strlen(suffix));
                    std::cout << "Auto-detected file prefix from " << suffix << ": " << condition << std::endl;
                    goto prefix_found;  // Break out of nested loops
                }
            }
        }
    }
    
prefix_found:
    if (condition.empty()) {
        std::cerr << "Could not auto-detect file prefix. No data files found." << std::endl;
        return;
    }
    
    bool hasMasterFiles = false;
    std::string masterConfigPath = absInputDir + "/" + condition + "_processing_config.json";
    std::string masterROIPath = absInputDir + "/" + condition + "_roi.csv";
    std::string masterBackgroundsPath = absInputDir + "/" + condition + "_backgrounds.bin";
    std::string masterImagesPath = absInputDir + "/" + condition + "_images.bin";
    std::string masterDataPath = absInputDir + "/" + condition + "_data.csv";
    
    // Check if all master files exist
    if (std::filesystem::exists(masterConfigPath) && 
        std::filesystem::exists(masterROIPath) && 
        std::filesystem::exists(masterBackgroundsPath) &&
        std::filesystem::exists(masterImagesPath)) {
        hasMasterFiles = true;
        std::cout << "Found consolidated master files. Using them for metrics calculation." << std::endl;
    } else {
        // Try alternative format (older datasets might have different naming)
        std::string altConfigPath = absInputDir + "/" + condition + ".json";
        std::string altDataPath = absInputDir + "/" + condition + ".csv";
        std::string altImagesPath = absInputDir + "/" + condition + ".bin";
        
        if (std::filesystem::exists(altImagesPath) && std::filesystem::exists(altDataPath)) {
            std::cout << "Found alternative format master files. Attempting to use them." << std::endl;
            // Use these files instead
            masterImagesPath = altImagesPath;
            masterDataPath = altDataPath;
            if (std::filesystem::exists(altConfigPath)) {
                masterConfigPath = altConfigPath;
            }
            hasMasterFiles = true;
        } else {
            // Print detailed information about which files are missing
            std::cout << "Not using master files because some are missing:" << std::endl;
            std::cout << "  Config: " << (std::filesystem::exists(masterConfigPath) ? "Found" : "Missing") << std::endl;
            std::cout << "  ROI: " << (std::filesystem::exists(masterROIPath) ? "Found" : "Missing") << std::endl;
            std::cout << "  Backgrounds: " << (std::filesystem::exists(masterBackgroundsPath) ? "Found" : "Missing") << std::endl;
            std::cout << "  Images: " << (std::filesystem::exists(masterImagesPath) ? "Found" : "Missing") << std::endl;
            std::cout << "  Data: " << (std::filesystem::exists(masterDataPath) ? "Found" : "Missing") << std::endl;
            std::cout << "Will try to use individual batch directories instead." << std::endl;
        }
    }
    
    // Create output CSV file
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to create output file: " << outputFilePath << std::endl;
        return;
    }
    
    // Write CSV header
    outputFile << "Batch,Condition,ImageIndex,Timestamp_us,Deformability,Area,RingRatio,Valid,Method,ProcessingConfig\n";
    
    // Helper function to load processing config for a batch
    auto loadMasterConfig = [](const std::string &configPath, int batchNum) -> ProcessingConfig {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to load master processing config from: " + configPath);
        }
        json masterConfig;
        configFile >> masterConfig;
        
        // Get the config for the specified batch
        std::string batchKey = "batch_" + std::to_string(batchNum);
        if (!masterConfig.contains(batchKey)) {
            throw std::runtime_error("Master config file does not contain configuration for batch " + std::to_string(batchNum));
        }
        
        json config = masterConfig[batchKey];

        return ProcessingConfig{
            config["gaussian_blur_size"],
            config["bg_subtract_threshold"],
            config["morph_kernel_size"],
            config["morph_iterations"],
            config["area_threshold_min"],
            config["area_threshold_max"],
            config.contains("filters") && config["filters"].contains("enable_border_check") ? 
                config["filters"]["enable_border_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_multiple_contours_check") ? 
                config["filters"]["enable_multiple_contours_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_area_range_check") ? 
                config["filters"]["enable_area_range_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("require_single_inner_contour") ? 
                config["filters"]["require_single_inner_contour"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("enable_contrast") ? 
                config["contrast_enhancement"]["enable_contrast"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("alpha") ? 
                config["contrast_enhancement"]["alpha"].get<double>() : 1.2,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("beta") ? 
                config["contrast_enhancement"]["beta"].get<int>() : 10
        };
    };
    
    // Helper function to load ROI for a batch from master CSV
    auto loadROIFromMasterCSV = [](const std::string &roiPath, int batchNum) -> cv::Rect {
        std::ifstream roiFile(roiPath);
        if (!roiFile.is_open()) {
            throw std::runtime_error("Failed to open master ROI file: " + roiPath);
        }
        
        std::string header;
        std::getline(roiFile, header); // Skip header
        
        std::string line;
        while (std::getline(roiFile, line)) {
            std::stringstream ss(line);
            std::string value;
            std::vector<std::string> values;
            
            while (std::getline(ss, value, ',')) {
                values.push_back(value);
            }
            
            if (values.size() >= 5 && std::stoi(values[0]) == batchNum) {
                return cv::Rect(
                    std::stoi(values[1]), // x
                    std::stoi(values[2]), // y
                    std::stoi(values[3]), // width
                    std::stoi(values[4])  // height
                );
            }
        }
        
        throw std::runtime_error("Failed to find ROI for batch " + std::to_string(batchNum) + " in master ROI file");
    };
    
    // Helper function to load background for a batch from master binary file
    auto loadBackgroundFromMasterBin = [](const std::string &bgPath, int batchNum) -> cv::Mat {
        std::ifstream bgFile(bgPath, std::ios::binary);
        if (!bgFile.is_open()) {
            throw std::runtime_error("Failed to open master backgrounds file: " + bgPath);
        }
        
        while (bgFile.good()) {
            int storedBatchNum, rows, cols, type;
            bgFile.read(reinterpret_cast<char *>(&storedBatchNum), sizeof(int));
            
            if (bgFile.eof()) {
                break;
            }
            
            bgFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
            bgFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
            bgFile.read(reinterpret_cast<char *>(&type), sizeof(int));
            
            cv::Mat background(rows, cols, type);
            bgFile.read(reinterpret_cast<char *>(background.data), rows * cols * background.elemSize());
            
            if (storedBatchNum == batchNum) {
                return background.clone(); // Found the background for this batch
            }
            
            // If not this batch, skip to next background
            if (bgFile.eof()) {
                break;
            }
        }
        
        throw std::runtime_error("Failed to find background for batch " + std::to_string(batchNum) + " in master backgrounds file");
    };
    
    // Helper function to load batch configuration
    auto loadBatchConfig = [](const std::filesystem::path &batchPath) -> ProcessingConfig {
        std::ifstream configFile(batchPath / "processing_config.json");
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to load processing config from: " + batchPath.string());
        }
        json config;
        configFile >> config;

        return ProcessingConfig{
            config["gaussian_blur_size"],
            config["bg_subtract_threshold"],
            config["morph_kernel_size"],
            config["morph_iterations"],
            config["area_threshold_min"],
            config["area_threshold_max"],
            config.contains("filters") && config["filters"].contains("enable_border_check") ? 
                config["filters"]["enable_border_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_multiple_contours_check") ? 
                config["filters"]["enable_multiple_contours_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_area_range_check") ? 
                config["filters"]["enable_area_range_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("require_single_inner_contour") ? 
                config["filters"]["require_single_inner_contour"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("enable_contrast") ? 
                config["contrast_enhancement"]["enable_contrast"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("alpha") ? 
                config["contrast_enhancement"]["alpha"].get<double>() : 1.2,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("beta") ? 
                config["contrast_enhancement"]["beta"].get<int>() : 10
        };
    };
    
    // Processing function to get a string representation of the config for CSV
    auto configToString = [](const ProcessingConfig &config) -> std::string {
        std::stringstream ss;
        ss << "G" << config.gaussian_blur_size 
           << "_T" << config.bg_subtract_threshold 
           << "_M" << config.morph_kernel_size << "x" << config.morph_iterations
           << "_A" << config.area_threshold_min << "-" << config.area_threshold_max;
        return ss.str();
    };
    
    if (hasMasterFiles) {
        // Process all images from the master images file
        std::vector<cv::Mat> allImages;
        std::vector<std::tuple<int, std::string, long long, double, double>> allMeasurements;
        std::set<int> availableBatches;
        
        // Load all measurements from the master CSV if available
        if (std::filesystem::exists(masterDataPath)) {
            std::ifstream csvFile(masterDataPath);
            std::string headerLine;
            std::getline(csvFile, headerLine); // Get header line
            
            // Parse headers to find column indices
            auto headers = parseCSVHeaders(headerLine);
            
            // Check for required columns
            if (headers.count("Batch") && headers.count("Timestamp_us") && 
                headers.count("Deformability") && headers.count("Area")) {
                
                // Get column indices
                int batchIdx = headers["Batch"];
                int conditionIdx = headers.count("Condition") ? headers["Condition"] : -1;
                int timestampIdx = headers["Timestamp_us"];
                int deformabilityIdx = headers["Deformability"];
                int areaIdx = headers["Area"];
                
                std::string line;
                while (std::getline(csvFile, line)) {
                    std::stringstream lineStream(line);
                    std::string cell;
                    std::vector<std::string> values;
                    
                    while (std::getline(lineStream, cell, ',')) {
                        values.push_back(cell);
                    }
                    
                    if (values.size() > std::max({batchIdx, conditionIdx, timestampIdx, deformabilityIdx, areaIdx})) {
                        try {
                            int batchNum = std::stoi(values[batchIdx]);
                            std::string condition = (conditionIdx >= 0) ? values[conditionIdx] : "unknown";
                            
                            allMeasurements.emplace_back(
                                batchNum,
                                condition,
                                std::stoll(values[timestampIdx]),
                                std::stod(values[deformabilityIdx]),
                                std::stod(values[areaIdx])
                            );
                            
                            availableBatches.insert(batchNum);
                        } catch (const std::exception& e) {
                            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
                        }
                    }
                }
            } else {
                std::cerr << "Master data CSV is missing required columns. Proceeding without stored measurements." << std::endl;
            }
        }
        
        // First pass: find all available batch numbers if not already loaded from master data
        if (availableBatches.empty()) {
            std::ifstream roiFile(masterROIPath);
            std::string header;
            std::getline(roiFile, header); // Skip header
            
            std::string line;
            while (std::getline(roiFile, line)) {
                std::stringstream ss(line);
                std::string value;
                std::vector<std::string> values;
                
                while (std::getline(ss, value, ',')) {
                    values.push_back(value);
                }
                
                if (!values.empty()) {
                    try {
                        availableBatches.insert(std::stoi(values[0]));
                    } catch (...) {
                        // Skip invalid lines
                    }
                }
            }
        }
        
        std::cout << "Found " << availableBatches.size() << " batches in master files." << std::endl;
        
        // Load all images from the master images binary file
        std::ifstream imageFile(masterImagesPath, std::ios::binary);
        while (imageFile.good()) {
            int rows, cols, type;
            imageFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
            imageFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
            imageFile.read(reinterpret_cast<char *>(&type), sizeof(int));
            
            if (imageFile.eof()) {
                break;
            }
            
            cv::Mat image(rows, cols, type);
            imageFile.read(reinterpret_cast<char *>(image.data), rows * cols * image.elemSize());
            allImages.push_back(image.clone());
        }
        
        std::cout << "Loaded " << allImages.size() << " images from master files." << std::endl;
        
        // Create a map to track how many images belong to each batch
        std::map<int, int> batchImageCounts;
        for (const auto &measurement : allMeasurements) {
            batchImageCounts[std::get<0>(measurement)]++;
        }
        
        // Create a map to track the starting index for each batch's images
        std::map<int, size_t> batchStartIndices;
        size_t currentIndex = 0;
        
        // Process each batch
        for (int batchNum : availableBatches) {
            std::cout << "Processing batch " << batchNum << "..." << std::endl;
            
            // Store the starting index for this batch
            batchStartIndices[batchNum] = currentIndex;
            
            // Load background, ROI, and processing config for this batch
            SharedResources shared;
            try {
                shared.backgroundFrame = loadBackgroundFromMasterBin(masterBackgroundsPath, batchNum);
                shared.roi = loadROIFromMasterCSV(masterROIPath, batchNum);
                shared.processingConfig = loadMasterConfig(masterConfigPath, batchNum);
                
                // Initialize the blurred background with the original config settings
                cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                                cv::Size(shared.processingConfig.gaussian_blur_size,
                                        shared.processingConfig.gaussian_blur_size), 0);

                if (shared.processingConfig.enable_contrast_enhancement) {
                    shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                                    shared.processingConfig.contrast_alpha,
                                                    shared.processingConfig.contrast_beta);
                }
            } catch (const std::exception &e) {
                std::cerr << "Error loading batch " << batchNum << " data: " << e.what() << std::endl;
                continue; // Skip this batch
            }
            
            // Initialize thread mats for processing
            ThreadLocalMats mats = initializeThreadMats(shared.backgroundFrame.rows, 
                                                       shared.backgroundFrame.cols, shared);
            
            // Filter images for this batch
            std::vector<cv::Mat> batchImages;
            std::vector<std::tuple<std::string, long long, double, double>> batchMeasurements;
            
            // Extract all measurements for this batch
            for (const auto &measurement : allMeasurements) {
                if (std::get<0>(measurement) == batchNum) {
                    batchMeasurements.emplace_back(
                        std::get<1>(measurement),  // condition
                        std::get<2>(measurement),  // timestamp
                        std::get<3>(measurement),  // deformability
                        std::get<4>(measurement)   // area
                    );
                }
            }
            
            // Count images for this batch
            int imageCount = batchImageCounts[batchNum];
            
            // If we know exactly how many images we should have for this batch,
            // select that many images from the full set starting from currentIndex
            if (imageCount > 0 && currentIndex + imageCount <= allImages.size()) {
                batchImages.assign(allImages.begin() + currentIndex, 
                                  allImages.begin() + currentIndex + imageCount);
                // Update currentIndex for the next batch
                currentIndex += imageCount;
            } else {
                // Otherwise, just use an estimated proportion of the images
                // This is a heuristic and might not be accurate for all datasets
                int batchSize = allImages.size() / availableBatches.size();
                int endIdx = std::min((int)allImages.size(), (int)currentIndex + batchSize);
                
                if (currentIndex >= allImages.size()) {
                    // If we've somehow gone past all images, reset to beginning
                    std::cerr << "Warning: Not enough images for batch " << batchNum 
                             << ". Using first available images instead." << std::endl;
                    currentIndex = 0;
                    endIdx = std::min(batchSize, (int)allImages.size());
                }
                
                batchImages.assign(allImages.begin() + currentIndex, allImages.begin() + endIdx);
                // Update currentIndex for the next batch
                currentIndex = endIdx;
            }
            
            std::cout << "Processing " << batchImages.size() << " images for batch " << batchNum << std::endl;
            
            // Process each image in this batch
            for (size_t i = 0; i < batchImages.size(); i++) {
                cv::Mat processedImage(batchImages[i].rows, batchImages[i].cols, CV_8UC1);
                processFrame(batchImages[i], shared, processedImage, mats);
                
                // First try with the current contour detection method
                FilterResult filterResult = filterProcessedImage(processedImage, shared.roi,
                                                               shared.processingConfig, 255);
                
                // Default to current method
                std::string methodUsed = "Current";
                bool isValid = filterResult.isValid;
                double deformability = filterResult.deformability;
                double area = filterResult.area;
                double ringRatio = filterResult.ringRatio;
                
                // If not valid, try with legacy contour detection for older data
                if (!isValid) {
                    FilterResult legacyResult = legacyContourAnalysis(processedImage, shared.roi, 
                                                                    shared.processingConfig);
                    
                    if (legacyResult.isValid) {
                        methodUsed = "Legacy";
                        isValid = true;
                        deformability = legacyResult.deformability;
                        area = legacyResult.area;
                        ringRatio = legacyResult.ringRatio;
                    }
                }
                
                // Get stored metrics if available
                std::string storedCondition = condition;
                long long timestamp = 0;
                double storedDeformability = 0.0;
                double storedArea = 0.0;
                
                if (i < batchMeasurements.size()) {
                    auto [cond, ts, def, ar] = batchMeasurements[i];
                    storedCondition = cond;
                    timestamp = ts;
                    storedDeformability = def;
                    storedArea = ar;
                }

                // Create and save overlay image with processed mask
                if (isValid) {
                    // Create a color version of the original image for overlay
                    cv::Mat overlayImage;
                    cv::cvtColor(batchImages[i], overlayImage, cv::COLOR_GRAY2BGR);
                    
                    // Create a color version of the processed image (red mask)
                    cv::Mat colorMask(processedImage.size(), CV_8UC3, cv::Scalar(0, 0, 0));
                    for (int y = 0; y < processedImage.rows; y++) {
                        for (int x = 0; x < processedImage.cols; x++) {
                            if (processedImage.at<uchar>(y, x) > 0) {
                                // Set to red with 50% transparency
                                colorMask.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
                            }
                        }
                    }
                    
                    // Blend the mask with the original image
                    cv::addWeighted(overlayImage, 0.7, colorMask, 0.3, 0, overlayImage);
                    
                    // Draw ROI rectangle
                    cv::rectangle(overlayImage, shared.roi, cv::Scalar(0, 255, 0), 1);
                    
                    // Add text with metrics
                    std::string metricsText = "Batch: " + std::to_string(batchNum) + 
                                             " | Def: " + std::to_string(deformability) +
                                             " | Area: " + std::to_string(area) +
                                             " | Method: " + methodUsed;
                    cv::putText(overlayImage, metricsText, 
                               cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                               cv::Scalar(0, 255, 255), 1);
                    
                    // Save the overlay image
                    std::filesystem::path overlayPath = overlaysDir / ("batch_" + std::to_string(batchNum) + 
                                                                     "_img_" + std::to_string(i) + ".png");
                    cv::imwrite(overlayPath.string(), overlayImage);
                    
                    // Every 100 images, report how many overlays we've saved
                    if ((i+1) % 100 == 0) {
                        std::cout << "Saved " << (i+1) << " overlay images for batch " << batchNum << std::endl;
                    }
                }
                
                // Write metrics to CSV
                outputFile << batchNum << ","
                          << storedCondition << ","
                          << i << ","
                          << timestamp << ","
                          << deformability << ","
                          << area << ","
                          << ringRatio << ","
                          << (isValid ? "Yes" : "No") << ","
                          << methodUsed << ","
                          << configToString(shared.processingConfig) << "\n";
                
                if ((i+1) % 100 == 0 || i == batchImages.size() - 1) {
                    std::cout << "Processed " << (i+1) << "/" << batchImages.size() << " images from batch " << batchNum << std::endl;
                }
            }
            
            std::cout << "Completed batch " << batchNum << "." << std::endl;
        }
    } else {
        // Process individual batch directories
        std::vector<std::filesystem::path> batchDirs;
        
        std::cout << "Searching for batch directories in: " << absInputDir << std::endl;
        
        for (const auto &entry : std::filesystem::directory_iterator(absInputDir)) {
            if (entry.is_directory() && entry.path().filename().string().find("batch_") != std::string::npos) {
                batchDirs.push_back(entry.path());
                std::cout << "  Found batch directory: " << entry.path().filename().string() << std::endl;
            } else if (entry.is_directory()) {
                std::cout << "  Found directory (not a batch dir): " << entry.path().filename().string() << std::endl;
            }
        }
        
        if (batchDirs.empty()) {
            std::cout << "No batch directories found in " << absInputDir << std::endl;
            std::cout << "Directory contents:" << std::endl;
            for (const auto &entry : std::filesystem::directory_iterator(absInputDir)) {
                std::cout << "  " << entry.path().filename().string() 
                          << (entry.is_directory() ? " (directory)" : " (file)") << std::endl;
            }
            return;
        }
        
        std::sort(batchDirs.begin(), batchDirs.end());
        
        // Process each batch directory
        for (const auto &batchDir : batchDirs) {
            // Extract batch number from directory name
            std::string batchName = batchDir.filename().string();
            int batchNum = -1;
            try {
                size_t pos = batchName.find("batch_");
                if (pos != std::string::npos) {
                    batchNum = std::stoi(batchName.substr(pos + 6));
                }
            } catch (...) {
                // Skip if we can't extract a batch number
                continue;
            }
            
            std::cout << "Processing " << batchDir << " (Batch " << batchNum << ")" << std::endl;
            
            // Load batch configuration
            SharedResources shared;
            try {
                // Load background image
                shared.backgroundFrame = cv::imread((batchDir / "background_clean.tiff").string(), cv::IMREAD_GRAYSCALE);
                if (shared.backgroundFrame.empty()) {
                    throw std::runtime_error("Failed to load background image");
                }
                
                // Read ROI from CSV
                std::ifstream roiFile((batchDir / "roi.csv").string());
                std::string roiHeader;
                std::getline(roiFile, roiHeader); // Skip header
                std::string roiData;
                std::getline(roiFile, roiData);
                std::stringstream ss(roiData);
                std::string value;
                std::vector<int> roiValues;
                while (std::getline(ss, value, ',')) {
                    roiValues.push_back(std::stoi(value));
                }
                
                shared.roi = cv::Rect(roiValues[0], roiValues[1], roiValues[2], roiValues[3]);
                
                // Load processing config
                shared.processingConfig = loadBatchConfig(batchDir);
                
                // Initialize the blurred background with the original config settings
                cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                                cv::Size(shared.processingConfig.gaussian_blur_size,
                                        shared.processingConfig.gaussian_blur_size), 0);

                if (shared.processingConfig.enable_contrast_enhancement) {
                    shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                                     shared.processingConfig.contrast_alpha,
                                                     shared.processingConfig.contrast_beta);
                }
            } catch (const std::exception &e) {
                std::cerr << "Error loading batch " << batchNum << " data: " << e.what() << std::endl;
                continue; // Skip this batch
            }
            
            // Initialize thread mats for processing
            ThreadLocalMats mats = initializeThreadMats(shared.backgroundFrame.rows, 
                                                      shared.backgroundFrame.cols, shared);
            
            // Load CSV data to get timestamps
            std::map<int, long long> timestamps;
            std::map<int, std::string> conditions;
            std::map<int, double> storedDeformabilities;
            std::map<int, double> storedAreas;
            
            std::ifstream csvFile((batchDir / "batch_data.csv").string());
            if (csvFile.is_open()) {
                std::string header;
                std::getline(csvFile, header); // Skip header
                
                // Parse headers to find column indices
                auto headers = parseCSVHeaders(header);
                int conditionIdx = headers.count("Condition") ? headers["Condition"] : -1;
                int timestampIdx = headers.count("Timestamp_us") ? headers["Timestamp_us"] : -1;
                int deformabilityIdx = headers.count("Deformability") ? headers["Deformability"] : -1;
                int areaIdx = headers.count("Area") ? headers["Area"] : -1;
                
                if (timestampIdx >= 0 || deformabilityIdx >= 0 || areaIdx >= 0) {
                    int idx = 0;
                    std::string line;
                    while (std::getline(csvFile, line)) {
                        std::stringstream ss(line);
                        std::string cell;
                        std::vector<std::string> values;
                        
                        while (std::getline(ss, cell, ',')) {
                            values.push_back(cell);
                        }
                        
                        if (conditionIdx >= 0 && values.size() > conditionIdx) {
                            conditions[idx] = values[conditionIdx];
                        }
                        
                        if (timestampIdx >= 0 && values.size() > timestampIdx) {
                            try {
                                timestamps[idx] = std::stoll(values[timestampIdx]);
                            } catch (...) {
                                // Skip invalid entries
                            }
                        }
                        
                        if (deformabilityIdx >= 0 && values.size() > deformabilityIdx) {
                            try {
                                storedDeformabilities[idx] = std::stod(values[deformabilityIdx]);
                            } catch (...) {
                                // Skip invalid entries
                            }
                        }
                        
                        if (areaIdx >= 0 && values.size() > areaIdx) {
                            try {
                                storedAreas[idx] = std::stod(values[areaIdx]);
                            } catch (...) {
                                // Skip invalid entries
                            }
                        }
                        
                        idx++;
                    }
                }
            }
            
            // Load and process images from binary file
            std::ifstream imageFile((batchDir / "images.bin").string(), std::ios::binary);
            int imageIndex = 0;
            
            while (imageFile.good()) {
                int rows, cols, type;
                imageFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
                imageFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
                imageFile.read(reinterpret_cast<char *>(&type), sizeof(int));
                
                if (imageFile.eof()) {
                    break;
                }
                
                cv::Mat image(rows, cols, type);
                imageFile.read(reinterpret_cast<char *>(image.data), rows * cols * image.elemSize());
                
                // Process the image and calculate metrics
                cv::Mat processedImage(rows, cols, CV_8UC1);
                processFrame(image, shared, processedImage, mats);
                
                // First try with the current contour detection method
                FilterResult filterResult = filterProcessedImage(processedImage, shared.roi,
                                                              shared.processingConfig, 255);
                
                // Default to current method
                std::string methodUsed = "Current";
                bool isValid = filterResult.isValid;
                double deformability = filterResult.deformability;
                double area = filterResult.area;
                double ringRatio = filterResult.ringRatio;
                
                // If not valid, try with legacy contour detection for older data
                if (!isValid) {
                    FilterResult legacyResult = legacyContourAnalysis(processedImage, shared.roi, 
                                                                   shared.processingConfig);
                    
                    if (legacyResult.isValid) {
                        methodUsed = "Legacy";
                        isValid = true;
                        deformability = legacyResult.deformability;
                        area = legacyResult.area;
                        ringRatio = legacyResult.ringRatio;
                    }
                }
                
                // Get stored condition for this image if available (default to condition from config)
                std::string storedCondition = condition;
                if (conditions.count(imageIndex)) {
                    storedCondition = conditions[imageIndex];
                }
                
                // Get timestamp for this image if available
                long long timestamp = (timestamps.count(imageIndex)) ? timestamps[imageIndex] : 0;
                
                // Create and save overlay image with processed mask
                if (isValid) {
                    // Create a color version of the original image for overlay
                    cv::Mat overlayImage;
                    cv::cvtColor(image, overlayImage, cv::COLOR_GRAY2BGR);
                    
                    // Create a color version of the processed image (red mask)
                    cv::Mat colorMask(processedImage.size(), CV_8UC3, cv::Scalar(0, 0, 0));
                    for (int y = 0; y < processedImage.rows; y++) {
                        for (int x = 0; x < processedImage.cols; x++) {
                            if (processedImage.at<uchar>(y, x) > 0) {
                                // Set to red with 50% transparency
                                colorMask.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
                            }
                        }
                    }
                    
                    // Blend the mask with the original image
                    cv::addWeighted(overlayImage, 0.7, colorMask, 0.3, 0, overlayImage);
                    
                    // Draw ROI rectangle
                    cv::rectangle(overlayImage, shared.roi, cv::Scalar(0, 255, 0), 1);
                    
                    // Add text with metrics
                    std::string metricsText = "Batch: " + std::to_string(batchNum) + 
                                             " | Def: " + std::to_string(deformability) +
                                             " | Area: " + std::to_string(area) +
                                             " | Method: " + methodUsed;
                    cv::putText(overlayImage, metricsText, 
                               cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                               cv::Scalar(0, 255, 255), 1);
                    
                    // Save the overlay image
                    std::filesystem::path overlayPath = overlaysDir / ("batch_" + std::to_string(batchNum) + 
                                                                     "_img_" + std::to_string(imageIndex) + ".png");
                    cv::imwrite(overlayPath.string(), overlayImage);
                    
                    // Every 100 images, report how many overlays we've saved
                    if ((imageIndex+1) % 100 == 0 || imageIndex == 0) {
                        std::cout << "Saved " << (imageIndex+1) << " overlay images for batch " << batchNum << std::endl;
                    }
                }

                // Write metrics to CSV
                outputFile << batchNum << ","
                          << storedCondition << ","
                          << imageIndex << ","
                          << timestamp << ","
                          << deformability << ","
                          << area << ","
                          << ringRatio << ","
                          << (isValid ? "Yes" : "No") << ","
                          << methodUsed << ","
                          << configToString(shared.processingConfig) << "\n";
                
                imageIndex++;
                if (imageIndex % 100 == 0 || imageIndex == 1) {
                    std::cout << "Processed " << imageIndex << " images from batch " << batchNum << std::endl;
                }
            }
            
            std::cout << "Completed batch " << batchNum << ". Processed " << imageIndex << " images." << std::endl;
        }
    }
    
    outputFile.close();
    std::cout << "Metrics calculation complete. Results saved to: " << outputFilePath << std::endl;
    std::cout << "Overlay images with masks saved to: " << overlaysDir.string() << std::endl;
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

void convertSavedMasksToStandardFormat(const std::string &binaryMaskFile, const std::string &outputDirectory)
{
    std::ifstream maskFile(binaryMaskFile, std::ios::binary);
    std::filesystem::create_directories(outputDirectory);

    int maskCount = 0;
    while (maskFile.good())
    {
        int rows, cols, type;
        maskFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
        maskFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
        maskFile.read(reinterpret_cast<char *>(&type), sizeof(int));

        if (maskFile.eof())
            break;

        cv::Mat mask(rows, cols, type);
        maskFile.read(reinterpret_cast<char *>(mask.data), rows * cols * mask.elemSize());

        std::string outputPath = outputDirectory + "/mask_" + std::to_string(maskCount++) + ".tiff";
        cv::imwrite(outputPath, mask);
    }

    std::cout << "Converted " << maskCount << " masks to TIFF format in " << outputDirectory << std::endl;
}

json readConfig(const std::string &filename)
{
    json config;

    if (!std::filesystem::exists(filename))
    {
        // Create default config with a more structured approach
        json image_processing = {
            {"gaussian_blur_size", 3},
            {"bg_subtract_threshold", 8},
            {"morph_kernel_size", 3},
            {"morph_iterations", 1},
            {"area_threshold_min", 250},
            {"area_threshold_max", 1000},
            {"filters", {{"enable_border_check", true}, {"enable_multiple_contours_check", true}, {"enable_area_range_check", true}, {"require_single_inner_contour", true}}},
            {"contrast_enhancement", {{"enable_contrast", true}, {"alpha", 1.2}, {"beta", 10}}}};

        config = {
            {"save_directory", "updated_results"},
            {"buffer_threshold", 1000},
            {"displayFPS", 100},
            {"cameraTargetFPS", 15000},
            {"simCameraTargetFPS", 15000},
            {"scatter_plot_enabled", false},
            {"histogram_enabled", true},
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

FilterResult legacyContourAnalysis(const cv::Mat &processedImage, const cv::Rect &roi, const ProcessingConfig &config)
{
    // Initialize result with default values
    FilterResult result = {false, false, false, false, 0, 0.0, 0.0, 0.0};
    
    // Find contours using the legacy approach (no hierarchy/nested contour detection)
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processedImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Filter out small noise contours
    std::vector<std::vector<cv::Point>> filteredContours;
    
    // Minimum area threshold to filter out noise
    const double minNoiseArea = 10.0;
    
    for (const auto &contour : contours)
    {
        double area = cv::contourArea(contour);
        if (area >= minNoiseArea)
        {
            filteredContours.push_back(contour);
        }
    }
    
    // Check for border touching (simple version)
    if (config.enable_border_check && !filteredContours.empty())
    {
        const int borderThreshold = 2;
        
        for (const auto &contour : filteredContours)
        {
            for (const auto &point : contour)
            {
                int x = point.x - roi.x;
                int y = point.y - roi.y;
                
                if (x >= 0 && x < roi.width && y >= 0 && y < roi.height)
                {
                    if (x < borderThreshold || x >= roi.width - borderThreshold ||
                        y < borderThreshold || y >= roi.height - borderThreshold)
                    {
                        result.touchesBorder = true;
                        break;
                    }
                }
                else
                {
                    result.touchesBorder = true;
                    break;
                }
            }
            
            if (result.touchesBorder)
                break;
        }
    }
    
    // If not touching border or border check disabled, calculate metrics
    if (!result.touchesBorder || !config.enable_border_check)
    {
        if (!filteredContours.empty())
        {
            // Find the largest contour
            size_t largestIdx = 0;
            double largestArea = 0.0;
            
            for (size_t i = 0; i < filteredContours.size(); i++)
            {
                double area = cv::contourArea(filteredContours[i]);
                if (area > largestArea)
                {
                    largestArea = area;
                    largestIdx = i;
                }
            }
            
            // Calculate metrics using the largest contour
            auto [deformability, area] = calculateMetrics(filteredContours[largestIdx]);
            result.deformability = deformability;
            result.area = area;
            
            // Check area range if needed
            if (!config.enable_area_range_check ||
                (area >= config.area_threshold_min && area <= config.area_threshold_max))
            {
                result.inRange = true;
                result.isValid = true;
            }
        }
    }
    
    return result;
}

// Update the formatProcessingConfig function to include background information
std::string formatProcessingConfig(const ProcessingConfig &config) {
    std::stringstream ss;
    ss << "Processing Config:" << std::endl
       << "  Gaussian Blur: " << config.gaussian_blur_size << std::endl
       << "  BG Subtract Threshold: " << config.bg_subtract_threshold << std::endl
       << "  Morph Kernel Size: " << config.morph_kernel_size << std::endl
       << "  Morph Iterations: " << config.morph_iterations << std::endl
       << "  Area Range: " << config.area_threshold_min << " - " << config.area_threshold_max << std::endl
       << "  Checks: Border=" << (config.enable_border_check ? "On" : "Off")
       << ", Area=" << (config.enable_area_range_check ? "On" : "Off")
       << ", SingleInner=" << (config.require_single_inner_contour ? "On" : "Off") << std::endl
       << "  Contrast: " << (config.enable_contrast_enhancement ? "On" : "Off")
       << " (alpha=" << config.contrast_alpha << ", beta=" << config.contrast_beta << ")";
    return ss.str();
}

// Update the function to properly handle the background with the batch's original config
void updateBackgroundForReview(SharedResources &shared) {
    if (shared.backgroundFrame.empty()) {
        return; // No background to update
    }

    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
    
    // Apply Gaussian blur to the original background frame with batch-specific config
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground,
                    cv::Size(shared.processingConfig.gaussian_blur_size,
                             shared.processingConfig.gaussian_blur_size),
                    0);

    // Apply contrast enhancement if it was enabled during recording
    if (shared.processingConfig.enable_contrast_enhancement) {
        // Apply alpha (contrast) and beta (brightness) adjustments
        shared.blurredBackground.convertTo(shared.blurredBackground, -1,
                                         shared.processingConfig.contrast_alpha,
                                         shared.processingConfig.contrast_beta);
    }
}

// Function to display keyboard controls
void displayKeyboardInstructions() {
    std::cout << "\n--------- KEYBOARD CONTROLS ---------" << std::endl;
    std::cout << "ESC: Exit review mode" << std::endl;
    std::cout << "SPACE: Toggle processing overlay" << std::endl;
    std::cout << "r: Toggle recalculated metrics display" << std::endl;
    std::cout << "c: Toggle configuration display" << std::endl;
    std::cout << "a: Previous image" << std::endl;
    std::cout << "d: Next image" << std::endl;
    std::cout << "-----------------------------------\n" << std::endl;
}

// Helper function to parse CSV headers and get column indices
std::map<std::string, int> parseCSVHeaders(const std::string& headerLine) {
    std::map<std::string, int> headerMap;
    std::stringstream ss(headerLine);
    std::string header;
    int index = 0;
    
    while (std::getline(ss, header, ',')) {
        // Trim whitespace
        header.erase(0, header.find_first_not_of(" \t"));
        header.erase(header.find_last_not_of(" \t") + 1);
        headerMap[header] = index++;
    }
    
    return headerMap;
}

void reviewSavedData()
{
    std::string projectPath = MenuSystem::navigateAndSelectFolder();
    std::vector<std::filesystem::path> batchDirs;
    ProcessingConfig processingConfig;

    // Auto-detect the file prefix from the directory contents
    std::string condition = "";
    for (const auto& entry : std::filesystem::directory_iterator(projectPath)) {
        std::string filename = entry.path().filename().string();
        if (filename.size() > 16 && filename.substr(filename.size() - 16) == "_backgrounds.bin") {
            condition = filename.substr(0, filename.size() - 16);
            std::cout << "Auto-detected file prefix: " << condition << std::endl;
            break;
        }
    }
    
    if (condition.empty()) {
        // Try alternative detection with other common file extensions
        for (const auto& suffix : {"_processing_config.json", "_roi.csv", "_images.bin", "_data.csv"}) {
            for (const auto& entry : std::filesystem::directory_iterator(projectPath)) {
                std::string filename = entry.path().filename().string();
                if (filename.size() > strlen(suffix) && 
                    filename.substr(filename.size() - strlen(suffix)) == suffix) {
                    condition = filename.substr(0, filename.size() - strlen(suffix));
                    std::cout << "Auto-detected file prefix from " << suffix << ": " << condition << std::endl;
                    goto prefix_found_review;  // Break out of nested loops
                }
            }
        }
    }
    
prefix_found_review:
    if (condition.empty()) {
        std::cerr << "Could not auto-detect file prefix. No data files found." << std::endl;
        return;
    }

    // Check if this is a consolidated dataset with master files
    bool hasMasterFiles = false;
    std::string masterConfigPath = projectPath + "/" + condition + "_processing_config.json";
    std::string masterROIPath = projectPath + "/" + condition + "_roi.csv";
    std::string masterBackgroundsPath = projectPath + "/" + condition + "_backgrounds.bin";
    std::string masterImagesPath = projectPath + "/" + condition + "_images.bin";
    std::string masterDataPath = projectPath + "/" + condition + "_data.csv";
    
    // Check if master files exist
    if (std::filesystem::exists(masterConfigPath) && 
        std::filesystem::exists(masterROIPath) && 
        std::filesystem::exists(masterBackgroundsPath) &&
        std::filesystem::exists(masterDataPath) &&
        std::filesystem::exists(masterImagesPath)) {
        hasMasterFiles = true;
        std::cout << "Found consolidated master files in this directory. Using them for data review." << std::endl;
    }

    auto loadMasterConfig = [](const std::string &configPath, int batchNum) -> ProcessingConfig {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to load master processing config from: " + configPath);
        }
        json masterConfig;
        configFile >> masterConfig;
        
        // Get the config for the specified batch
        std::string batchKey = "batch_" + std::to_string(batchNum);
        if (!masterConfig.contains(batchKey)) {
            throw std::runtime_error("Master config file does not contain configuration for batch " + std::to_string(batchNum));
        }
        
        json config = masterConfig[batchKey];

        return ProcessingConfig{
            config["gaussian_blur_size"],
            config["bg_subtract_threshold"],
            config["morph_kernel_size"],
            config["morph_iterations"],
            config["area_threshold_min"],
            config["area_threshold_max"],
            config.contains("filters") && config["filters"].contains("enable_border_check") ? 
                config["filters"]["enable_border_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_multiple_contours_check") ? 
                config["filters"]["enable_multiple_contours_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_area_range_check") ? 
                config["filters"]["enable_area_range_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("require_single_inner_contour") ? 
                config["filters"]["require_single_inner_contour"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("enable_contrast") ? 
                config["contrast_enhancement"]["enable_contrast"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("alpha") ? 
                config["contrast_enhancement"]["alpha"].get<double>() : 1.2,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("beta") ? 
                config["contrast_enhancement"]["beta"].get<int>() : 10
        };
    };

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
            config["morph_iterations"],
            config["area_threshold_min"],
            config["area_threshold_max"],
            config.contains("filters") && config["filters"].contains("enable_border_check") ? 
                config["filters"]["enable_border_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_multiple_contours_check") ? 
                config["filters"]["enable_multiple_contours_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("enable_area_range_check") ? 
                config["filters"]["enable_area_range_check"].get<bool>() : true,
            config.contains("filters") && config["filters"].contains("require_single_inner_contour") ? 
                config["filters"]["require_single_inner_contour"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("enable_contrast") ? 
                config["contrast_enhancement"]["enable_contrast"].get<bool>() : true,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("alpha") ? 
                config["contrast_enhancement"]["alpha"].get<double>() : 1.2,
            config.contains("contrast_enhancement") && config["contrast_enhancement"].contains("beta") ? 
                config["contrast_enhancement"]["beta"].get<int>() : 10
        };
    };

    // Function to load ROI for a specific batch from master ROI CSV
    auto loadROIFromMasterCSV = [](const std::string &roiPath, int batchNum) -> cv::Rect {
        std::ifstream roiFile(roiPath);
        if (!roiFile.is_open()) {
            throw std::runtime_error("Failed to open master ROI file: " + roiPath);
        }
        
        std::string header;
        std::getline(roiFile, header); // Skip header
        
        std::string line;
        while (std::getline(roiFile, line)) {
            std::stringstream ss(line);
            std::string value;
            std::vector<std::string> values;
            
            while (std::getline(ss, value, ',')) {
                values.push_back(value);
            }
            
            if (values.size() >= 5 && std::stoi(values[0]) == batchNum) {
                return cv::Rect(
                    std::stoi(values[1]), // x
                    std::stoi(values[2]), // y
                    std::stoi(values[3]), // width
                    std::stoi(values[4])  // height
                );
            }
        }
        
        throw std::runtime_error("Failed to find ROI for batch " + std::to_string(batchNum) + " in master ROI file");
    };
    
    // Function to load background for a specific batch from master backgrounds.bin
    auto loadBackgroundFromMasterBin = [](const std::string &bgPath, int batchNum) -> cv::Mat {
        std::ifstream bgFile(bgPath, std::ios::binary);
        if (!bgFile.is_open()) {
            throw std::runtime_error("Failed to open master backgrounds file: " + bgPath);
        }
        
        while (bgFile.good()) {
            int storedBatchNum, rows, cols, type;
            bgFile.read(reinterpret_cast<char *>(&storedBatchNum), sizeof(int));
            
            if (bgFile.eof()) {
                break;
            }
            
            bgFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
            bgFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
            bgFile.read(reinterpret_cast<char *>(&type), sizeof(int));
            
            cv::Mat background(rows, cols, type);
            bgFile.read(reinterpret_cast<char *>(background.data), rows * cols * background.elemSize());
            
            if (storedBatchNum == batchNum) {
                return background.clone(); // Found the background for this batch
            }
        }
        
        throw std::runtime_error("Failed to find background for batch " + std::to_string(batchNum) + " in master backgrounds file");
    };

    // If we have master files, use them to initialize shared resources
    SharedResources shared;
    cv::Mat backgroundClean;
    
    if (hasMasterFiles) {
        // Process all images from the master images file
        std::vector<cv::Mat> allImages;
        std::vector<std::tuple<int, std::string, long long, double, double>> allMeasurements;
        
        // Load all measurements from the master CSV
        std::ifstream csvFile(masterDataPath);
        std::string headerLine;
        std::getline(csvFile, headerLine); // Get header line
        
        // Parse headers to find column indices
        auto headers = parseCSVHeaders(headerLine);
        
        // Debug header information
        std::cout << "CSV Headers: " << headerLine << std::endl;
        std::cout << "Parsed header mapping: ";
        for (const auto& [name, index] : headers) {
            std::cout << name << "=" << index << " ";
        }
        std::cout << std::endl;
        
        // Check for required columns
        if (!headers.count("Batch") || !headers.count("Timestamp_us") || 
            !headers.count("Deformability") || !headers.count("Area")) {
            std::cerr << "Error: Missing required columns in CSV. Expected: Batch, Timestamp_us, Deformability, Area" << std::endl;
            return;
        }
        
        // Get column indices
        int batchIdx = headers["Batch"];
        int conditionIdx = headers.count("Condition") ? headers["Condition"] : -1;
        int timestampIdx = headers["Timestamp_us"];
        int deformabilityIdx = headers["Deformability"];
        int areaIdx = headers["Area"];
        
        std::string line;
        while (std::getline(csvFile, line)) {
            std::stringstream lineStream(line);
            std::string cell;
            std::vector<std::string> values;
            
            while (std::getline(lineStream, cell, ',')) {
                values.push_back(cell);
            }
            
            if (values.size() > std::max({batchIdx, conditionIdx, timestampIdx, deformabilityIdx, areaIdx})) {
                try {
                    std::string condition = (conditionIdx >= 0) ? values[conditionIdx] : "unknown";
                    
                    allMeasurements.emplace_back(
                        std::stoi(values[batchIdx]),
                        condition,
                        std::stoll(values[timestampIdx]),
                        std::stod(values[deformabilityIdx]),
                        std::stod(values[areaIdx])
                    );
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
                }
            }
        }
        
        // Load all images from the master images binary file
        std::ifstream imageFile(masterImagesPath, std::ios::binary);
        while (imageFile.good()) {
            int rows, cols, type;
            imageFile.read(reinterpret_cast<char *>(&rows), sizeof(int));
            imageFile.read(reinterpret_cast<char *>(&cols), sizeof(int));
            imageFile.read(reinterpret_cast<char *>(&type), sizeof(int));
            
            if (imageFile.eof()) {
                break;
            }
            
            cv::Mat image(rows, cols, type);
            imageFile.read(reinterpret_cast<char *>(image.data), rows * cols * image.elemSize());
            allImages.push_back(image.clone());
        }
        
        // Allow user to select which batch to review
        std::set<int> availableBatches;
        for (const auto &measurement : allMeasurements) {
            availableBatches.insert(std::get<0>(measurement));
        }
        
        std::cout << "Available batches: ";
        for (int batch : availableBatches) {
            std::cout << batch << " ";
        }
        std::cout << std::endl;
        std::cout << "Enter batch number to review (or -1 for all): ";
        int selectedBatch;
        std::cin >> selectedBatch;
        
        std::vector<cv::Mat> filteredImages;
        std::vector<std::tuple<int, std::string, long long, double, double>> filteredMeasurements;
        
        if (selectedBatch >= 0) {
            // Filter images and measurements by batch
            for (size_t i = 0; i < allMeasurements.size() && i < allImages.size(); ++i) {
                if (std::get<0>(allMeasurements[i]) == selectedBatch) {
                    filteredImages.push_back(allImages[i]);
                    filteredMeasurements.push_back(allMeasurements[i]);
                }
            }
            
            // Load batch-specific background, ROI, and config
            try {
                backgroundClean = loadBackgroundFromMasterBin(masterBackgroundsPath, selectedBatch);
                shared.roi = loadROIFromMasterCSV(masterROIPath, selectedBatch);
                processingConfig = loadMasterConfig(masterConfigPath, selectedBatch);
                shared.processingConfig = processingConfig;
                
                // Initialize background with original configuration
                shared.backgroundFrame = backgroundClean.clone();
                updateBackgroundForReview(shared);
            } catch (const std::exception &e) {
                std::cerr << "Error loading batch-specific data: " << e.what() << std::endl;
                return;
            }
        } else {
            // Use all images and measurements
            filteredImages = allImages;
            filteredMeasurements = allMeasurements;
            
            // Load background, ROI, and config from the first batch
            if (!availableBatches.empty()) {
                int firstBatch = *availableBatches.begin();
                try {
                    backgroundClean = loadBackgroundFromMasterBin(masterBackgroundsPath, firstBatch);
                    shared.roi = loadROIFromMasterCSV(masterROIPath, firstBatch);
                    processingConfig = loadMasterConfig(masterConfigPath, firstBatch);
                    shared.processingConfig = processingConfig;
                    
                    // Initialize background with original configuration
                    shared.backgroundFrame = backgroundClean.clone();
                    updateBackgroundForReview(shared);
                } catch (const std::exception &e) {
                    std::cerr << "Error loading batch-specific data: " << e.what() << std::endl;
                    return;
                }
            } else {
                std::cerr << "No batches found in the master files" << std::endl;
                return;
            }
        }
        
        // Initialize thread mats for processing
        ThreadLocalMats mats = initializeThreadMats(backgroundClean.rows, backgroundClean.cols, shared);
        
        // Create display window
        cv::namedWindow("Data Review", cv::WINDOW_NORMAL);
        cv::resizeWindow("Data Review", backgroundClean.cols, backgroundClean.rows);
        
        // Display keyboard instructions
        displayKeyboardInstructions();
        
        // Review logic for consolidated data
        size_t currentImageIndex = 0;
        bool showProcessed = false;
        bool showRecalculated = false;
        bool showConfig = false;
        bool running = true;
        
        while (running && currentImageIndex < filteredImages.size()) {
            // Create display image
            cv::Mat displayImage;
            cv::cvtColor(filteredImages[currentImageIndex], displayImage, cv::COLOR_GRAY2BGR);
            cv::Mat processedImage;
            double recalcDeformability = 0.0;
            double recalcArea = 0.0;
            bool recalcValid = false;
            
            // Process the current image
            processedImage = cv::Mat(filteredImages[currentImageIndex].rows,
                                    filteredImages[currentImageIndex].cols,
                                    CV_8UC1);
            processFrame(filteredImages[currentImageIndex], shared, processedImage, mats);
            
            // First try with the current contour detection method
            FilterResult filterResult = filterProcessedImage(processedImage, shared.roi, 
                                                            shared.processingConfig, 255, 
                                                            filteredImages[currentImageIndex]);
            recalcDeformability = filterResult.deformability;
            recalcArea = filterResult.area;
            recalcValid = filterResult.isValid;
            
            // Default to current method
            std::string methodUsed = "C"; // C for Current method
            
            // If not valid, try with legacy contour detection for older data
            if (!recalcValid) {
                FilterResult legacyResult = legacyContourAnalysis(processedImage, shared.roi, shared.processingConfig);
                
                if (legacyResult.isValid) {
                    recalcDeformability = legacyResult.deformability;
                    recalcArea = legacyResult.area;
                    recalcValid = true;
                    methodUsed = "L"; // L for Legacy method
                }
            }
            
            if (showProcessed) {
                cv::Mat processedOverlay;
                cv::cvtColor(processedImage, processedOverlay, cv::COLOR_GRAY2BGR);
                cv::addWeighted(displayImage, 0.7, processedOverlay, 0.3, 0, displayImage);
            }
            
            // Draw ROI rectangle
            cv::rectangle(displayImage, shared.roi, cv::Scalar(0, 255, 0), 2);
            
            cv::imshow("Data Review", displayImage);
            
            // Print current image information with comparison to recalculated values
            if (currentImageIndex < filteredMeasurements.size()) {
                auto [batchNum, condition, timestamp, storedDeformability, storedArea] = filteredMeasurements[currentImageIndex];
                
                double deformDiff = recalcDeformability - storedDeformability;
                double areaDiff = recalcArea - storedArea;
                
                std::cout << "\rBatch: " << batchNum 
                          << " | Frame: " << currentImageIndex << "/" << (filteredImages.size() - 1);
                
                if (showRecalculated) {
                    std::cout << " | Stored Def: " << std::fixed << std::setprecision(4) << storedDeformability
                              << " | Recalc Def(" << methodUsed << "): " << std::fixed << std::setprecision(4) << recalcDeformability
                              << " | Diff: " << std::fixed << std::setprecision(4) << deformDiff
                              << " | Stored Area: " << std::fixed << std::setprecision(1) << storedArea
                              << " | Recalc Area(" << methodUsed << "): " << std::fixed << std::setprecision(1) << recalcArea
                              << " | Diff: " << std::fixed << std::setprecision(1) << areaDiff
                              << " | Valid: " << (recalcValid ? "Yes" : "No") << "          ";
                } else {
                    std::cout << " | Deformability: " << std::fixed << std::setprecision(4) << storedDeformability
                              << " | Area: " << std::fixed << std::setprecision(1) << storedArea << "                    ";
                }
                
                std::cout << std::flush;
            }
            
            // Handle keyboard input
            int key = cv::waitKey(0);
            switch (key) {
                case 27: // ESC
                    running = false;
                    break;
                case ' ': // Spacebar - toggle processing overlay
                    showProcessed = !showProcessed;
                    break;
                case 'r': // Toggle recalculated values display
                    showRecalculated = !showRecalculated;
                    break;
                case 'c': // Toggle config display
                    showConfig = !showConfig;
                    if (showConfig) {
                        // Clear current line and display config
                        std::cout << "\r" << std::string(120, ' ') << std::endl;
                        std::cout << formatProcessingConfig(shared.processingConfig) << std::endl;
                    }
                    break;
                case 'a': // Previous image
                    if (currentImageIndex > 0)
                        currentImageIndex--;
                    break;
                case 'd': // Next image
                    if (currentImageIndex < filteredImages.size() - 1)
                        currentImageIndex++;
                    break;
            }
        }
        
        cv::destroyAllWindows();
        return;
    }

    // Original code for batch-by-batch review if not using master files
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

        // Initialize shared resources with batch's original settings
        shared.roi = cv::Rect(roiValues[0], roiValues[1], roiValues[2], roiValues[3]);
        shared.backgroundFrame = backgroundClean.clone();
        updateBackgroundForReview(shared);

        return backgroundClean;
    };

    std::sort(batchDirs.begin(), batchDirs.end());
    size_t currentBatchIndex = 0;
    size_t currentImageIndex = 0;
    bool showProcessed = true;
    bool showRecalculated = false;
    bool showConfig = true;

    // Initialize resources
    backgroundClean = initializeBatch(batchDirs[currentBatchIndex], shared);
    ThreadLocalMats mats = initializeThreadMats(backgroundClean.rows, backgroundClean.cols, shared);

    // Create display window
    cv::namedWindow("Data Review", cv::WINDOW_NORMAL);
    cv::resizeWindow("Data Review", backgroundClean.cols, backgroundClean.rows);
    
    // Display keyboard instructions
    displayKeyboardInstructions();

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
        std::string headerLine;
        std::getline(csvFile, headerLine); // Get header line
        
        // Parse headers to find column indices
        auto headers = parseCSVHeaders(headerLine);
        
        // Debug header information
        std::cout << "Batch CSV Headers: " << headerLine << std::endl;
        
        // Check for required columns and get indices
        if (!headers.count("Timestamp_us") || !headers.count("Deformability") || !headers.count("Area")) {
            std::cerr << "Error: Missing required columns in batch CSV. Expected: Timestamp_us, Deformability, Area" << std::endl;
            continue; // Skip this batch
        }
        
        int conditionIdx = headers.count("Condition") ? headers["Condition"] : -1;
        int timestampIdx = headers["Timestamp_us"];
        int deformabilityIdx = headers["Deformability"];
        int areaIdx = headers["Area"];

        std::vector<std::tuple<long long, double, double>> measurements; // timestamp, deformability, area

        std::string line;
        while (std::getline(csvFile, line))
        {
            std::stringstream ss(line);
            std::string value;
            std::vector<std::string> values;

            while (std::getline(ss, value, ','))
            {
                values.push_back(value);
            }

            if (values.size() > std::max({timestampIdx, deformabilityIdx, areaIdx}))
            {
                try {
                    measurements.emplace_back(
                        std::stoll(values[timestampIdx]),
                        std::stod(values[deformabilityIdx]),
                        std::stod(values[areaIdx])
                    );
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing batch data line: " << line << " - " << e.what() << std::endl;
                }
            }
        }

        while (currentImageIndex < images.size() && running)
        {
            // Create display image
            cv::Mat displayImage;
            cv::cvtColor(images[currentImageIndex], displayImage, cv::COLOR_GRAY2BGR);
            cv::Mat processedImage;
            double recalcDeformability = 0.0;
            double recalcArea = 0.0;
            bool recalcValid = false;
            
            // Process the current image
            processedImage = cv::Mat(images[currentImageIndex].rows,
                                    images[currentImageIndex].cols,
                                    CV_8UC1);
            processFrame(images[currentImageIndex], shared, processedImage, mats);
            
            // First try with the current contour detection method
            FilterResult filterResult = filterProcessedImage(processedImage, shared.roi, 
                                                           shared.processingConfig, 255, 
                                                           images[currentImageIndex]);
            recalcDeformability = filterResult.deformability;
            recalcArea = filterResult.area;
            recalcValid = filterResult.isValid;
            
            // Default to current method
            std::string methodUsed = "C"; // C for Current method
            
            // If not valid, try with legacy contour detection for older data
            if (!recalcValid) {
                FilterResult legacyResult = legacyContourAnalysis(processedImage, shared.roi, 
                                                                   shared.processingConfig);
                
                if (legacyResult.isValid) {
                    methodUsed = "Legacy";
                    recalcValid = true;
                    recalcDeformability = legacyResult.deformability;
                    recalcArea = legacyResult.area;
                }
            }

            if (showProcessed)
            {
                cv::Mat processedOverlay;
                cv::cvtColor(processedImage, processedOverlay, cv::COLOR_GRAY2BGR);
                cv::addWeighted(displayImage, 0.7, processedOverlay, 0.3, 0, displayImage);
            }

            // Draw ROI rectangle
            cv::rectangle(displayImage, shared.roi, cv::Scalar(0, 255, 0), 2);

            cv::imshow("Data Review", displayImage);
            
            // Print current image information with comparison to recalculated values
            if (currentImageIndex < measurements.size())
            {
                auto [timestamp, storedDeformability, storedArea] = measurements[currentImageIndex];
                
                double deformDiff = recalcDeformability - storedDeformability;
                double areaDiff = recalcArea - storedArea;
                
                std::cout << "\rBatch: " << currentBatchIndex 
                          << " | Frame: " << currentImageIndex << "/" << (images.size() - 1);
                
                if (showRecalculated) {
                    std::cout << " | Stored Def: " << std::fixed << std::setprecision(4) << storedDeformability
                              << " | Recalc Def(" << methodUsed << "): " << std::fixed << std::setprecision(4) << recalcDeformability
                              << " | Diff: " << std::fixed << std::setprecision(4) << deformDiff
                              << " | Stored Area: " << std::fixed << std::setprecision(1) << storedArea
                              << " | Recalc Area(" << methodUsed << "): " << std::fixed << std::setprecision(1) << recalcArea
                              << " | Diff: " << std::fixed << std::setprecision(1) << areaDiff
                              << " | Valid: " << (recalcValid ? "Yes" : "No") << "          ";
                } else {
                    std::cout << " | Deformability: " << std::fixed << std::setprecision(4) << storedDeformability
                              << " | Area: " << std::fixed << std::setprecision(1) << storedArea << "                    ";
                }
                
                std::cout << std::flush;
            }

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
            case 'r': // Toggle recalculated values display
                showRecalculated = !showRecalculated;
                break;
            case 'c': // Toggle config display
                showConfig = !showConfig;
                if (showConfig) {
                    // Clear current line and display config
                    std::cout << "\r" << std::string(120, ' ') << std::endl;
                    std::cout << formatProcessingConfig(shared.processingConfig) << std::endl;
                }
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
                    
                    // Show the new batch's config
                    std::cout << "\r" << std::string(120, ' ') << std::endl;
                    std::cout << "Loaded batch " << currentBatchIndex << " with config:" << std::endl;
                    std::cout << formatProcessingConfig(shared.processingConfig) << std::endl;
                    
                    // Display keyboard instructions after switching batches
                    displayKeyboardInstructions();
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
                    
                    // Show the new batch's config
                    std::cout << "\r" << std::string(120, ' ') << std::endl;
                    std::cout << "Loaded batch " << currentBatchIndex << " with config:" << std::endl;
                    std::cout << formatProcessingConfig(shared.processingConfig) << std::endl;
                    
                    // Display keyboard instructions after switching batches
                    displayKeyboardInstructions();
                    break;
                }
                break;
            }

            // If batch changed, break inner loop to load new batch data
            if (key == 'q' || key == 'e')
            {
                // Display keyboard instructions after switching batches
                displayKeyboardInstructions();
                break;
            }
        }
    }

    cv::destroyAllWindows();
}

std::string autoDetectPrefix(const std::string& dir) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string fname = entry.path().filename().string();
        if (fname.size() > 16 && fname.substr(fname.size() - 16) == "_backgrounds.bin") {
            return fname.substr(0, fname.size() - 16);
        }
    }
    return "";
}
