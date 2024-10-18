# Cell Analysis Project

## Getting Started

To start the project, follow these steps:

In CMakeLists.txt select the EGrabber that is for testing if you don't have a camera to work with.

Before starting the project, you will need to install Visual Studio 2022 community edition with C++ development tools. A package manager called vcpkg is included in the installation; add the tool's path to PATH and create a system variable "VCPKG_ROOT" that corresponds to the path to vcpkg. Then, modify the CMakeUserPreset to point the vcpkg path to the path on your system.

1. Navigate to the project's root, where CMakeLists.txt and README.md are located.

2. Run the following command to set up vcpkg and install the relevant libraries:

   ```
   cmake --preset=default
   ```

   This will start vcpkg to install the required libraries into the build folder.

3. Enter the build folder:

   ```
   cd build
   ```

4. Generate the build files:

   ```
   cmake ..
   ```

5. Build the project in Release configuration:

   ```
   cmake --build . --config Release
   ```
## Project Structure

The project is organized into several key components:

1. **Main Application** (`src/main.cpp`): The entry point of the application, which sets up and runs the menu system.

2. **Menu System** (`src/menu_system/menu_system.cpp`): Handles user interaction and directs the program flow based on user choices.

3. **Image Processing** (`src/image_processing/`):
   - `image_processing_core.cpp`: Contains core image processing functions.
   - `image_processing_threads.cpp`: Implements multi-threaded image processing tasks.

4. **Circular Buffer** (`src/CircularBuffer/`): A custom circular buffer implementation for efficient image data management.

## Features

1. **Mock Sample**: Allows processing of pre-recorded images for testing and development purposes.
2. **Live Sample**: (Placeholder) For future implementation of real-time image capture and processing.
3. **Convert Saved Images**: Converts binary image files to a standard format for further analysis.

## Usage

1. Run the application.
2. Choose from the following options in the menu:
   - Run Mock Sample
   - Run Live Sample (not yet implemented)
   - Convert Saved Images
   - Exit

### Running a Mock Sample

1. Select "Run Mock Sample" from the menu.
2. Enter the path to the directory containing the sample images when prompted.
3. The program will process the images, displaying results in real-time.
4. Use keyboard controls during processing:
   - ESC: Stop capture
   - Space: Pause/Resume
   - 'a': Move to older frame
   - 'd': Move to newer frame
   - 'q': Clear circularities vector
   - 's': Save current frames

### Converting Saved Images

1. Select "Convert Saved Images" from the menu.
2. Enter the path to the binary image file when prompted.
3. Enter the output directory for converted images.
4. The program will convert the binary images to a standard format.

## Dependencies

- vcpkg (for managing most libraries)
- EGrabber (embedded)
- Matplotplusplus (requires gnuplot)
- OpenCV
- nlohmann/json

## Building

1. Ensure vcpkg is installed and properly configured in CMakeUserPresets.json.
2. Run `cmake --preset=default` in the project root directory.
3. Build the project using your preferred method (e.g., Visual Studio, command-line tools).

## Notes

- The live sampling feature is not yet implemented.
- Ensure all dependencies are properly installed before building the project.
- For any issues or questions, please refer to the project documentation or contact the development team.
