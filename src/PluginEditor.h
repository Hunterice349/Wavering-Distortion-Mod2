#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class WaveringDistortionModAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    WaveringDistortionModAudioProcessorEditor (WaveringDistortionModAudioProcessor&);
    ~WaveringDistortionModAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    WaveringDistortionModAudioProcessor& audioProcessor;

    // --- Компоненты ---
    
    // VU Meter Component
    class VUMeterComponent : public juce::Component
    {
    public:
        VUMeterComponent(WaveringDistortionModAudioProcessor& proc) : processor(proc) {}
        void paint(juce::Graphics& g) override;
        void updateLevels(float rms, float peak);
    private:
        WaveringDistortionModAudioProcessor& processor;
        float currentRms = 0.0f;
        float needlePos = 0.0f; // Для анимации
    };

    // Custom Knob Component
    class PremiumKnob : public juce::Component
    {
    public:
        PremiumKnob(juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& label);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
    private:
        juce::AudioProcessorValueTreeState::ParameterAttachment attachment;
        juce::String label;
        bool isDragging = false;
        float lastY = 0.0f;
    };

    // Footswitch Component
    class PowerFootswitch : public juce::Component
    {
    public:
        PowerFootswitch(juce::AudioProcessorValueTreeState& state, const juce::String& paramID);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
    private:
        juce::AudioProcessorValueTreeState::ParameterAttachment attachment;
        bool isPressed = false;
        bool ledOn = false;
    };

    std::unique_ptr<VUMeterComponent> vuMeter;
    std::unique_ptr<PowerFootswitch> footSwitch;
    
    std::unique_ptr<PremiumKnob> knobDrive;
    std::unique_ptr<PremiumKnob> knobTone;
    std::unique_ptr<PremiumKnob> knobBias;
    std::unique_ptr<PremiumKnob> knobBody;
    std::unique_ptr<PremiumKnob> knobMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringDistortionModAudioProcessorEditor)
};