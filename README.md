# MIB-Studio Project Setup Guide

This guide will help you set up and run the MIB-Studio project, even if you have little programming experience. If you don't have a hardware camera, you'll be using the `mock_grabber` test.

## Prerequisites

1. Windows 10 or later
2. Visual Studio 2022 Community Edition (or later)
3. CMake (version 3.20 or later)
4. OpenCV (version 4.5 or later)

## Installation Steps

### 1. Install Visual Studio 2022 Community Edition

- Download from: https://visualstudio.microsoft.com/vs/community/
- During installation, make sure to select "Desktop development with C++"

### 2. Install CMake

- Download from: https://cmake.org/download/
- Choose the Windows x64 Installer
- During installation, select the option to add CMake to the system PATH

### 3. Install OpenCV

- Download from: https://opencv.org/releases/
- Choose the Windows version
- Extract the downloaded file to a location of your choice (e.g., `C:\opencv`)

### 4. Set up Environment Variables

1. Search for "Environment Variables" in the Windows start menu
2. Click on "Edit the system environment variables"
3. Click the "Environment Variables" button
4. Under "System variables", find "Path" and click "Edit"
5. Click "New" and add the path to OpenCV's bin directory (e.g., `C:\opencv\build\x64\vc16\bin`)

### 5. Clone or Download the MIB-Studio Project

- If you're familiar with Git, clone the repository
- Otherwise, download the project as a ZIP file and extract it

### 6. Configure the Project

1. Open a command prompt in the project directory
2. Create a build directory:
   ```
   mkdir build
   cd build
   ```
3. Run CMake:
   ```
   cmake .. -DOpenCV_DIR=C:/opencv/build
   ```
   Replace `C:/opencv/build` with the actual path to your OpenCV build directory

### 7. Build the Project

1. Open the generated `MockGrabberProject.sln` file in Visual Studio 2022
2. In the Solution Explorer, right-click on the `mock_grabber` project
3. Select "Set as Startup Project"
4. Click on "Build" in the top menu, then "Build Solution"

## Running the Tests

To run the `mock_grabber` test:

1. In Visual Studio, make sure `mock_grabber` is set as the startup project
2. Click on "Debug" in the top menu, then "Start Without Debugging" (or press Ctrl+F5)
3. When prompted, enter the path to a directory containing test images

## Troubleshooting

- If you encounter any "file not found" errors, double-check your OpenCV installation path and environment variables
- Make sure all required DLLs are in the same directory as the executable or in your system PATH

For more detailed information or if you encounter any issues, please refer to the project documentation or seek assistance from the project maintainers.
