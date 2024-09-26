cmake_minimum_required(VERSION 3.10)
project(MockGrabberProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add this line to set the OpenCV_DIR
# set(OpenCV_DIR "D:/code/opencv/build")

find_package(OpenCV REQUIRED)

# Set the project root directory correctly
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)

# Include the directory containing CircularBuffer.h
include_directories(${PROJECT_ROOT_DIR}/lib/CircularBuffer)

# Collect all source files in a variable
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_ROOT_DIR}/lib/CircularBuffer/CircularBuffer.cpp
)

# Print the SOURCES variable for debugging
message(STATUS "SOURCES: ${SOURCES}")


# Add the executable target
add_executable(mock_grabber ${SOURCES})

# Link libraries to the target
target_link_libraries(mock_grabber ${OpenCV_LIBS})

# Include OpenCV directories
target_include_directories(mock_grabber PRIVATE ${OpenCV_INCLUDE_DIRS})