cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(OMNIVY VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE path
if(NOT DEFINED JUCE_PATH)
  set(JUCE_PATH "F:/BACKUP/FREELA/PROGRAMS/VST/Dependencies/JUCE/JUCE")
endif()

message(STATUS "JUCE_PATH: ${JUCE_PATH}")
message(STATUS "JUCE version: 9.0.5")

# Include JUCE CMake module
set(JUCE_USE_VST3SDK TRUE)
set(JUCE_USE_CARBON FALSE)
set(JUCE_USE_WINSDK TRUE)

# Windows x64 target
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Target: Windows x64")
  else()
    message(FATAL_ERROR "OMNIVY requires x64 target on Windows")
  endif()
endif()

# Path to JUCE CMake module
list(APPEND CMAKE_MODULE_PATH "${JUCE_PATH}/modules/juce_core/build.cmake")
# Actually JUCE provides juce_add_plugin via juce_add_plugin macro
include("${JUCE_PATH}/modules/juce_core/build.cmake" OPTIONAL RESULT_VARIABLE JUCE_BUILD_INCLUDE_RESULT)
if(NOT JUCE_BUILD_INCLUDE_RESULT)
  message(WARNING "JUCE build.cmake not found at expected path, using default JUCE CMake integration")
endif()

# Plugin target
add_library(OmnivyPlugin MODULE
  src/Core/PluginProcessor.cpp
  src/Core/PluginEditor.cpp
)

target_include_directories(OmnivyPlugin PRIVATE
  src
  ${JUCE_PATH}/modules
)

target_compile_definitions(OmnivyPlugin PRIVATE
  JUCE_VST3=1
  JUCE_STANDALONE=0
)

# JUCE plugin configuration
juce_add_plugin(OmnivyPlugin
  PLUGIN_NAME "Omnivy"
  PLUGIN_DESCRIPTION "OMNIVY Professional Multi-Engine Software Synthesizer"
  PLUGIN_MANUFACTURER "OMNIVY"
  PLUGIN_VERSION "1.0.0"
  PLUGIN_UNIQUE_ID "OMNV"
  PLUGIN_FORMATS "VST3"
  PLUGIN_WINDOWS_BITNESS 64
)

# Standalone target
add_executable(OmnivyStandalone
  src/Core/Main.cpp
)

target_include_directories(OmnivyStandalone PRIVATE
  src
  ${JUCE_PATH}/modules
)

target_compile_definitions(OmnivyStandalone PRIVATE
  JUCE_VST3=0
  JUCE_STANDALONE=1
)

juce_add_plugin(OmnivyStandalone
  PLUGIN_NAME "Omnivy"
  PLUGIN_DESCRIPTION "OMNIVY Professional Multi-Engine Software Synthesizer (Standalone)"
  PLUGIN_MANUFACTURER "OMNIVY"
  PLUGIN_VERSION "1.0.0"
  PLUGIN_UNIQUE_ID "OMNV"
  PLUGIN_FORMATS "Standalone"
  PLUGIN_WINDOWS_BITNESS 64
)

# Output directories
set_target_properties(OmnivyPlugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)

set_target_properties(OmnivyStandalone PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)

# Link libraries (MSVC)
if(MSVC)
  target_link_libraries(OmnivyPlugin PRIVATE
    "${JUCE_PATH}/modules/juce_audio_plugin_client/VST3_SDK/VST3_SDK.lib"
  )
  target_link_options(OmnivyPlugin PRIVATE /NODEFAULTLIB:LIBCMT)
endif()
