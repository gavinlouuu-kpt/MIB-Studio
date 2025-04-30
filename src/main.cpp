#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "menu_system/menu_system.h"
#include "qt_ui/MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
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
            // Set the plugin path explicitly before creating QApplication
            QStringList paths = QApplication::libraryPaths();
            paths.append(QDir::currentPath() + "/plugins");
            QApplication::setLibraryPaths(paths);
            
            // Initialize Qt application
            QApplication app(argc, argv);
            
            // Print debug information about Qt plugin paths
            std::cout << "Qt library paths: " << QApplication::libraryPaths().join("; ").toStdString() << std::endl;
            std::cout << "Qt plugin path: " << QLibraryInfo::path(QLibraryInfo::PluginsPath).toStdString() << std::endl;
            std::cout << "Current dir: " << QDir::currentPath().toStdString() << std::endl;

            // Check if platform plugins exist in expected locations
            if (QFile::exists("./plugins/platforms/qwindows.dll")) {
                std::cout << "Found qwindows.dll in ./plugins/platforms/" << std::endl;
            } else {
                std::cout << "qwindows.dll not found in ./plugins/platforms/" << std::endl;
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
