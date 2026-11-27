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
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(WaveringDistortionMod PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/DistortionDSP.cpp  # <--- ОБЯЗАТЕЛЬНО ДОБАВЬТЕ ЭТО
    src/PluginProcessor.h
    src/PluginEditor.h
    src/DistortionDSP.h    # <--- И ЭТО
)
target_compile_definitions(WaveringDistortionMod PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(WaveringDistortionMod PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)

# Standalone DSP test harness uses DistortionDSP and the C++ standard library.
enable_testing()
add_executable(DistortionDSPTests
    tests/DSPTests.cpp
    src/DistortionDSP.cpp)
target_include_directories(DistortionDSPTests PRIVATE src)
target_compile_features(DistortionDSPTests PRIVATE cxx_std_17)
add_test(NAME DistortionDSPTests
         COMMAND DistortionDSPTests "${CMAKE_BINARY_DIR}/distortion_metrics.csv")
