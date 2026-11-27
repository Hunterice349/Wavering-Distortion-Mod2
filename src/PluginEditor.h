/*
  ==============================================================================

    PluginEditor.h
    WAVERING DISTORTION MOD - GOTHIC RED NEON PREMIUM REDESIGN

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Forward declaration for custom components
class LabeledKnob;
class CustomVuMeter;
class GothicFootswitch;

//==============================================================================
/**
*/
class WaveringDistortionModAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    WaveringDistortionModAudioProcessorEditor (WaveringDistortionModAudioProcessor&);
    ~WaveringDistortionModAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // Helper to load font
    void loadGothicFont();

    // Reference to the processor
    WaveringDistortionModAudioProcessor& audioProcessor;

    // Custom Components
    std::unique_ptr<LabeledKnob> driveKnob;
    std::unique_ptr<LabeledKnob> toneKnob;
    std::unique_ptr<LabeledKnob> biasKnob;
    std::unique_ptr<LabeledKnob> bodyKnob;
    std::unique_ptr<LabeledKnob> mixKnob;

    std::unique_ptr<CustomVuMeter> vuMeter;
    std::unique_ptr<GothicFootswitch> footswitch;

    // Fonts
    juce::Typeface::Ptr gothicTypeface;
    juce::Font gothicFont;
    juce::Font labelFont;

    // Colors
    static constexpr juce::uint32 NeonRed = 0xFFFF0033;
    static constexpr juce::uint32 DarkMetal = 0xFF1A1A1A;
    static constexpr juce::uint32 BrushedBlack = 0xFF0D0D0D;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringDistortionModAudioProcessorEditor)
};

//==============================================================================
/**
    Custom Component: Labeled Knob with Numeric Input
*/
class LabeledKnob : public juce::Component,
                    public juce::Slider::Listener,
                    public juce::Label::Listener
{
public:
    LabeledKnob (const juce::String& paramName, 
                 juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& parameterID,
                 juce::Font& font)
        : name(paramName), attachedFont(font)
    {
        // Setup Slider
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(nullptr); // We will paint manually or use default lookAndFeel if needed, but here we override paint
        
        // Attach to APVTS
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, parameterID, slider);
        
        addAndMakeVisible(slider);
        slider.addListener(this);

        // Setup Label for Numeric Input
        valueLabel.setEditable(true, true, true);
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setFont(attachedFont.withHeight(14.0f));
        valueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        valueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x88000000));
        valueLabel.setColour(juce::Label::outlineWhenFocusedColourId, juce::Colour(NeonRed));
        
        addAndMakeVisible(valueLabel);
        valueLabel.addListener(this);

        // Initial text update
        updateLabelText();
    }

    ~LabeledKnob() override {}

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Slider takes upper part
        slider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.75));
        
        // Label takes lower part
        valueLabel.setBounds(bounds.reduced(5, 2));
    }

    void paint(juce::Graphics& g) override
    {
        // Draw Name in Gothic Font
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(attachedFont.withHeight(16.0f));
        g.drawText(name, getLocalBounds(), juce::Justification::bottomCenter, true);
        
        // Optional: Subtle red glow under the name
        g.setGradientFill(juce::ColourGradient(juce::Colour(NeonRed).withAlpha(0.3f), 
                                               getLocalBounds().getCentreX(), getLocalBounds().getBottom(),
                                               juce::Colour(NeonRed).withAlpha(0.0f), 
                                               getLocalBounds().getCentreX(), getLocalBounds().getBottom() + 10, false));
        g.fillRect(getLocalBounds());
    }

    void sliderValueChanged(juce::Slider* sliderThatWasMoved) override
    {
        if (sliderThatWasMoved == &slider)
        {
            updateLabelText();
        }
    }

    void labelTextChanged(juce::Label* labelThatHasChanged) override
    {
        if (labelThatHasChanged == &valueLabel)
        {
            juce::String text = valueLabel.getText();
            double newValue = text.getDoubleValue();
            
            // Get range from slider to ensure validity
            juce::Range<double> range = slider.getRange();
            
            if (newValue >= range.getStart() && newValue <= range.getEnd())
            {
                slider.setValue(newValue, juce::sendNotificationAsync);
            }
            else
            {
                // Revert to current valid value if out of bounds
                updateLabelText();
            }
        }
    }

private:
    void updateLabelText()
    {
        // Format value based on parameter characteristics (e.g. 2 decimal places)
        double val = slider.getValue();
        valueLabel.setText(juce::String(val, 2), juce::dontSendNotification);
    }

    juce::String name;
    juce::Slider slider;
    juce::Label valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Font& attachedFont;
};

//==============================================================================
/**
    Custom Component: Gothic VU Meter
*/
class CustomVuMeter : public juce::Component, public juce::Timer
{
public:
    CustomVuMeter(WaveringDistortionModAudioProcessor& proc) : processor(proc)
    {
        startTimerHz(30); // Update rate for visual smoothness
    }

