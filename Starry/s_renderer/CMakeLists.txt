# ------------------------------- Project Settings -------------------------------
cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME s_renderer)
set(VERSION "0.0.6")

project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES C CXX)

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/renderer")

FILE(GLOB SOURCES "${SOURCE_DIR}/*.cpp")

# ------------------------------- Dependencies -------------------------------

set(DEPEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

add_subdirectory("${DEPEND_DIR}/glm" "glm")
set(GLFW_INCLUDE "${DEPEND_DIR}/glfw/include")

# Vulkan
include (FindVulkan)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
else()
  find_package(Vulkan REQUIRED)
endif()

# ------------------------------- ImGui -------------------------------
set(IMGUI_LIB imgui_lib)

# IMGUI
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
FILE(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")

add_library(${IMGUI_LIB} OBJECT ${IMGUI_SOURCES} "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp" "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp")

target_include_directories(${IMGUI_LIB}
    PRIVATE
    ${Vulkan_INCLUDE_DIRS}

    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends

    ${GLFW_INCLUDE}
)

set_target_properties(${IMGUI_LIB} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "imgui"
  LIBRARY_OUTPUT_DIRECTORY "imgui"
  OUTPUT_NAME "${IMGUI_LIB}"
)

# ------------------------------- Main Lib -------------------------------
set(MAIN_LIB s_renderer)

add_library(${MAIN_LIB} OBJECT 
  ${SOURCES}
  $<TARGET_OBJECTS:${IMGUI_LIB}> 
  $<TARGET_OBJECTS:glm>
)
target_compile_definitions(${MAIN_LIB} PRIVATE "VERSION=\"${VERSION}\"")

target_include_directories( ${MAIN_LIB}
    PRIVATE
    ${INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/s_manager/include
)

target_include_directories( ${MAIN_LIB}
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_include_directories( ${MAIN_LIB}
    PUBLIC
  $<BUILD_INTERFACE:${DEPEND_DIR}/glm>
  $<BUILD_INTERFACE:${DEPEND_DIR}/glfw/include>
  ${Vulkan_INCLUDE_DIRS}

  $<BUILD_INTERFACE:${IMGUI_DIR}>
  $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
)

set_target_properties(${MAIN_LIB} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${MAIN_LIB}"
  LIBRARY_OUTPUT_DIRECTORY "${MAIN_LIB}"
  OUTPUT_NAME "${MAIN_LIB}"
)