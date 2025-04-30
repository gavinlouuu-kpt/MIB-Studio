#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "menu_system/menu_system.h"
#include "qt_ui/MainWindow.h"
#include <QApplication>
#include <QDir>
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
            // Initialize Qt application first
            QApplication app(argc, argv);
            
            // Now we can use QCoreApplication::applicationDirPath()
            // Set the Qt plugin path to help find the platform plugin
            QString appDir = QDir(QCoreApplication::applicationDirPath()).absolutePath();
            
            // Try multiple possible plugin locations
            QStringList pluginPaths;
            
            // Check vcpkg installed location
            pluginPaths << appDir + "/../../vcpkg_installed/x64-windows/qt6/plugins";
            pluginPaths << appDir + "/../vcpkg_installed/x64-windows/qt6/plugins";
            pluginPaths << appDir + "/vcpkg_installed/x64-windows/qt6/plugins";
            
            // Check other common locations
            pluginPaths << appDir + "/plugins";
            pluginPaths << appDir;
            
            // Debug output
            std::cout << "Checking for Qt plugins in the following paths:" << std::endl;
            for (const QString& path : pluginPaths) {
                std::cout << "  " << path.toStdString() << std::endl;
                if (QDir(path).exists()) {
                    std::cout << "  --> Path exists" << std::endl;
                    // Add to application library paths
                    QCoreApplication::addLibraryPath(path);
                }
            }
            
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
