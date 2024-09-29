cmake_minimum_required(VERSION 3.10)
project(MockGrabberProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the target name as a variable
set(TARGET_NAME mock_grabber)

find_package(OpenCV REQUIRED)

# Set the project root directory correctly
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)

# Include the directory containing CircularBuffer.h
include_directories(${PROJECT_ROOT_DIR}/lib/CircularBuffer)
include_directories(${PROJECT_ROOT_DIR}/lib/image_processing)

# Collect all source files in a variable
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_ROOT_DIR}/lib/CircularBuffer/CircularBuffer.cpp
    ${PROJECT_ROOT_DIR}/lib/image_processing/image_processing_utils.cpp
    ${PROJECT_ROOT_DIR}/lib/image_processing/image_processing_core.cpp
    ${PROJECT_ROOT_DIR}/lib/image_processing/image_processing_threads.cpp
)

# Print the SOURCES variable for debugging
message(STATUS "SOURCES: ${SOURCES}")


# Add the executable target
add_executable(${TARGET_NAME} ${SOURCES})

# Link libraries to the target
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})

# Include OpenCV directories
target_include_directories(${TARGET_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})

