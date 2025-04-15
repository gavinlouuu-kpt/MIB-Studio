# MIB-Studio Project Brief

## Project Overview
MIB-Studio is an image processing application that currently utilizes a Terminal User Interface (TUI) via FTXUI library and OpenCV's imshow windows to visualize image processing speed and camera status. The application is built using C++ with CMake and vcpkg for dependency management.

## Core Requirements
- Real-time image processing capabilities
- Camera status monitoring
- Performance visualization and metrics
- Integrated user interface

## Transition Goal
Migrate from the current separated TUI and OpenCV window approach to a unified Qt-based GUI that integrates all functionality into a cohesive application.

## Key Milestones
1. Qt integration into build system
2. UI component design and implementation
3. Migration of OpenCV visualization to Qt
4. Migration of terminal UI elements to Qt
5. Integration of all components

## Success Criteria
- All existing functionality preserved
- Improved user experience with integrated interface
- Maintained or improved performance
- Clean architecture supporting future extensions 