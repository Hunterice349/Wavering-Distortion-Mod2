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
    // 1. LookAndFeel ДОЛЖЕН быть объявлен первым, чтобы инициализироваться до контролов
    class GothicLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        GothicLookAndFeel();
        
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override;
        
        juce::Font getLabelFont (juce::Label&) override;
        void drawLabel (juce::Graphics&, juce::Label&) override;
        
        const juce::Colour bgBlack = juce::Colour (0xff050505);
        const juce::Colour graphite = juce::Colour (0xff1a1a1a);
        const juce::Colour steel = juce::Colour (0xff2a2a2a);
        const juce::Colour neonRed = juce::Colour (0xffff0033);
        const juce::Colour bloodRed = juce::Colour (0xff8a0000);
        const juce::Colour textSilver = juce::Colour (0xffcccccc);
    };

    GothicLookAndFeel gothicLF;

    // 2. Кастомные структуры контролов
    struct PremiumKnob : public juce::Slider
    {
        PremiumKnob()
        {
            setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
            // LookAndFeel будет назначен в конструкторе редактора
        }
    };

    struct GothicLabel : public juce::Label
    {
        GothicLabel()
        {
            setJustificationType (juce::Justification::centred);
            setFont (juce::Font ("Courier New", 12.0f, juce::Font::bold));
        }
    };

    // 3. Привязки параметров (APVTS)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        driveAtt, toneAtt, biasAtt, bodyAtt, mixAtt,
        rateAtt, resAtt, fbAtt, dampAtt, charAtt, waveMixAtt;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        distOnAtt, waveOnAtt;

    // 4. Контролы: Distortion
    PremiumKnob driveKnob, toneKnob, biasKnob, bodyKnob, mixKnob;
    GothicLabel driveLbl, toneLbl, biasLbl, bodyLbl, mixLbl;
    juce::TextButton distOnBtn;

    // 5. Контролы: Waveguide
    PremiumKnob rateKnob, resKnob, fbKnob, dampKnob, charKnob, waveMixKnob;
    GothicLabel rateLbl, resLbl, fbLbl, dampLbl, charLbl, waveMixLbl;
    juce::TextButton waveOnBtn;

    // 6. Заголовок и зоны макета
    GothicLabel titleLbl;
    juce::Rectangle<int> distortionArea, waveguideArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessorEditor)
};