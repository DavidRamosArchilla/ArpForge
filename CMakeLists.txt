cmake_minimum_required (VERSION 3.22)

project (ArpForge VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the plugin doesn't need the VC++ redistributable.
set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option (ARPFORGE_TESTS "Build the engine unit tests" ON)

add_subdirectory (libs/JUCE)

juce_add_plugin (ArpForge
    COMPANY_NAME "ArpForge"
    BUNDLE_ID "com.arpforge.arpforge"
    PLUGIN_MANUFACTURER_CODE Afrg
    PLUGIN_CODE Arpf
    PRODUCT_NAME "ArpForge"
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator)

juce_add_binary_data (ArpForgeData
    SOURCES
        resources/fonts/Inter-Regular.ttf
        resources/fonts/Inter-Medium.ttf
        resources/fonts/Inter-SemiBold.ttf)

target_sources (ArpForge PRIVATE
    src/engine/ArpEngine.cpp
    src/engine/Patterns.cpp
    src/Parameters.cpp
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ui/Controls.cpp
    src/ui/ForgeLookAndFeel.cpp
    src/ui/PatternView.cpp
    src/ui/Theme.cpp)

target_compile_definitions (ArpForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries (ArpForge
    PRIVATE
        ArpForgeData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (ARPFORGE_TESTS)
    add_executable (EngineTests
        tests/EngineTests.cpp
        src/engine/ArpEngine.cpp
        src/engine/Patterns.cpp)
    enable_testing()
    add_test (NAME EngineTests COMMAND EngineTests)
endif()

option (ARPFORGE_TOOLS "Build developer tools (UI snapshot renderer)" ON)

if (ARPFORGE_TOOLS)
    juce_add_console_app (UiSnapshot PRODUCT_NAME "UiSnapshot")
    target_sources (UiSnapshot PRIVATE
        tools/UiSnapshot.cpp
        src/engine/ArpEngine.cpp
        src/engine/Patterns.cpp
        src/Parameters.cpp
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/ui/Controls.cpp
        src/ui/ForgeLookAndFeel.cpp
        src/ui/PatternView.cpp
        src/ui/Theme.cpp)
    target_compile_definitions (UiSnapshot PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="ArpForge")
    target_link_libraries (UiSnapshot PRIVATE
        ArpForgeData
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)

    juce_add_console_app (HostTest PRODUCT_NAME "HostTest")
    target_sources (HostTest PRIVATE tools/HostTest.cpp)
    target_compile_definitions (HostTest PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries (HostTest PRIVATE
        juce::juce_audio_processors
        juce::juce_recommended_config_flags)
endif()
