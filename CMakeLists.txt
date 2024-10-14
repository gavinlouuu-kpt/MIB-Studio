cmake_minimum_required(VERSION 3.10)
project(MockGrabberProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure CMake is set to Release mode
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


# Find OpenCV package
find_package(OpenCV REQUIRED)

# Set the project root directory correctly
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/" ABSOLUTE)

# Include directories
include_directories(
    ${PROJECT_ROOT_DIR}/lib/CircularBuffer
    ${PROJECT_ROOT_DIR}/lib/image_processing
    ${PROJECT_ROOT_DIR}/lib/menu_system
    ${PROJECT_ROOT_DIR}/lib/json
    ${PROJECT_ROOT_DIR}/lib/egrabber
    ${OpenCV_INCLUDE_DIRS}
)

# Collect all header files
file(GLOB_RECURSE LIB_HEADERS 
    "${PROJECT_ROOT_DIR}/lib/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# Collect all source files except main files and test files
file(GLOB_RECURSE LIB_SOURCES 
    "${PROJECT_ROOT_DIR}/lib/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\.cpp$")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*test.*\.cpp$")
# list(FILTER LIB_SOURCES EXCLUDE REGEX ".*image_processing_.*\.cpp$")

# Print the LIB_SOURCES variable for debugging
message(STATUS "LIB_SOURCES: ${LIB_SOURCES}")

# Function to create an executable with OpenCV
function(create_opencv_executable name)
    add_executable(${name} ${LIB_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/${name}.cpp)
    target_link_libraries(${name} ${OpenCV_LIBS})
    
    if(WIN32)
        # Copy OpenCV DLLs
        foreach(lib ${OpenCV_LIBS})
            if(CMAKE_BUILD_TYPE MATCHES Debug)
                set(dll_name "${OpenCV_DIR}/bin/Debug/${lib}d.dll")
            else()
                set(dll_name "${OpenCV_DIR}/bin/Release/${lib}.dll")
            endif()
            if(EXISTS ${dll_name})
                add_custom_command(TARGET ${name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${dll_name}
                    $<TARGET_FILE_DIR:${name}>
                )
            endif()
        endforeach()
    endif()
endfunction()

# Create executables for each test
create_opencv_executable(main_test)
create_opencv_executable(mock_grabber)

message(STATUS "OpenCV_DIR is set to: ${OpenCV_DIR}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")


# At the end of your file, add:
message(STATUS "CMAKE_BUILD_TYPE is: ${CMAKE_BUILD_TYPE}")
