#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class WaveringAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    WaveringAudioProcessorEditor (WaveringAudioProcessor&);
    ~WaveringAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // Custom LookAndFeel for Gothic Neon aesthetics
    class GothicLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        GothicLookAndFeel();
        
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override;
        
        juce::Font getLabelFont (juce::Label&) override;
        void drawLabel (juce::Graphics&, juce::Label&) override;
        
        // Colors
        const juce::Colour bgBlack = juce::Colour (0xff050505);
        const juce::Colour graphite = juce::Colour (0xff1a1a1a);
        const juce::Colour steel = juce::Colour (0xff2a2a2a);
        const juce::Colour neonRed = juce::Colour (0xffff0033);
        const juce::Colour bloodRed = juce::Colour (0xff8a0000);
        const juce::Colour textSilver = juce::Colour (0xffcccccc);
    };

    GothicLookAndFeel gothicLF;

    // Helper to create a knob with numerical entry
    struct PremiumKnob : public juce::Slider
    {
        PremiumKnob()
        {
            setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
            setColour (juce::Slider::textBoxTextColourId, gothicLF.textSilver);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::rotarySliderFillColourId, gothicLF.neonRed);
            setColour (juce::Slider::rotarySliderOutlineColourId, gothicLF.graphite);
            setLookAndFeel (&gothicLF);
        }
        ~PremiumKnob() override { setLookAndFeel (nullptr); }
    };

    struct GothicLabel : public juce::Label
    {
        GothicLabel()
        {
            setJustificationType (juce::Justification::centred);
            setColour (juce::Label::textColourId, gothicLF.textSilver);
            setFont (juce::Font ("Courier New", 12.0f, juce::Font::bold));
        }
    };

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        driveAtt, toneAtt, biasAtt, bodyAtt, mixAtt,
        rateAtt, resAtt, fbAtt, dampAtt, charAtt, waveMixAtt;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        distOnAtt, waveOnAtt;

    // Controls - Distortion
    PremiumKnob driveKnob, toneKnob, biasKnob, bodyKnob, mixKnob;
    GothicLabel driveLbl, toneLbl, biasLbl, bodyLbl, mixLbl;
    juce::TextButton distOnBtn;

    // Controls - Waveguide
    PremiumKnob rateKnob, resKnob, fbKnob, dampKnob, charKnob, waveMixKnob;
    GothicLabel rateLbl, resLbl, fbLbl, dampLbl, charLbl, waveMixLbl;
    juce::TextButton waveOnBtn;

    // Title
    GothicLabel titleLbl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessorEditor)
};