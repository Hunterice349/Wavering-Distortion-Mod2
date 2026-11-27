#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#include "PluginProcessor.h"

//==============================================================================
// Premium metallic knob with numeric input
//==============================================================================

class MetalKnob : public juce::Component
{
public:
    explicit MetalKnob(const juce::String& labelText);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    void setValue(float newValue);
    float getValue() const;
    void setRange(double min, double max, double interval);
    
    std::function<void(float)> onValueChange;
    
private:
    juce::String labelText;
    double currentValue = 0.0;
    double minValue = 0.0;
    double maxValue = 10.0;
    double interval = 0.01;
    bool isDragging = false;
    float lastY = 0.0f;
    
    class NumericTextBox;
    std::unique_ptr<NumericTextBox> textBox;
    
    void updateTextBox();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalKnob)
};

//==============================================================================
// Mechanical guitar pedal footswitch
//==============================================================================

class DistortionFootswitch : public juce::Button
{
public:
    DistortionFootswitch();
    
    void paintButton(
        juce::Graphics& g,
        bool highlighted,
        bool down
    ) override;
};

//==============================================================================
// Analog VU meter
//==============================================================================

class PremiumVUMeter : public juce::Component
{
public:
    PremiumVUMeter();
    
    void setLevels(float rms, float peak);
    void paint(juce::Graphics& g) override;
    
private:
    float currentRms = 0.0f;
    float currentPeak = 0.0f;
    float smoothedNeedle = 0.0f;
    float peakPosition = 0.0f;
    
    void drawScale(
        juce::Graphics& g,
        juce::Rectangle<float> bounds
    );
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PremiumVUMeter)
};

//==============================================================================
// Main editor
//==============================================================================

class WaveringAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit WaveringAudioProcessorEditor(
        WaveringAudioProcessor& processor
    );
    
    ~WaveringAudioProcessorEditor() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    WaveringAudioProcessor& audioProcessor;
    
    void timerCallback() override;
    
    void drawWoodPanels(juce::Graphics& g);
    void drawMetalPanel(juce::Graphics& g);
    void drawHeader(juce::Graphics& g);
    void drawGothicOrnaments(juce::Graphics& g);
    
    PremiumVUMeter vuMeter;
    
    MetalKnob driveKnob { "DRIVE" };
    MetalKnob toneKnob { "TONE" };
    MetalKnob biasKnob { "BIAS" };
    MetalKnob bodyKnob { "BODY" };
    MetalKnob mixKnob { "MIX" };
    
    DistortionFootswitch footswitch;
    
    std::vector<
        std::unique_ptr<
            juce::AudioProcessorValueTreeState::SliderAttachment
        >
    > sliderAttachments;
    
    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment
    > footswitchAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        WaveringAudioProcessorEditor
    )
};