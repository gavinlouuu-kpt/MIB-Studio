# MIB-Studio Technical Context

## Current Technology Stack

The application currently uses the following technologies:

| Technology | Purpose | Version |
|------------|---------|---------|
| C++ | Core programming language | C++17 |
| CMake | Build system | 3.10+ |
| vcpkg | Package manager | Latest |
| OpenCV | Image processing | 4.x |
| FTXUI | User interface | Latest |
| Matplotplusplus | Data visualization | Latest |
| nlohmann-json | JSON handling | Latest |
| EGrabber | Camera interface | Latest |

## Dependencies and Integration

The project manages dependencies through vcpkg, with the following key integrations:

```json
{
  "dependencies": [
    "ftxui",
    "matplotplusplus",
    "opencv4",
    "nlohmann-json",
    "qtbase",
    "qtcharts"
  ]
}
```

## Development Environment

- **Required Tools**:
  - Visual Studio 2022 with C++ development tools
  - CMake
  - vcpkg package manager
  - Git

- **Build Configuration**:
  - Uses CMake presets for consistency
  - Requires system environment variable `VCPKG_ROOT`
  - Uses CMakeUserPresets.json for user-specific settings

## Technical Constraints

1. **Performance Requirements**:
   - Must handle real-time image processing
   - Smooth UI responsiveness during intensive tasks
   - Efficient memory usage for large image sets

2. **External Integration Requirements**:
   - Compatible with EGrabber camera interface
   - Support for common image file formats
   - Ability to export analysis results in standard formats

3. **Platform Requirements**:
   - Primary: Windows 10/11
   - Build system must support cross-platform if needed in future

## Qt Integration Strategy

The Qt integration will involve:

1. **Qt Base Components**:
   - QtWidgets for UI components
   - QtCore for application foundation
   - QtGui for graphics handling

2. **Qt Extensions**:
   - QtCharts for data visualization
   - Optional: QtConcurrent for parallel processing

3. **OpenCV Integration**:
   - Convert between OpenCV Mat and QImage/QPixmap
   - Maintain OpenCV processing pipeline integrity
   - Keep image processing logic separate from UI

4. **Build System Updates**:
   - Update CMakeLists.txt for Qt requirements
   - Integrate Qt MOC (Meta-Object Compiler) processing
   - Configure Qt resources 