#include <iostream>
#include <string>
#include <limits>
#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "menu_system/menu_system.h"
#include <EGrabber.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <filesystem>
#include "mib_grabber/mib_grabber.h"

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
        std::cout << "3. Run Hybrid Sample\n";
        std::cout << "4. Convert Saved Images\n";
        std::cout << "5. EGrabber Config\n";
        std::cout << "6. EGrabber Hot Reload\n";
        std::cout << "7. Exit\n";
        std::cout << "Enter your choice: ";
    }

    std::string navigateAndSelectFolder()
    {
        namespace fs = std::filesystem;
        fs::path currentPath = fs::current_path();

        auto screen = ftxui::ScreenInteractive::TerminalOutput();
        std::string selectedPath;

        auto updateEntries = [&]()
        {
            std::vector<std::string> entries;
            for (const auto &entry : fs::directory_iterator(currentPath))
            {
                if (entry.is_directory())
                {
                    entries.push_back(entry.path().filename().string());
                }
            }
            return entries;
        };

        std::vector<std::string> entries = updateEntries();
        int selected = 0;

        auto menu = ftxui::Menu(&entries, &selected);

        auto renderer = ftxui::Renderer(menu, [&]
                                        {
        std::string parentDir = currentPath.parent_path().string();
        std::string currentDir = currentPath.string();
        
        return ftxui::vbox({
            ftxui::hbox({
                ftxui::vbox({
                    ftxui::text("Parent:"),
                    ftxui::text(parentDir) | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 30)
                }) | ftxui::border,
                ftxui::vbox({
                    ftxui::text("Current:"),
                    ftxui::text(currentDir),
                    ftxui::separator(),
                    menu->Render()
                }) | ftxui::flex | ftxui::border,
            }),
            ftxui::text("Press ESC to return to main menu") | ftxui::color(ftxui::Color::GrayDark),
        }); });

        auto component = ftxui::CatchEvent(renderer, [&](ftxui::Event event)
                                           {
        if (event == ftxui::Event::Escape) {
            selectedPath = "";
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == ftxui::Event::Return) {
            selectedPath = (currentPath / entries[selected]).string();
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == ftxui::Event::ArrowLeft) {
            currentPath = currentPath.parent_path();
            entries = updateEntries();
            selected = 0;
            return true;
        }
        if (event == ftxui::Event::ArrowRight && !entries.empty()) {
            currentPath /= entries[selected];
            entries = updateEntries();
            selected = 0;
            return true;
        }
        return false; });

        screen.Loop(component);

        return selectedPath;
    }

    std::string navigateAndSelectFile()
    {
        namespace fs = std::filesystem;
        fs::path currentPath = fs::current_path();

        auto screen = ftxui::ScreenInteractive::TerminalOutput();
        std::string selectedFile;

        auto updateEntries = [&]()
        {
            std::vector<std::string> entries;
            entries.push_back(".."); // Add option to go up a directory
            for (const auto &entry : fs::directory_iterator(currentPath))
            {
                if (entry.is_directory())
                {
                    entries.push_back(entry.path().filename().string() + "/");
                }
                else
                {
                    entries.push_back(entry.path().filename().string());
                }
            }
            return entries;
        };

        std::vector<std::string> entries = updateEntries();
        int selected = 0;

        auto menu = ftxui::Menu(&entries, &selected);

        auto renderer = ftxui::Renderer(menu, [&]
                                        {
        std::string parentDir = currentPath.parent_path().string();
        std::string currentDir = currentPath.string();
        
        return ftxui::vbox({
            ftxui::hbox({
                ftxui::vbox({
                    ftxui::text("Parent:"),
                    ftxui::text(parentDir) | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 30)
                }) | ftxui::border,
                ftxui::vbox({
                    ftxui::text("Current:"),
                    ftxui::text(currentDir),
                    ftxui::separator(),
                    menu->Render()
                }) | ftxui::flex | ftxui::border,
            }),
            ftxui::text("Press ESC to return to main menu") | ftxui::color(ftxui::Color::GrayDark),
        }); });

        auto component = ftxui::CatchEvent(renderer, [&](ftxui::Event event)
                                           {
        if (event == ftxui::Event::Escape) {
            selectedFile = "";
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == ftxui::Event::Return) {
            if (selected == 0) {  // ".." option
                currentPath = currentPath.parent_path();
                entries = updateEntries();
                selected = 0;
            } else {
                fs::path selectedPath = currentPath / entries[selected];
                if (fs::is_directory(selectedPath)) {
                    currentPath = selectedPath;
                    entries = updateEntries();
                    selected = 0;
                } else {
                    selectedFile = selectedPath.string();
                    screen.ExitLoopClosure()();
                }
            }
            return true;
        }
        return false; });

        screen.Loop(component);

        return selectedFile;
    }

    void egrabberConfig()
    {
        std::string configPath = navigateAndSelectFile();
        configure_js(configPath);
    }

    void egrabberHotReload()
    {
        configure_js("egrabberConfig.js");
    }

    void runMockSample()
    {

        std::cout << "Select the image directory:\n";
        std::string imageDirectory = navigateAndSelectFolder();

        try
        {
            ImageParams params = initializeImageParams(imageDirectory);
            CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);
            CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
            CircularBuffer processingBuffer(params.bufferCount, params.imageSize);
            loadImages(imageDirectory, cameraBuffer, true);

            SharedResources shared;
            initializeMockBackgroundFrame(shared, params, cameraBuffer);
            shared.roi = cv::Rect(0, 0, static_cast<int>(params.width), static_cast<int>(params.height));

            temp_mockSample(params, cameraBuffer, circularBuffer, processingBuffer, shared);

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
        try
        {
            mib_grabber_main();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    void convertSavedImages()
    {
        std::string saveDirectory = navigateAndSelectFolder();

        // std::cout << "Enter the path to the save directory containing batch folders: ";
        // std::getline(std::cin, saveDirectory);

        try
        {
            processAllBatches(saveDirectory);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void processAllBatches(const std::string &saveDirectory)
    {
        namespace fs = std::filesystem;

        // First check for master files in the save directory
        std::string condition;
        try {
            json config = readConfig("config.json");
            condition = config["save_directory"];
        } catch (const std::exception &e) {
            std::cerr << "Error reading config: " << e.what() << std::endl;
            condition = "results"; // Default condition name
        }
        
        std::string masterImagesPath = saveDirectory + "/" + condition + "_images.bin";
        std::string masterMasksPath = saveDirectory + "/" + condition + "_masks.bin";
        
        // Process master images if they exist
        if (fs::exists(masterImagesPath))
        {
            std::string masterImagesDir = saveDirectory + "/master_images";
            std::cout << "Processing master images: " << masterImagesPath << std::endl;
            try
            {
                convertSavedImagesToStandardFormat(masterImagesPath, masterImagesDir);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing " << masterImagesPath << ": " << e.what() << std::endl;
            }
        }
        
        // Process master masks if they exist
        if (fs::exists(masterMasksPath))
        {
            std::string masterMasksDir = saveDirectory + "/master_masks";
            std::cout << "Processing master masks: " << masterMasksPath << std::endl;
            try
            {
                convertSavedMasksToStandardFormat(masterMasksPath, masterMasksDir);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing " << masterMasksPath << ": " << e.what() << std::endl;
            }
        }

        // Process individual batch folders
        for (const auto &entry : fs::directory_iterator(saveDirectory))
        {
            if (entry.is_directory() && entry.path().filename().string().find("batch_") == 0)
            {
                std::string batchPath = entry.path().string();
                std::string imagesBinPath = batchPath + "/images.bin";
                std::string masksBinPath = batchPath + "/masks.bin";

                // Process images if they exist
                if (fs::exists(imagesBinPath))
                {
                    std::cout << "Processing images: " << imagesBinPath << std::endl;
                    try
                    {
                        convertSavedImagesToStandardFormat(imagesBinPath, batchPath);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error processing " << imagesBinPath << ": " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cout << "Skipping " << batchPath << ": images.bin not found" << std::endl;
                }
                
                // Process masks if they exist
                if (fs::exists(masksBinPath))
                {
                    std::cout << "Processing masks: " << masksBinPath << std::endl;
                    try
                    {
                        convertSavedMasksToStandardFormat(masksBinPath, batchPath);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error processing " << masksBinPath << ": " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cout << "Skipping " << batchPath << ": masks.bin not found" << std::endl;
                }
            }
        }

        std::cout << "Finished processing all batches." << std::endl;
    }

    void calculateMetrics()
    {
        std::cout << "Select the project directory containing batches:\n";
        std::string inputDirectory = navigateAndSelectFolder();
        
        if (inputDirectory.empty()) {
            std::cout << "Operation cancelled.\n";
            return;
        }
        
        std::cout << "Enter the output CSV filename (will be saved in the selected project directory): ";
        std::string outputFilename;
        std::getline(std::cin, outputFilename);
        
        if (outputFilename.empty()) {
            outputFilename = "metrics_output.csv";
            std::cout << "Using default filename: " << outputFilename << std::endl;
        }
        
        // Combine the input directory with the output filename to save in the project directory
        std::string fullOutputPath = inputDirectory + "/" + outputFilename;
        std::cout << "Output will be saved to: " << fullOutputPath << std::endl;
        
        try {
            calculateMetricsFromSavedData(inputDirectory, fullOutputPath);
        } catch (const std::exception &e) {
            std::cerr << "Error calculating metrics: " << e.what() << std::endl;
        }
    }

    int runMenu()
    {
        using namespace ftxui;

        int selected = 0;
        std::vector<std::string> entries = {
            "Run Mock Sample",
            "Run Live Sample",
            "Run Hybrid Sample",
            "Review Saved Data",
            "Calculate Metrics from Saved Data",
            "Convert Saved Images",
            "EGrabber Config",
            "EGrabber Hot Reload",
            "Exit"};

        auto menu = Menu(&entries, &selected);

        auto screen = ScreenInteractive::TerminalOutput();

        bool quit = false;

        auto renderer = Renderer(menu, [&]
                                 { return vbox({
                                       text("=== Cell Analysis Menu ===") | bold | color(Color::Blue),
                                       separator(),
                                       menu->Render() | frame | border,
                                   }); });

        auto event_handler = CatchEvent(renderer, [&](Event event)
                                        {
            if (event == Event::Return) {
                quit = true;
                screen.ExitLoopClosure()();
                return true;
            }
            return false; });

        screen.Loop(event_handler);

        if (quit)
        {
            switch (selected)
            {
            case 0:
                runMockSample();
                break;
            case 1:
                runLiveSample();
                break;
            case 2:
                runHybridSample();
                break;
            case 3:
                reviewSavedData();
                break;
            case 4:
                calculateMetrics();
                break;
            case 5:
                convertSavedImages();
                break;
            case 6:
                egrabberConfig();
                break;
            case 7:
                egrabberHotReload();
                break;
            case 8:
                std::cout << "Exiting program.\n";
                return 0;
            }
        }

        return runMenu(); // Recursive call to show menu again after action
    }

} // namespace MenuSystem
