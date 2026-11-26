#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MetalKnob : public juce::Slider
{
public:
    explicit MetalKnob (const juce::String& labelText);
    void paint (juce::Graphics&) override;

private:
    juce::String labelText;
};

class DistortionSwitch : public juce::Button
{
public:
    DistortionSwitch();
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};

class WaveguideSwitch : public juce::Button
{
public:
    WaveguideSwitch();
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};

class RingModSwitch : public juce::Button
{
public:
    RingModSwitch();
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};

class WaveringAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WaveringAudioProcessorEditor (WaveringAudioProcessor&);
    ~WaveringAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WaveringAudioProcessor& audioProcessor;
    MetalKnob driveKnob { "DRIVE" }, toneKnob { "TONE" }, biasKnob { "BIAS" },
              bodyKnob { "BODY" }, mixKnob { "MIX" };
    DistortionSwitch distortionSwitch;
    MetalKnob rateKnob { "RATE" }, resonanceKnob { "RESONANCE" }, feedbackKnob { "FEEDBACK" },
              dampingKnob { "DAMPING" }, characterKnob { "CHARACTER" }, waveMixKnob { "MIX" };
    WaveguideSwitch waveguideSwitch;
    MetalKnob ringRateKnob { "RATE" }, ringDepthKnob { "DEPTH" }, ringFineKnob { "FINE" },
              ringMixKnob { "MIX" };
    juce::ComboBox ringWaveformSelector;
    juce::Label ringWaveformLabel;
    RingModSwitch ringModSwitch;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> switchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> waveguideSwitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ringModSwitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ringWaveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessorEditor)
};
