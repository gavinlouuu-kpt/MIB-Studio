cmake_minimum_required(VERSION 3.10)
project(MIB_Studio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Qt settings
set(CMAKE_AUTOMOC ON)  # Enable Qt's Meta-Object Compiler
set(CMAKE_AUTORCC ON)  # Enable automatic compilation of Qt resource files
set(CMAKE_AUTOUIC ON)  # Enable automatic UI compilation
set(CMAKE_INCLUDE_CURRENT_DIR ON)  # Include the build directory (for generated UI headers)

# Find packages
find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Qt packages
find_package(Qt6 COMPONENTS Core Widgets Gui Charts REQUIRED)

# Set OpenCV_DIR to the correct location
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

# List all your source files here
set(SOURCES
    src/main.cpp
    src/image_processing/image_processing_core.cpp
    src/image_processing/image_processing_utils.cpp
    src/image_processing/image_processing_threads.cpp
    src/menu_system/menu_system.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/mib_grabber/mib_grabber.cpp
    # Qt specific sources
    src/qt_ui/MainWindow.cpp
    src/qt_ui/ImageView.cpp
    src/qt_ui/QtUtils.cpp
    # Add other source files here
)

# Qt UI files
set(UI_FILES
    src/qt_ui/MainWindow.ui
)

# Qt resource files
set(RESOURCE_FILES
    resources/resources.qrc
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${UI_FILES} ${RESOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # Add binary dir for generated files
    ${OpenCV_INCLUDE_DIRS}
)
# Separately include the EGrabber folder for the nested files
include_directories(${PROJECT_SOURCE_DIR}/include/EGrabber) # does not work with camera only for testing
# include_directories("C:/Program Files/Euresys/eGrabber/include") # required for camera

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    # Qt libraries
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Charts
)

# Build test executables
file(GLOB TEST_SOURCES "src/tests/*.cpp")
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source}
        src/image_processing/image_processing_core.cpp
        src/image_processing/image_processing_utils.cpp
        src/image_processing/image_processing_threads.cpp
        src/menu_system/menu_system.cpp
        src/CircularBuffer/CircularBuffer.cpp
        src/mib_grabber/mib_grabber.cpp
    )
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EGrabber
        # "C:/Program Files/Euresys/eGrabber/include"
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${test_name} PRIVATE 
        Matplot++::matplot
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
    )
endforeach()
