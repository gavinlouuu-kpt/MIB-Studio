cmake_minimum_required(VERSION 3.10)
project(MIB_Studio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress character encoding warnings from third-party headers
if(MSVC)
    add_compile_options(/wd4828)  # Suppress C4828: character encoding warning
endif()

find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Set OpenCV_DIR to the correct location
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

# Find the XMT_DLL_SER library
find_library(XMT_DLL_SER_LIB XMT_DLL_SER 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    NO_DEFAULT_PATH
)

# List all your source files here
set(SOURCES
    src/main.cpp
    src/core/image_processing/image_processing_core.cpp
    src/core/image_processing/image_processing_utils.cpp
    src/core/image_processing/image_processing_threads.cpp
    src/menu_system/menu_system.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/core/grabber/mib_grabber.cpp
    # Add other source files here
)

# New modular targets for decoupled architecture
# Core static library (no GUI framework dependency intended; current sources include HighGUI and will be refactored later)
set(MIB_CORE_SOURCES
    src/core/image_processing/image_processing_core.cpp
    src/core/image_processing/image_processing_utils.cpp
    src/core/image_processing/image_processing_threads.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/core/grabber/mib_grabber.cpp
)

add_library(mib_core STATIC ${MIB_CORE_SOURCES})
target_include_directories(mib_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(mib_core PUBLIC
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    ${XMT_DLL_SER_LIB}
)
target_compile_definitions(mib_core PUBLIC MIB_DISABLE_DISPLAY_THREADS=1)

# API static library (placeholder for controller + public API headers)
add_library(mib_api STATIC
    src/api/dummy.cpp
    src/api/MibController.cpp
)
target_include_directories(mib_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(mib_api PUBLIC mib_core)

# Optional Qt adapter (disabled by default if Qt is not available)
option(BUILD_QT_ADAPTER ON)
if(BUILD_QT_ADAPTER)
    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    add_library(mib_qt_adapter STATIC
        src/qt/dummy.cpp
        src/qt/QtMibController.h
        src/qt/QtMibController.cpp
    )
    target_include_directories(mib_qt_adapter PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(mib_qt_adapter PUBLIC
        mib_api
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
    set_target_properties(mib_qt_adapter PROPERTIES AUTOMOC ON)
endif()

# Combined TUI + Qt image window app
add_executable(mib_tui_qt
    apps/tui_qt/main.cpp
)
target_include_directories(mib_tui_qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(mib_tui_qt PRIVATE
    mib_api
    mib_qt_adapter
    ftxui::screen
    ftxui::dom
    ftxui::component
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Standalone CLI app using API
add_executable(mib_cli
    apps/cli/main.cpp
)
target_include_directories(mib_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(mib_cli PRIVATE mib_api)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
# Separately include the EGrabber folder for the nested files
# include_directories(${PROJECT_SOURCE_DIR}/include/EGrabber) # does not work with camera only for testing
include_directories("C:/Program Files/Euresys/eGrabber/include") # required for camera
# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    ${XMT_DLL_SER_LIB}
)

# Copy the Coremor DLL to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor/XMT_DLL_SER.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Build test executables
file(GLOB TEST_SOURCES "src/tests/*.cpp")
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source}
        src/image_processing/image_processing_core.cpp
        src/image_processing/image_processing_utils.cpp
        src/image_processing/image_processing_threads.cpp
        src/menu_system/menu_system.cpp
        src/CircularBuffer/CircularBuffer.cpp
        src/core/grabber/mib_grabber.cpp

    )
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # ${CMAKE_CURRENT_SOURCE_DIR}/include/EGrabber
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Coremor
        "C:/Program Files/Euresys/eGrabber/include"
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${test_name} PRIVATE 
        Matplot++::matplot
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        ${XMT_DLL_SER_LIB}
    )
endforeach()
