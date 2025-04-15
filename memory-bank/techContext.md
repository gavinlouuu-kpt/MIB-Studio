# Technical Context

## Current Technology Stack
- **C++17**: Core programming language
- **CMake**: Build system
- **vcpkg**: Package manager
- **OpenCV 4**: Image processing library
- **FTXUI**: Terminal UI framework
- **Matplotplusplus**: For plotting graphs in terminal
- **nlohmann-json**: JSON handling

## Development Environment
- **Visual Studio 2022**: Primary IDE 
- **Windows 10/11**: Development OS
- **vcpkg integrated with CMake**: Dependency management

## CMake Configuration
The project uses CMake with vcpkg integration for managing dependencies. CMake configuration includes:
- Visual Studio 2022 generator
- x64 architecture
- vcpkg toolchain integration

## Current Dependencies (vcpkg)
```json
{
  "dependencies": [
    "ftxui",
    "matplotplusplus",
    "opencv4",
    "nlohmann-json"
  ]
}
```

## Planned Qt Integration
- Add Qt6 as a dependency via vcpkg
- Required Qt modules:
  - QtCore
  - QtGui
  - QtWidgets
  - QtCharts (for metrics visualization)

## Technical Constraints
- Maintain compatibility with existing OpenCV pipeline
- Ensure real-time performance with UI rendering overhead
- Keep build system complexity manageable
- Support future extensions to the processing pipeline
- Ensure thread safety between UI and processing components

## Required Tools for Transition
- Qt Designer: For UI layout design
- Qt resource compiler (rcc): For resource management
- CMake Qt tools: For integrating Qt with build system

## Build System Modifications
- Update vcpkg.json to include Qt dependencies
- Configure CMake to handle Qt's MOC, UIC, and RCC
- Add Qt-specific build flags and configurations
- Update include paths for Qt headers
- Configure library linking for Qt components 