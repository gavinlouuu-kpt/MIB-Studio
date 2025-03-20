#pragma once

#include <string>

namespace MenuSystem
{

    void clearInputBuffer();
    void displayMenu();
    void runMockSample();
    void runLiveSample();
    void convertSavedImages();
    void egrabberConfig();
    int runMenu();
    void processAllBatches(const std::string &saveDirectory);
    std::string navigateAndSelectFolder();
} // namespace MenuSystem
