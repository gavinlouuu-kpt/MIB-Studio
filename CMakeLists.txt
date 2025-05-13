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

# Find the XMT_DLL_SER library
find_library(XMT_DLL_SER_LIB XMT_DLL_SER 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    NO_DEFAULT_PATH
)

# List all your source files here
set(SOURCES
    src/main.cpp
    src/image_processing/image_processing_core.cpp
    src/image_processing/image_processing_utils.cpp
    src/image_processing/image_processing_threads.cpp
    src/menu_system/menu_system.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/mib_grabber/mib_grabber.cpp
    # Add other source files here
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    ${XMT_DLL_SER_LIB}
)

# Copy the Coremor DLL to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor/XMT_DLL_SER.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor
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
        ${XMT_DLL_SER_LIB}
    )
endforeach()
