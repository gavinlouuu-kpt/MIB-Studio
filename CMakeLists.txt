cmake_minimum_required(VERSION 3.10)
project(MIB_Studio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Qt settings
set(CMAKE_AUTOMOC ON)  # Enable Qt's Meta-Object Compiler
set(CMAKE_AUTORCC ON)  # Enable automatic compilation of Qt resource files
set(CMAKE_AUTOUIC ON)  # Enable automatic UI compilation
set(CMAKE_INCLUDE_CURRENT_DIR ON)  # Include the build directory (for generated UI headers)

# Find packages
find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Qt packages
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
    # Qt specific sources
    src/qt_ui/MainWindow.cpp
    src/qt_ui/ImageView.cpp
    src/qt_ui/QtUtils.cpp
    # Add other source files here
)

# Qt UI files
set(UI_FILES
    src/qt_ui/MainWindow.ui
)

# Qt resource files
set(RESOURCE_FILES
    resources/resources.qrc
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${UI_FILES} ${RESOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # Add binary dir for generated files
    ${OpenCV_INCLUDE_DIRS}
)
# Separately include the EGrabber folder for the nested files
include_directories(${PROJECT_SOURCE_DIR}/include/EGrabber) # does not work with camera only for testing
# include_directories("C:/Program Files/Euresys/eGrabber/include") # required for camera

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    # Qt libraries
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Charts
)

# Setup Qt deployment - copy plugins to the build output directory
if(WIN32)
    # Create a qt.conf file to tell Qt where to find plugins
    file(GENERATE OUTPUT $<TARGET_FILE_DIR:${PROJECT_NAME}>/qt.conf CONTENT "[Paths]\nPlugins = ./plugins\n")
    
    # Create plugin directories
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/styles
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/imageformats
        COMMENT "Creating plugin directories"
    )
    
    # Helper function to copy Qt plugin with debug/release variant
    function(copy_qt_plugin)
        cmake_parse_arguments(ARG "" "PLUGIN_PATH;PLUGIN_NAME;OUTPUT_DIR" "" ${ARGN})
        
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(plugin_suffix "d")
        else()
            set(plugin_suffix "")
        endif()
        
        set(plugin_file "${ARG_PLUGIN_PATH}/${ARG_PLUGIN_NAME}${plugin_suffix}.dll")
        
        # Check if the plugin file exists
        if(EXISTS "${plugin_file}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${plugin_file}"
                    "${ARG_OUTPUT_DIR}/"
                COMMENT "Copying ${ARG_PLUGIN_NAME}${plugin_suffix}.dll"
            )
        else()
            # Try the alternative suffix if the first attempt failed
            if(plugin_suffix STREQUAL "d")
                set(alt_plugin_file "${ARG_PLUGIN_PATH}/${ARG_PLUGIN_NAME}.dll")
            else()
                set(alt_plugin_file "${ARG_PLUGIN_PATH}/${ARG_PLUGIN_NAME}d.dll")
            endif()
            
            if(EXISTS "${alt_plugin_file}")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${alt_plugin_file}"
                        "${ARG_OUTPUT_DIR}/"
                    COMMENT "Copying alternative ${ARG_PLUGIN_NAME} plugin"
                )
            else()
                message(STATUS "Plugin file not found: ${plugin_file} or ${alt_plugin_file}")
            endif()
        endif()
    endfunction()
    
    # Copy platform plugins
    copy_qt_plugin(
        PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins/platforms"
        PLUGIN_NAME "qwindows"
        OUTPUT_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms"
    )
    
    # Copy style plugins
    copy_qt_plugin(
        PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins/styles"
        PLUGIN_NAME "qwindowsvistastyle"
        OUTPUT_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/styles"
    )
    
    # Copy image format plugins
    foreach(img_plugin qjpeg qgif qico qsvg)
        copy_qt_plugin(
            PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins/imageformats"
            PLUGIN_NAME "${img_plugin}"
            OUTPUT_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/imageformats"
        )
    endforeach()
    
    # Copy Qt core DLLs
    foreach(qt_lib Core Gui Widgets Charts)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(lib_suffix "d")
            set(bin_dir "debug/bin")
        else()
            set(lib_suffix "")
            set(bin_dir "bin")
        endif()
        
        set(qt_dll "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/${bin_dir}/Qt6${qt_lib}${lib_suffix}.dll")
        
        if(EXISTS "${qt_dll}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${qt_dll}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
                COMMENT "Copying Qt6${qt_lib}${lib_suffix}.dll"
            )
        else()
            message(STATUS "Qt DLL not found: ${qt_dll}")
        endif()
    endforeach()
endif()

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
        src/mib_grabber/mib_grabber.cpp
    )
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EGrabber
        # "C:/Program Files/Euresys/eGrabber/include"
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${test_name} PRIVATE 
        Matplot++::matplot
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
    )
endforeach()
