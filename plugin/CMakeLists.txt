cmake_minimum_required(VERSION 3.22)
project(Chop VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# Clone JUCE into plugin/JUCE/ first:
#   git submodule add https://github.com/juce-framework/JUCE.git plugin/JUCE
add_subdirectory(JUCE)

juce_add_plugin(Chop
    COMPANY_NAME                "Synthoven"
    PLUGIN_MANUFACTURER_CODE    Snth
    PLUGIN_CODE                 Chop
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Chop"
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_SYNTH                    FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

juce_generate_juce_header(Chop)

target_sources(Chop PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Library/SampleLibrary.cpp
    Source/Library/SamplePlayer.cpp
    Source/Bridge/JSBridge.cpp
)

target_include_directories(Chop PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/webview2/build/native/include"
)

target_compile_definitions(Chop PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

add_custom_command(TARGET Chop_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/webview2/build/native/x64/WebView2Loader.dll"
        "$<TARGET_FILE_DIR:Chop_Standalone>"
    COMMENT "Copying WebView2Loader.dll"
)

target_link_libraries(Chop
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
