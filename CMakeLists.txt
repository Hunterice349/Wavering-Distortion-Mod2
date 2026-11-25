cmake_minimum_required(VERSION 3.22)
project(WaveringDistortionMod VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(WaveringDistortionMod
    COMPANY_NAME            "Hunterice"
    PLUGIN_MANUFACTURER_CODE HUNT
    PLUGIN_CODE             WVDM
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Wavering Distortion Mod"
    BUNDLE_ID               "com.hunterice.waveringdistortionmod"
    COPY_PLUGIN_AFTER_BUILD TRUE) # <-- Скобка закрывается здесь, и никаких других параметров ниже!

target_sources(WaveringDistortionMod PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h)

target_compile_definitions(WaveringDistortionMod PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(WaveringDistortionMod PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
