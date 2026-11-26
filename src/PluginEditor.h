#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>
#include <functional>
#include "PluginProcessor.h"

class MetalKnob : public juce::Slider
{
public:
    explicit MetalKnob (const juce::String& labelText);
    void paint (juce::Graphics&) override;

private:
    juce::String labelText;
};

class WaveformKnob : public juce::Slider
{
public:
    WaveformKnob();
    void paint (juce::Graphics&) override;
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

class SpectralDelaySwitch : public juce::Button
{
public:
    SpectralDelaySwitch();
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};

class WaveringAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::ComboBox::Listener,
                                     private juce::AudioProcessorValueTreeState::Listener,
                                     private juce::AsyncUpdater
{
private:
    class ScaledContent : public juce::Component
    {
    public:
        std::function<void (juce::Graphics&)> paintContent;
        void paint (juce::Graphics& g) override
        {
            if (paintContent)
                paintContent (g);
        }
    };

public:
    explicit WaveringAudioProcessorEditor (WaveringAudioProcessor&);
    ~WaveringAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int designWidth = 760;
    static constexpr int designHeight = 1040;
    void paintDesign (juce::Graphics&);
    void applyScaleMode (int selectedItemId);
    void comboBoxChanged (juce::ComboBox*) override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    WaveringAudioProcessor& audioProcessor;
    ScaledContent scaledContent;
    juce::Label scaleLabel;
    juce::ComboBox scaleSelector;
    std::atomic<int> pendingScaleChoice { 1 };

    MetalKnob driveKnob { "DRIVE" }, toneKnob { "TONE" }, biasKnob { "BIAS" },
              bodyKnob { "BODY" }, mixKnob { "MIX" };
    DistortionSwitch distortionSwitch;
    MetalKnob rateKnob { "RATE" }, resonanceKnob { "RESONANCE" }, feedbackKnob { "FEEDBACK" },
              dampingKnob { "DAMPING" }, characterKnob { "CHARACTER" }, waveMixKnob { "MIX" };
    WaveguideSwitch waveguideSwitch;
    MetalKnob ringRateKnob { "RATE" }, ringDepthKnob { "DEPTH" }, ringFineKnob { "FINE" },
              ringMixKnob { "MIX" };
    WaveformKnob ringWaveformSelector;
    RingModSwitch ringModSwitch;
    MetalKnob spectralTimeKnob { "TIME" }, spectralLowKnob { "LOW" }, spectralMidKnob { "MID" },
              spectralHighKnob { "HIGH" }, spectralFeedbackKnob { "FEEDBACK" },
              spectralWidthKnob { "WIDTH" }, spectralMixKnob { "MIX" };
    SpectralDelaySwitch spectralDelaySwitch;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> switchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> waveguideSwitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ringModSwitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> spectralDelaySwitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ringWaveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessorEditor)
};
