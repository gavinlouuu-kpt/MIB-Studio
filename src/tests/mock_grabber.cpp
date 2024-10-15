#include "image_processing.h"
#include "CircularBuffer.h"
#include <iostream>
#include <string>
#include <filesystem>
#include "menu_system.h"

int main()
{
    try
    {
        return MenuSystem::runMenu();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}