    void timerCallback() override
    {
        // Fetch level from processor (atomic read)
        currentLevel = processor.getCurrentLevel();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background: Dark Glass
        g.setColour(juce::Colour(0xFF050505));
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        
        // Border: Red Neon
        g.setColour(juce::Colour(NeonRed));
        g.drawRoundedRectangle(bounds.toFloat(), 5.0f, 2.0f);

        // Scale Arc
        float arcRadius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
        juce::Point<float> centre(bounds.getCentreX(), bounds.getCentreY() + 10);
        
        juce::Path scalePath;
        scalePath.addArc(centre.getX() - arcRadius, centre.getY() - arcRadius, 
                         arcRadius * 2, arcRadius * 2, 
                         juce::MathConstants<float>::pi * 1.2f, 
                         juce::MathConstants<float>::pi * 1.8f);
        
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.strokePath(scalePath, juce::PathStrokeType(2.0f));

        // Needle
        float angle = juce::MathConstants<float>::pi * 1.2f + 
                      (currentLevel * (juce::MathConstants<float>::pi * 0.6f)); // Map 0..1 to arc
        
        juce::Point<float> needleEnd(
            centre.getX() + std::cos(angle) * (arcRadius - 5),
            centre.getY() + std::sin(angle) * (arcRadius - 5)
        );

        g.setColour(juce::Colour(NeonRed));
        g.drawLine(centre.x, centre.y, needleEnd.x, needleEnd.y, 2.0f);
        
        // Pivot
        g.fillEllipse(centre.getX() - 4, centre.getY() - 4, 8, 8);
    }

private:
    WaveringDistortionModAudioProcessor& processor;
    float currentLevel = 0.0f;
};

//==============================================================================
/**
    Custom Component: Gothic Footswitch
*/
class GothicFootswitch : public juce::Component, public juce::Button::Listener
{
public:
    GothicFootswitch(WaveringDistortionModAudioProcessor& proc) : processor(proc)
    {
        switchBtn.setButtonText("ON");
        switchBtn.setClickingTogglesState(true);
        switchBtn.setLookAndFeel(nullptr); // Custom paint
        
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.getState(), "distortionEnabled", switchBtn);
            
        addAndMakeVisible(switchBtn);
        switchBtn.addListener(this);
    }

    void buttonClicked(juce::Button* button) override
    {
        repaint();
    }

    void resized() override
    {
        switchBtn.setBounds(getLocalBounds());
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        bool isOn = switchBtn.getToggleState();

        // Metal Base
        juce::DropShadow shadow(juce::Colours::black, 10, juce::Point<int>(0, 4));
        shadow.drawForRectangle(g, bounds);

        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xFF333333), 0, 0,
            juce::Colour(0xFF111111), 0, (float)bounds.getHeight(), false));
        g.fillRoundedRectangle(bounds.toFloat().reduced(2), 10.0f);

        // Inner Circle (Button Top)
        auto innerBounds = bounds.reduced(10).toFloat();
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xFF222222), 0, 0,
            juce::Colour(0xFF000000), 0, innerBounds.getHeight(), false));
        g.fillEllipse(innerBounds);

        // LED Indicator
        juce::Point<float> ledCentre(bounds.getCentreX(), bounds.getCentreY() - 10);
        float ledRadius = 6.0f;
        
        if (isOn)
        {
            // Glow
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(NeonRed).withAlpha(0.8f), ledCentre.x, ledCentre.y,
                juce::Colour(NeonRed).withAlpha(0.0f), ledCentre.x, ledCentre.y + 20, false));
            g.fillEllipse(ledCentre.x - ledRadius * 2, ledCentre.y - ledRadius * 2, ledRadius * 4, ledRadius * 4);
            
            // Core
            g.setColour(juce::Colours::white);
            g.fillEllipse(ledCentre.x - ledRadius, ledCentre.y - ledRadius, ledRadius * 2, ledRadius * 2);
        }
        else
        {
            g.setColour(juce::Colour(0xFF330000));
            g.fillEllipse(ledCentre.x - ledRadius, ledCentre.y - ledRadius, ledRadius * 2, ledRadius * 2);
        }
        
        // Text Label below
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(gothicFontFallback.withHeight(14.0f)); // Using a simple fallback here or passed font
        g.drawText(isOn ? "ACTIVE" : "BYPASS", bounds.removeFromBottom(20), juce::Justification::centred);
    }

private:
    WaveringDistortionModAudioProcessor& processor;
    juce::TextButton switchBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    
    // Simple fallback font for this component if not passed
    juce::Font gothicFontFallback = juce::Font("Serif", 14.0f, juce::Font::bold);
};

