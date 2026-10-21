
cmake_minimum_required(VERSION 3.22)

project(PnordeDreamSequencer VERSION 0.6.21 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE may be kept on either drive. Prefer the new PNORDE workspace.
# You may override from PowerShell with -DJUCE_DIR="C:/path/to/JUCE".
if(NOT DEFINED JUCE_DIR)
    foreach(candidate
        "H:/Music Library/PNORDE/dev/JUCE"
        "H:/Music Library/PNORDE/JUCE"
        "D:/Music Projects/ABLETON MUSIC STUFF/Nova Sequencer/dev/JUCE")
        if(EXISTS "${candidate}/modules/juce_core")
            set(JUCE_DIR "${candidate}")
            break()
        endif()
    endforeach()
endif()
if(NOT DEFINED JUCE_DIR OR NOT EXISTS "${JUCE_DIR}/modules/juce_core")
    message(FATAL_ERROR "JUCE not found. Pass -DJUCE_DIR=path/to/JUCE")
endif()
message(STATUS "PNORDE: using JUCE from ${JUCE_DIR}")
add_subdirectory("${JUCE_DIR}" JUCE)

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
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PnordeRetainedTransport.h
        Source/PnordeVibes.h
        Source/PnordeLights.h
        Source/PnordePresetFormat.h
        Source/PnordeSafeCC.h
        Source/PnordeLane.h
        Source/PnordeVisualOrder.h
        Source/PnordeAftertouchGlide.h
        Source/PnordeTempoGlide.h
        Source/PnordeDials.h
        Source/PnordeScales.h
        Source/PnordeEDO.h
        Source/PnordeManualTempo.h
        Source/PnordeMicroOutput.h
        Source/PnordePatternLibrary.h
        Source/PnordePlayback.h
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
