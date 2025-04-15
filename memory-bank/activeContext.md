# Active Context

## Current Focus
- Enhancing Qt integration for MIB-Studio
- Building robust image display capabilities
- Planning metrics visualization components

## Recent Changes
- Initialized memory bank for project documentation
- Updated vcpkg.json to include Qt6 dependencies
- Configured CMakeLists.txt for Qt support
- Created directory structure for Qt components (src/qt, src/ui, src/resources)
- Implemented OpenCV Mat to QImage conversion utility
- Created basic MainWindow class for displaying images
- Created a Qt test application
- Fixed Qt platform plugin deployment issues
- Created deployment script (deploy_qt.ps1)
- Added post-build commands to automate Qt deployment

## Active Decisions
- Use Qt6 rather than Qt5 for future compatibility
- Maintain OpenCV as the core image processing library
- Create a separate utility for OpenCV Mat to QImage conversion
- Use QLabel for initial image display, may switch to QGraphicsView later
- Implement modular UI with dockable components
- Automate Qt deployment through CMake post-build steps

## Next Steps
1. **Enhanced Image Display**
   - Implement zoom/pan capabilities
   - Support multiple image views
   - Add overlay capability for annotations and measurements
   - Create customized image display widget

2. **Metrics Visualization**
   - Create basic dashboard using QtCharts
   - Implement real-time performance metrics display
   - Create dock widget for metrics visualization
   - Design performance history graph components

3. **Controls Migration**
   - Design UI for parameter adjustments
   - Implement settings dialogs
   - Create camera control panel
   - Add configuration persistence

4. **Integration with Existing Code**
   - Create abstraction layer for UI framework selection
   - Implement command-line switch for UI selection
   - Ensure all existing functionality is preserved
   - Create smooth transition path

5. **Final Integration**
   - Connect all components with Qt signals/slots
   - Create unified UI layout with docking capabilities
   - Add menus and keyboard shortcuts
   - Finalize consistent styling

## Current Challenges
- Ensuring thread safety between UI and image processing
- Maintaining real-time performance with Qt rendering
- Creating seamless integration with existing codebase
- Designing flexible UI that scales with different image sizes
- Training team members on Qt development practices 