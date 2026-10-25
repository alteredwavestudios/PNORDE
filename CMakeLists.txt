# ===========================================================================
# PNORDE build instructions for CMake (beginner explanation)
# ===========================================================================
# CMake is a tool which TELLS the C++ compiler what to build and where to
# find each source file. It is not the compiler itself: on Windows, Visual
# Studio supplies the actual C++ compiler and linker.
#
# We ask for C++17, locate the JUCE music-plugin framework, and tell JUCE to
# build both the VST3 and a standalone application.
#
# Don't rearrange the existing plugin IDs or host parameter IDs casually:
# existing Ableton sets use them when reopening old projects.
# ===========================================================================


cmake_minimum_required(VERSION 3.22)

project(PnordeDreamSequencer VERSION 0.6.29 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is the external framework used to compile PNORDE.
# We search ONLY locations relative to this repository, then an optional path
# explicitly supplied by the developer. No computer-specific drive paths!
#
# A GitHub clone with submodules places JUCE in third_party/JUCE.
# A downloaded ZIP can use scripts/Get-JUCE.ps1 to create that same folder.
# You can also select an existing JUCE checkout with:
#   cmake -S . -B build -DPNORDE_JUCE_DIR="C:/your/own/path/JUCE"
# The legacy JUCE_DIR variable remains accepted for existing build commands.
set(PNORDE_JUCE_DIR "" CACHE PATH "Optional path to a local JUCE source checkout")

if(PNORDE_JUCE_DIR)
    set(_pnorde_juce "${PNORDE_JUCE_DIR}")
elseif(JUCE_DIR)
    set(_pnorde_juce "${JUCE_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/modules/juce_core")
    set(_pnorde_juce "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_core")
    set(_pnorde_juce "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
else()
    message(FATAL_ERROR
        "JUCE not found. Run scripts/Get-JUCE.ps1, clone with --recurse-submodules, "
        "or pass -DPNORDE_JUCE_DIR=path/to/JUCE.")
endif()

if(NOT EXISTS "${_pnorde_juce}/modules/juce_core")
    message(FATAL_ERROR "JUCE modules missing: ${_pnorde_juce}")
endif()
message(STATUS "PNORDE: using JUCE from ${_pnorde_juce}")
# Give JUCE a private binary output folder under the chosen build tree.
add_subdirectory("${_pnorde_juce}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

# Create the plugin.
juce_add_plugin(PnordeDreamSequencer
    COMPANY_NAME "Altered Wave Studios"
    BUNDLE_ID "com.alteredwavestudios.pnorde"
    PLUGIN_MANUFACTURER_CODE AlWS
    PLUGIN_CODE PnDr

    FORMATS VST3 Standalone

    PRODUCT_NAME "PNORDE - Dream Sequencer"

    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE

    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE

    VST3_CATEGORIES Instrument
)

# Generate JuceHeader.h for our source files.
juce_generate_juce_header(PnordeDreamSequencer)

# Plugin source files.
target_sources(PnordeDreamSequencer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PnordeProjectVersion.h
        Source/PnordeProgramPolling.h
        Source/PnordeDeveloperStats.h
        Source/PnordeMidiBlackBox.h
        Source/PnordeUIViewport.h
        Source/PnordeStateLimits.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PnordeRetainedTransport.h
        Source/PnordeSwing.h
        Source/PnordeVibes.h
        Source/PnordeLights.h
        Source/PnordePresetFormat.h
        Source/PnordeSafeCC.h
        Source/PnordeSafeAutomation.h
        Source/PnordeLane.h
        Source/PnordeVisualOrder.h
        Source/PnordeAftertouchGlide.h
        Source/PnordeTempoGlide.h
        Source/PnordeDials.h
        Source/PnordeScales.h
        Source/PnordeRelativePitch.h
        Source/PnordeScaleDegreeEdit.h
        Source/PnordeEDO.h
        Source/PnordeManualTempo.h
        Source/PnordeMicroOutput.h
        Source/PnordePatternLibrary.h
        Source/PnordePlayback.h
        Source/PnordeProbability.h
        Source/PnordeRotation.h
        Source/PnordeProgramLane.h
        Source/PnordeProgramRequests.h
        Source/PnordeGlobalReverse.h
        Source/PnordeFactoryPresets.h
        Source/PnordeCompositionEngine.h
)

# JUCE compile settings.
target_compile_definitions(PnordeDreamSequencer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_WIN_WEBVIEW2=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# JUCE modules.
target_link_libraries(PnordeDreamSequencer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ===========================================================================
# OPTIONAL STAGE 1 INTEGRATION TEST (OFF in ordinary PNORDE builds)
# ===========================================================================
# A console app is a little program that prints PASS or FAIL rather than
# appearing in Ableton. It links PNORDE's SAME processor implementation, so
# it can test actual Quick Program save/load without making a second plugin.
#
# The VST3 build is unchanged unless -DPNORDE_BUILD_PROGRAM_STATE_TESTS=ON
# is explicitly passed to CMake. Tests must not become release-time features.
option(PNORDE_BUILD_PROGRAM_STATE_TESTS
       "Build optional JUCE Quick Program save/load integration test" OFF)

if(PNORDE_BUILD_PROGRAM_STATE_TESTS)
    juce_add_console_app(PnordeProgramStateTests
        PRODUCT_NAME "PNORDE Program State Tests")
    juce_generate_juce_header(PnordeProgramStateTests)
    target_sources(PnordeProgramStateTests PRIVATE
        tests/test_juce_program_state.cpp)
    target_link_libraries(PnordeProgramStateTests PRIVATE
        PnordeDreamSequencer
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()


# ==========================================================================
# OPTIONAL STAGE 2 NUMERICAL SAFETY TEST (OFF for normal plugin builds)
# ==========================================================================
# This tiny console program uses ONLY standard C++ and our pure maths helpers.
# It checks malformed host values and verifies ordinary timing calculations
# remain identical to those in the tested v0.6.25 Stage 1 source.
option(PNORDE_BUILD_NUMERICAL_TESTS
       "Build optional Stage 2 numerical safety regression tests" OFF)
if(PNORDE_BUILD_NUMERICAL_TESTS)
    add_executable(PnordeStage2SafetyTests
        tests/test_stage2_numerical_safety.cpp)
    target_compile_features(PnordeStage2SafetyTests PRIVATE cxx_std_17)
endif()

# ==========================================================================
# OPTIONAL STAGE 3 MUSICAL CONSISTENCY TEST (OFF for ordinary builds)
# ==========================================================================
# This JUCE-free console program verifies that the composition generator's
# scale degrees exactly match the EXISTING PITCH editor/playback scale masks.
# No MIDI or UI code is run by this test; real Ableton checks still follow.
option(PNORDE_BUILD_MUSICAL_TESTS
       "Build optional Stage 3 musical consistency regression tests" OFF)
if(PNORDE_BUILD_MUSICAL_TESTS)
    add_executable(PnordeStage3MusicalTests tests/test_stage3_musical_consistency.cpp)
    target_compile_features(PnordeStage3MusicalTests PRIVATE cxx_std_17)
endif()

# ===========================================================================
# OPTIONAL STAGE 4 VIEWPORT + VERSION TEST (OFF for ordinary plugin builds)
# ===========================================================================
option(PNORDE_BUILD_UI_TESTS "Build Stage 4 viewport/version safety tests" OFF)
if(PNORDE_BUILD_UI_TESTS)
    add_executable(PnordeStage4UITests tests/test_stage4_ui_viewport.cpp)
    target_compile_features(PnordeStage4UITests PRIVATE cxx_std_17)
endif()

# ===========================================================================
# OPTIONAL STAGE 5 DEDICATED MIDI CC MAPPING TEST (JUCE-FREE)
# ===========================================================================
# Checks the controller numbers AND controller values generated for every
# normal 0..100 lane position. This runs as a tiny console executable.
# It deliberately does not instantiate a VST3 or alter plugin parameters.
option(PNORDE_BUILD_CC_MAPPING_TESTS
       "Build Stage 5 MIDI dedicated CC mapping parity tests" OFF)
if(PNORDE_BUILD_CC_MAPPING_TESTS)
    add_executable(PnordeStage5CCMappingTests tests/test_stage5_cc_mapping.cpp)
    target_compile_features(PnordeStage5CCMappingTests PRIVATE cxx_std_17)
endif()

# Optional lightweight v0.6.26 polling/version sanity test.
# Does NOT link the entire JUCE plugin, and does not affect the VST3.
option(PNORDE_BUILD_POLLING_TESTS "Build v0.6.26 poll rate and version test" OFF)
if(PNORDE_BUILD_POLLING_TESTS)
    add_executable(PnordeStage56PollingTests tests/test_stage56_polling.cpp)
    target_compile_features(PnordeStage56PollingTests PRIVATE cxx_std_17)
    target_include_directories(PnordeStage56PollingTests PRIVATE Source)
endif()

# Standalone v0.6.27 Developer Tools counters test (does not require JUCE).
option(PNORDE_BUILD_DEVELOPER_TESTS "Build Developer Tools counter tests" OFF)
if(PNORDE_BUILD_DEVELOPER_TESTS)
    add_executable(PnordeDeveloperToolsTests tests/test_v0627_developer_tools.cpp)
    target_compile_features(PnordeDeveloperToolsTests PRIVATE cxx_std_17)
    target_include_directories(PnordeDeveloperToolsTests PRIVATE Source)
endif()

# v0.6.28: optional JUCE-independent profiling counter test.
# Opt-in; the VST3 build gets no new host parameters or dependencies.
option(PNORDE_BUILD_V0628_TESTS "Build the v0.6.28 restore phase counter test" OFF)
if(PNORDE_BUILD_V0628_TESTS)
    add_executable(PnordeV0628ProfileTests tests/test_v0628_restore_breakdown.cpp)
    target_include_directories(PnordeV0628ProfileTests PRIVATE Source)
    target_compile_features(PnordeV0628ProfileTests PRIVATE cxx_std_17)
endif()

# v0.6.29: opt-in MIDI black box and aggregate timing counter tests (no JUCE).
option(PNORDE_BUILD_V0629_TESTS "Build v0.6.29 MIDI black box tests" OFF)
if(PNORDE_BUILD_V0629_TESTS)
    add_executable(PnordeV0629BlackBoxTests tests/test_v0629_blackbox.cpp)
    target_include_directories(PnordeV0629BlackBoxTests PRIVATE Source)
    target_compile_features(PnordeV0629BlackBoxTests PRIVATE cxx_std_17)
endif()
