cmake_minimum_required(VERSION 3.10)
project(MockGrabberProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add this line to set the OpenCV_DIR
set(OpenCV_DIR "D:/code/opencv/build")

find_package(OpenCV REQUIRED)

# Include the directory containing CircularBuffer.h
include_directories("D:/code/VS_EGrabber/EGrabber/lib/CircularBuffer")

# Collect all source files in a variable
set(SOURCES
    src/main.cpp
    D:/code/VS_EGrabber/EGrabber/lib/CircularBuffer/CircularBuffer.cpp
    # D:/code/VS_EGrabber/EGrabber/lib/MockGrabber/MockGrabber.cpp
    # Add more source files here
)

# Add the executable target
add_executable(mock_grabber ${SOURCES})

# Link libraries to the target
target_link_libraries(mock_grabber ${OpenCV_LIBS})

# Include OpenCV directories
target_include_directories(mock_grabber PRIVATE ${OpenCV_INCLUDE_DIRS})