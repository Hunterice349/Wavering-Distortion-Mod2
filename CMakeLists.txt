cmake_minimum_required(VERSION 3.22)

project(WaveringDistortionMod
    VERSION 1.0.0
    LANGUAGES C CXX
)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(WaveringDistortionMod
    COMPANY_NAME "Hunterice"
    PLUGIN_MANUFACTURER_CODE HUNT
    PLUGIN_CODE WVDM
    FORMATS VST3 Standalone
    PRODUCT_NAME "Wavering Distortion Mod"
    BUNDLE_ID "com.hunterice.waveringdistortionmod"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(WaveringDistortionMod)

target_sources(WaveringDistortionMod
    PRIVATE
        src/DistortionDSP.cpp
        src/DistortionDSP.h
        src/WaveguideDSP.cpp
        src/WaveguideDSP.h
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h
)

target_compile_features(WaveringDistortionMod
    PRIVATE
        cxx_std_17
)

target_compile_definitions(WaveringDistortionMod
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(WaveringDistortionMod
    PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

enable_testing()

add_executable(DistortionDSPTests
    tests/DSPTests.cpp
    src/DistortionDSP.cpp
    src/WaveguideDSP.cpp
)

target_include_directories(DistortionDSPTests
    PRIVATE
        src
)

target_compile_features(DistortionDSPTests
    PRIVATE
        cxx_std_17
)

add_test(
    NAME DistortionDSPTests
    COMMAND DistortionDSPTests
)

set_tests_properties(
    DistortionDSPTests
    PROPERTIES TIMEOUT 30
)
