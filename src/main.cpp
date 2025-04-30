#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "menu_system/menu_system.h"
#include "qt_ui/MainWindow.h"
#include <QApplication>
#include <iostream>
#include <string>
#include <filesystem>

int main(int argc, char *argv[])
{
    try
    {
        // Check if we should use the Qt UI or fallback to the FTXUI interface
        bool useQt = true;  // Set to false to use the FTXUI interface
        
        if (useQt) {
            // Initialize Qt application
            QApplication app(argc, argv);
            
            // Create and show the main window
            MainWindow mainWindow;
            mainWindow.show();
            
            // Run the application event loop
            return app.exec();
        } else {
            // Fallback to the original FTXUI-based menu system
            return MenuSystem::runMenu();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
