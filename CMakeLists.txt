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

# ---------------------------------------------------------------------------
# Pré-configura o vst3_helper em Release para evitar incompatibilidade de CRT.
# O CUSTOM_COMMAND gerado pelo JUCE configura o helper sem -DCMAKE_BUILD_TYPE,
# resultando em Debug (/MDd) que entra em conflito com o projeto principal
# (MultiThreadedDLL /MD). Ao pré-criar o cache em Release, o CUSTOM_COMMAND
# manterá o valor existente (Release) ao reconfigurar em tempo de build.
# ---------------------------------------------------------------------------
execute_process(
    COMMAND "${CMAKE_COMMAND}" -GNinja
        -S "${JUCE_PATH}/extras/Build/CMake/juce_vst3_helper"
        -B "${CMAKE_BINARY_DIR}/vst3_helpers/OmnivyPlugin"
        -Dhelper_name=vst3_helper
        -Dsource_file="${JUCE_PATH}/modules/juce_audio_plugin_client/VST3/juce_VST3ManifestHelper.cpp"
        -Dshared_defs_file="${CMAKE_BINARY_DIR}/vst3_helpers/OmnivyPlugin/shared_defs_Release.txt"
        -Dshared_incs_file="${CMAKE_BINARY_DIR}/vst3_helpers/OmnivyPlugin/shared_incs_Release.txt"
        -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
        -DCMAKE_BUILD_TYPE=Release
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    RESULT_VARIABLE _omnivy_vst3_helper_result
    OUTPUT_QUIET
    ERROR_QUIET
)
if (_omnivy_vst3_helper_result)
    message(WARNING "Falha ao pré-configurar vst3_helper em Release (código ${_omnivy_vst3_helper_result}). O VST3 pode falhar com incompatibilidade de CRT.")
endif()

target_sources(OmnivyPlugin PRIVATE
    src/Core/PluginProcessor.cpp
    src/Core/PluginEditor.cpp

    # --- DSP: Utilities ---
    # (header-only)

    # --- DSP: Oscillators ---
    src/DSP/Oscillators/AdditiveOscillator.cpp
    src/DSP/Oscillators/VAOscillator.cpp
    src/DSP/Oscillators/NoiseOscillator.cpp
    src/DSP/Oscillators/FMOscillator.cpp
    src/DSP/Oscillators/GranularOscillator.cpp

    # --- DSP: Wavetable ---
    src/DSP/Wavetable/WavetableData.cpp
    src/DSP/Wavetable/WavetableGenerator.cpp
    src/DSP/Wavetable/WavetableOscillator.cpp

    # --- DSP: Filters ---
    src/DSP/Filters/StateVariableFilter.cpp

    # --- DSP: Envelopes ---
    src/DSP/Envelopes/ADSREnvelope.cpp

    # --- DSP: Engine ---
    src/DSP/Engine/SynthVoice.cpp
    src/DSP/Engine/VoiceManager.cpp
    src/DSP/Engine/AudioEngine.cpp
    src/DSP/Engine/OscillatorEngine.cpp

    # --- DSP: Routing ---
    src/DSP/Routing/Connection.cpp
    src/DSP/Routing/RoutingGraph.cpp

    # --- Parameters ---
    src/Parameters/SynthParameters.cpp)

target_compile_definitions(OmnivyPlugin PRIVATE
  JUCE_VST3=1
  JUCE_STANDALONE=0
  JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OmnivyPlugin PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_dsp
)

target_include_directories(OmnivyPlugin PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
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
    WIN32_EXECUTABLE TRUE
)

