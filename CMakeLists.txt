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
    PLUGIN_MANUFACTURER_CODE HUNT          # Ровно 4 заглавных символа
    PLUGIN_CODE             WVDM          # Ровно 4 заглавных символа
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Wavering Distortion Mod"
    BUNDLE_ID               "com.hunterice.waveringdistortionmod" # Обязательно для macOS/iOS
    COPY_PLUGIN_AFTER_BUILD TRUE          # Автоматически копирует плагин в папку плагинов после сборки
    IS_VST_DEPRECATED       false)

target_sources(WaveringDistortionMod PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h                 # Добавлено для корректного отображения в IDE
    src/PluginEditor.cpp
    src/PluginEditor.h)                   # Добавлено для корректного отображения в IDE

target_compile_definitions(WaveringDistortionMod PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(WaveringDistortionMod PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)     # Опционально: включение Link Time Optimization для уменьшения размера и ускорения
