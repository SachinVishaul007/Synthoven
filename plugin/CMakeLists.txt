cmake_minimum_required(VERSION 3.22)

project(ChopPlugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Pull JUCE automatically (no manual install needed) ────────────────────────
# First configure will git-clone JUCE (~a minute). To use a local JUCE checkout
# instead, pass -DJUCE_PATH=/path/to/JUCE and it'll be used directly.
include(FetchContent)

if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ── The plugin target ─────────────────────────────────────────────────────────
juce_add_plugin(ChopPlugin
    PRODUCT_NAME            "Chop Sample Browser"
    COMPANY_NAME            "Chop"
    BUNDLE_ID               "com.chop.samplebrowser"
    PLUGIN_MANUFACTURER_CODE Chop
    PLUGIN_CODE             Chsb
    FORMATS                 VST3 Standalone
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(ChopPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ChopApiClient.cpp
    Source/SampleListBox.cpp
    Source/BpeTokenizer.cpp
    Source/MelSpectrogramExtractor.cpp
    Source/ClapModelManager.cpp)

target_compile_definitions(ChopPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    # On Linux, set JUCE_USE_CURL=1 and link curl for networking. macOS/Windows
    # use native networking, so curl is not required there.
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ChopPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(APPLE)
    FetchContent_Declare(
        onnxruntime
        URL https://github.com/microsoft/onnxruntime/releases/download/v1.18.0/onnxruntime-osx-arm64-1.18.0.tgz
    )
    FetchContent_MakeAvailable(onnxruntime)

    target_include_directories(ChopPlugin PRIVATE "${onnxruntime_SOURCE_DIR}/include")
    target_link_libraries(ChopPlugin PRIVATE "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.dylib")
elseif(WIN32)
    FetchContent_Declare(
        onnxruntime
        URL https://github.com/microsoft/onnxruntime/releases/download/v1.18.0/onnxruntime-win-x64-1.18.0.zip
    )
    FetchContent_MakeAvailable(onnxruntime)

    target_include_directories(ChopPlugin PRIVATE "${onnxruntime_SOURCE_DIR}/include")
    target_link_libraries(ChopPlugin PRIVATE "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib")

    if(TARGET ChopPlugin_Standalone)
        add_custom_command(TARGET ChopPlugin_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:ChopPlugin_Standalone>"
        )
    endif()

    if(TARGET ChopPlugin_VST3)
        add_custom_command(TARGET ChopPlugin_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:ChopPlugin_VST3>"
        )
    endif()
endif()
