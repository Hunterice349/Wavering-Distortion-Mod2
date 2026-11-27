#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class WaveringLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    WaveringLookAndFeel();

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class WaveringAudioProcessorEditor final
    : public juce::AudioProcessorEditor
{
public:
    explicit WaveringAudioProcessorEditor(WaveringAudioProcessor&);
    ~WaveringAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int parameterCount = 11;

    WaveringAudioProcessor& audioProcessor;
    WaveringLookAndFeel lookAndFeel;

    std::array<juce::Slider, parameterCount> sliders;
    std::array<juce::Label, parameterCount> labels;

    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    juce::TextButton distortionButton { "DISTORTION" };
    juce::TextButton waveguideButton { "WAVEGUIDE" };

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        distortionAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        waveguideAttachment;

    const std::array<const char*, parameterCount> parameterIDs {
        "drive", "tone", "bias", "body", "mix",
        "waveRate", "waveResonance", "waveFeedback",
        "waveDamping", "waveCharacter", "waveMix"
    };

    const std::array<const char*, parameterCount> parameterNames {
        "DRIVE", "TONE", "BIAS", "BODY", "MIX",
        "RATE", "RESONANCE", "FEEDBACK",
        "DAMPING", "CHARACTER", "WAVE MIX"
    };

    void setupSlider(int index);
    void drawPanel(juce::Graphics&, juce::Rectangle<float>,
                   const juce::String& title);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        WaveringAudioProcessorEditor)
};