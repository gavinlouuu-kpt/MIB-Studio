#pragma once

#include <string>

namespace MenuSystem
{

    void clearInputBuffer();
    void displayMenu();
    void runMockSample();
    void runLiveSample();
    void convertSavedImages();
    int runMenu();
    void processAllBatches(const std::string &saveDirectory);

} // namespace MenuSystem