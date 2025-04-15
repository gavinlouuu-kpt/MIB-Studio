cmake_minimum_required(VERSION 3.15)
project(MIB_Studio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt6 specific configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Needed for proper MSVC detection
if(MSVC)
  add_compile_options(/W4)
endif()

find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Gui Charts REQUIRED)

# Set OpenCV_DIR to the correct location
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

# List all your source files here
set(SOURCES
    src/main.cpp
    src/image_processing/image_processing_core.cpp
    src/image_processing/image_processing_utils.cpp
    src/image_processing/image_processing_threads.cpp
    src/menu_system/menu_system.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/mib_grabber/mib_grabber.cpp
    # Add other source files here
)

# Qt-specific sources
set(QT_SOURCES
    src/qt/main_window.cpp
)

# Qt UI files
set(QT_UI_FILES
    # Will be populated as Qt UI files are created
)

# Qt resource files
set(QT_RESOURCES
    src/resources/resources.qrc
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${QT_SOURCES} ${QT_UI_FILES} ${QT_RESOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Separately include the EGrabber folder for the nested files
if(EXISTS "C:/Program Files/Euresys/eGrabber/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/Euresys/eGrabber/include")
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Charts
)

# Build test executables
file(GLOB TEST_SOURCES "src/tests/*.cpp")
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Skip qt_test.cpp since we're handling it separately
    if(NOT test_name STREQUAL "qt_test")
        add_executable(${test_name} ${test_source}
            src/image_processing/image_processing_core.cpp
            src/image_processing/image_processing_utils.cpp
            src/image_processing/image_processing_threads.cpp
            src/menu_system/menu_system.cpp
            src/CircularBuffer/CircularBuffer.cpp
            src/mib_grabber/mib_grabber.cpp
        )
        
        target_include_directories(${test_name} PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${OpenCV_INCLUDE_DIRS}
        )
        
        if(EXISTS "C:/Program Files/Euresys/eGrabber/include")
            target_include_directories(${test_name} PRIVATE "C:/Program Files/Euresys/eGrabber/include")
        endif()
        
        target_link_libraries(${test_name} PRIVATE 
            Matplot++::matplot
            ftxui::screen
            ftxui::dom
            ftxui::component
            nlohmann_json::nlohmann_json
            ${OpenCV_LIBS}
        )
    endif()
endforeach()

# Add Qt test specifically
add_executable(qt_test_app 
    src/tests/qt_test.cpp
    src/qt/main_window.cpp
    ${QT_RESOURCES}
)

target_include_directories(qt_test_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(qt_test_app PRIVATE 
    ${OpenCV_LIBS}
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Charts
)

# Add post-build command to deploy Qt dependencies
if(WIN32)
    find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell)
    
    if(POWERSHELL_EXECUTABLE)
        add_custom_command(TARGET qt_test_app POST_BUILD
            COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/deploy_qt.ps1"
            COMMENT "Deploying Qt plugins and dependencies..."
        )
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/deploy_qt.ps1"
            COMMENT "Deploying Qt plugins and dependencies..."
        )
    endif()
endif()
