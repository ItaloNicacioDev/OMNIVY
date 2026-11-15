cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(OMNIVY VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library" FORCE)
endif()

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
if(NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "F:/BACKUP/FREELA/PROGRAMS/VST/Dependencies/JUCE")
endif()
message(STATUS "JUCE_PATH: ${JUCE_PATH}")
if(NOT EXISTS "${JUCE_PATH}/modules/juce_core")
    message(FATAL_ERROR "JUCE nao encontrado em ${JUCE_PATH}")
endif()

# Windows x64 target enforcement
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "OMNIVY requer target x64 no Windows")
    endif()
    message(STATUS "Target: Windows x64")
endif()

# Traz o JUCE para a build para que juce_add_plugin e os targets
# juce::juce_* fiquem disponiveis. Metodo oficial documentado em
# Dependencies/JUCE/docs/CMake API.md (secao "Using add_subdirectory").
add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

# ---------------------------------------------------------------------------
# Biblioteca compartilhada do plugin (DSP + Editor).
# juce_add_plugin cria essa estatica + um target wrapper por formato.
# ---------------------------------------------------------------------------
juce_add_plugin(OmnivyPlugin
    PLUGIN_NAME "Omnivy"
    PLUGIN_DESCRIPTION "OMNIVY Professional Multi-Engine Software Synthesizer"
    PLUGIN_MANUFACTURER "OMNIVY"
    PLUGIN_MANUFACTURER_CODE "OMNV"
    PLUGIN_CODE "OMNV"
    PLUGIN_VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    FORMATS VST3
)

target_sources(OmnivyPlugin PRIVATE
    src/Core/PluginProcessor.cpp
    src/Core/PluginEditor.cpp
)

target_link_libraries(OmnivyPlugin PRIVATE
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_dsp
)

target_link_libraries(OmnivyPlugin PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Aplicacao Standalone (reusa a biblioteca compartilhada + main proprio).
# ---------------------------------------------------------------------------
add_executable(OmnivyStandalone
    src/Core/Main.cpp
)

target_link_libraries(OmnivyStandalone PRIVATE
    OmnivyPlugin
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_audio_utils
)

target_compile_definitions(OmnivyStandalone PRIVATE
    JUCE_STANDALONE=1
)

# ---------------------------------------------------------------------------
# Saida
# ---------------------------------------------------------------------------
set_target_properties(OmnivyPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)

set_target_properties(OmnivyStandalone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)
