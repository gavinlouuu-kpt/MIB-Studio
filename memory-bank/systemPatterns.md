# MIB-Studio System Patterns

## System Architecture

The MIB-Studio application follows a modular architecture with these primary components:

1. **User Interface Layer** - Currently FTXUI-based, transitioning to Qt
2. **Application Logic Layer** - Core application flow and state management
3. **Image Processing Layer** - OpenCV-based image analysis algorithms
4. **Hardware Interface Layer** - Camera and device integration components

```
+-----------------------+
|     User Interface    |
| (FTXUI → Qt)          |
+-----------------------+
           |
+-----------------------+
|   Application Logic   |
+-----------------------+
           |
+------------+  +----------------+
| Image      |  | Hardware       |
| Processing |  | Interface      |
+------------+  +----------------+
```

## Key Design Patterns

1. **Model-View-Controller (MVC)** - Separation of data, presentation, and control logic
   - Models: Image data, analysis results, application settings
   - Views: UI components (menus, image display, charts)
   - Controllers: Event handlers and business logic

2. **Observer Pattern** - Used for updating UI components when underlying data changes
   - Will be implemented through Qt's signals and slots mechanism

3. **Strategy Pattern** - For swappable image processing algorithms
   - Different analysis techniques can be applied to the same image data

4. **Factory Pattern** - For creating appropriate image processors based on requirements
   - Used to instantiate the correct processing objects based on image type

## Component Relationships

### Current Structure
- **Menu System** - FTXUI-based interface for navigation and file selection
- **Circular Buffer** - Custom data structure for managing image sequences
- **Image Processing** - Core OpenCV-based processing algorithms
- **MIB Grabber** - Hardware interface for camera integration

### Planned Qt Structure
- **MainWindow** - Qt main application window containing all UI elements
- **ImageView** - Qt widget for displaying and interacting with images
- **ProcessingEngine** - Backend service for handling image processing tasks
- **DataVisualizer** - QtCharts-based visualization components
- **SettingsManager** - Configuration and preferences management

## Technology Stack Transitions

| Component | Current | Target |
|-----------|---------|--------|
| UI Framework | FTXUI | Qt |
| Image Processing | OpenCV | OpenCV (unchanged) |
| Visualization | Matplotplusplus | QtCharts |
| Build System | CMake | CMake (with Qt integration) |
| Data Storage | Custom + JSON | Qt + JSON | 