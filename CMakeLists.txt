cmake_minimum_required(VERSION 3.0)
project(app)

find_package(glfw3 REQUIRED FATAL_ERROR)

set(CMAKE_BUILD_TYPE Debug)

message(STATUS "Using " $ENV{VULKAN_SDK})

# set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
# set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
# add_subdirectory( ${CMAKE_HOME_DIRECTORY}/extern/tinygltf )

set(IMGUI_INCLUDE_DIRS external/imgui external/imgui/backends)
include_directories(${IMGUI_INCLUDE_DIRS})

set(IMGUI_CORE_FILES
    ${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_demo.cpp
    ${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_draw.cpp
    ${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_tables.cpp
    ${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_widgets.cpp
    ${CMAKE_HOME_DIRECTORY}/external/imgui/imgui.cpp
)
set(IMGUI_BACKEND_FILES
    ${CMAKE_HOME_DIRECTORY}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_HOME_DIRECTORY}/external/imgui/backends/imgui_impl_vulkan.cpp)

set(IMGUI_SOURCES "")
set(IMGUI_SOURCES ${IMGUI_CORE_FILES} ${IMGUI_BACKEND_FILES})



add_executable( ${PROJECT_NAME} main.cpp
    Helpers.cpp Helpers.hpp
    Resources.cpp Resources.hpp
    Loader.cpp Loader.hpp
    vkmDeviceFeatureManager.cpp vkmDeviceFeatureManager.hpp
    vkmInit.cpp vkmInit.hpp
    ${IMGUI_SOURCES} )

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories( ${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/include ${CMAKE_HOME_DIRECTORY}/external )
target_link_libraries( ${PROJECT_NAME} PRIVATE 
    $ENV{VULKAN_SDK}/lib/libvulkan.so
    glfw
)
