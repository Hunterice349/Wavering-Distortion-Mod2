#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ================= 3D-ручка =================
class MetalKnob : public juce::Slider
{
public:
    explicit MetalKnob (const juce::String& labelText);
    void paint (juce::Graphics&) override;
private:
    juce::String labelText;
};

// ============ VU-метр (стрелка) ============
class VuMeter : public juce::Component
{
public:
    explicit VuMeter (WaveringAudioProcessor& p);
    void update();                       // баллистика + авто-покачивание
    void paint (juce::Graphics&) override;
private:
    WaveringAudioProcessor& proc;
    float needleDb = -40.0f;
    double swayPhase = 0.0;
};

// ========= осциллограф ring-mod =========
class RingScope : public juce::Component
{
public:
    explicit RingScope (WaveringAudioProcessor& p);
    void update();
    void paint (juce::Graphics&) override;
private:
    WaveringAudioProcessor& proc;
    double t = 0.0;
    float amp = 0.0f;
};

// ================= LED =================
class Led : public juce::Component
{
public:
    void setState (juce::Colour c, float glowIn) { colour = c; glow = glowIn; repaint(); }
    void paint (juce::Graphics&) override;
private:
    juce::Colour colour { juce::Colours::green };
    float glow = 0.5f;
};

// =============== BYPASS ================
class BypassSwitch : public juce::Component
{
public:
    explicit BypassSwitch (WaveringAudioProcessor& p);
    void mouseDown (const juce::MouseEvent&) override;
    void paint (juce::Graphics&) override;
private:
    WaveringAudioProcessor& proc;
};

// ================= EDITOR =================
class WaveringAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::Timer
{
public:
    explicit WaveringAudioProcessorEditor (WaveringAudioProcessor&);
    ~WaveringAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    WaveringAudioProcessor& audioProcessor;

    // These are constructed before the editor constructor body can call setSize(),
    // which may synchronously invoke resized().
    std::unique_ptr<VuMeter> vu { std::make_unique<VuMeter> (audioProcessor) };
    std::unique_ptr<RingScope> scope { std::make_unique<RingScope> (audioProcessor) };

    MetalKnob driveKnob { "DRIVE" }, waveKnob { "WAVE" }, ringKnob { "RING" },
              resKnob   { "RES"  }, toneKnob  { "TONE" }, mixKnob  { "MIX" },
              outKnob   { "OUTPUT" };

    BypassSwitch bypass;
    Led powerLed, clipLed;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    juce::Image brushedTex, woodTex;
    double animTime = 0.0;

    void buildTextures();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessorEditor)
};
