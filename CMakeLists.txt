cmake_minimum_required(VERSION 3.10)
project(MIB_Studio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

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
    # Add other source files here
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
# Separately include the EGrabber folder for the nested files
include_directories(${PROJECT_SOURCE_DIR}/include/EGrabber)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)

