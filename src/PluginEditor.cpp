/*
  ==============================================================================

    PluginEditor.cpp
    WAVERING DISTORTION MOD - GOTHIC RED NEON PREMIUM REDESIGN

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WaveringDistortionModAudioProcessorEditor::WaveringDistortionModAudioProcessorEditor (WaveringDistortionModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set size
    setSize (800, 500);
    
    // Load Font
    loadGothicFont();

    // Create Components
    
    // 1. Knobs with Numeric Input
    // Note: Parameter IDs must match exactly those in PluginProcessor.cpp
    driveKnob = std::make_unique<LabeledKnob>("DRIVE", audioProcessor.getState(), "drive", gothicFont);
    toneKnob  = std::make_unique<LabeledKnob>("TONE", audioProcessor.getState(), "tone", gothicFont);
    biasKnob  = std::make_unique<LabeledKnob>("BIAS", audioProcessor.getState(), "bias", gothicFont);
    bodyKnob  = std::make_unique<LabeledKnob>("BODY", audioProcessor.getState(), "body", gothicFont);
    mixKnob   = std::make_unique<LabeledKnob>("MIX", audioProcessor.getState(), "mix", gothicFont);
    
    addAndMakeVisible(*driveKnob);
    addAndMakeVisible(*toneKnob);
    addAndMakeVisible(*biasKnob);
    addAndMakeVisible(*bodyKnob);
    addAndMakeVisible(*mixKnob);

    // 2. VU Meter
    vuMeter = std::make_unique<CustomVuMeter>(audioProcessor);
    addAndMakeVisible(*vuMeter);

    // 3. Footswitch
    footswitch = std::make_unique<GothicFootswitch>(audioProcessor);
    addAndMakeVisible(*footswitch);

    // Start Timer for VU Meter updates if not handled inside component
    startTimerHz(30);
}

WaveringDistortionModAudioProcessorEditor::~WaveringDistortionModAudioProcessorEditor()
{
    stopTimer();
}

void WaveringDistortionModAudioProcessorEditor::loadGothicFont()
{
    // Attempt to load a specific Gothic font file included in the project resources
    // Ensure 'UnifrakturMaguntia-Regular.ttf' or similar is added to BinaryData or project folder
    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::UnifrakturMaguntiaRegular_ttf, BinaryData::UnifrakturMaguntiaRegular_ttfSize);
    
    if (typeface != nullptr)
    {
        gothicTypeface = typeface;
        gothicFont = juce::Font(gothicTypeface);
    }
    else
    {
        // Fallback to standard Serif Bold if font file is missing
        gothicFont = juce::Font("Times New Roman", 16.0f, juce::Font::bold);
    }
    
    // Adjust font settings for readability
    gothicFont.setExtraKerningFactor(0.05f);
}

void WaveringDistortionModAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Background: Deep Black Metal
    g.fillAll (juce::Colour (BrushedBlack));

    // 2. Brushed Metal Effect (Subtle Gradient)
    juce::ColourGradient metalGrad(
        juce::Colour(0xFF151515), 0, 0,
        juce::Colour(0xFF050505), 0, (float)getHeight(), false);
    g.setGradientFill(metalGrad);
    g.fillRect(getLocalBounds());

    // 3. Decorative Screws (Corners)
    auto screwSize = 12;
    auto drawScrew = [&](int x, int y) {
        g.setColour(juce::Colour(0xFF333333));
        g.fillEllipse(x, y, screwSize, screwSize);
        g.setColour(juce::Colour(0xFF555555));
        g.drawEllipse(x, y, screwSize, screwSize, 1.0f);
        // Cross slot
        g.drawLine(x + 2, y + 2, x + screwSize - 2, y + screwSize - 2, 1.0f);
        g.drawLine(x + screwSize - 2, y + 2, x + 2, y + screwSize - 2, 1.0f);
    };
    
    drawScrew(10, 10);
    drawScrew(getWidth() - 10 - screwSize, 10);
    drawScrew(10, getHeight() - 10 - screwSize);
    drawScrew(getWidth() - 10 - screwSize, getHeight() - 10 - screwSize);

    // 4. Title: WAVERING DISTORTION MOD
    g.setColour(juce::Colours::white);
    g.setFont(gothicFont.withHeight(32.0f));
    g.drawText("WAVERING DISTORTION MOD", getLocalBounds().removeFromTop(60), juce::Justification::centredTop);
    
    // Red Neon Glow under title
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(NeonRed).withAlpha(0.4f), 
        (float)getWidth()/2, 40,
        juce::Colour(NeonRed).withAlpha(0.0f), 
        (float)getWidth()/2, 70, false));
    g.fillRect(0, 40, getWidth(), 30);

    // 5. Panel Borders (Subtle Red Lines)
    g.setColour(juce::Colour(NeonRed).withAlpha(0.3f));
    g.drawRect(getLocalBounds().reduced(20), 1.0f);
}

void WaveringDistortionModAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    // Top Section: Title already handled in paint, reserve space
    area.removeFromTop(60);

    // Main Controls Area
    auto controlsArea = area.removeFromTop(area.getHeight() * 0.6);
    
    // Split into 5 columns for knobs
    auto knobWidth = controlsArea.getWidth() / 5;
    
    driveKnob->setBounds(controlsArea.removeFromLeft(knobWidth));
    toneKnob->setBounds(controlsArea.removeFromLeft(knobWidth));
    biasKnob->setBounds(controlsArea.removeFromLeft(knobWidth));
    bodyKnob->setBounds(controlsArea.removeFromLeft(knobWidth));
    mixKnob->setBounds(controlsArea.removeFromLeft(knobWidth));

    // Bottom Section: VU Meter and Footswitch
    auto bottomArea = area;
    
    // VU Meter (Left-Center)
    auto vuWidth = jmin(200, bottomArea.getWidth() / 3);
    auto vuArea = bottomArea.removeFromLeft(vuWidth).reduced(10);
    vuMeter->setBounds(vuArea);

    // Footswitch (Right-Center)
    auto fsWidth = 100;
    auto fsArea = bottomArea.removeFromRight(fsWidth).reduced(10);
    footswitch->setBounds(fsArea);
}

void WaveringDistortionModAudioProcessorEditor::timerCallback()
{
    // Trigger repaint for VU meter if it doesn't handle its own timer internally
    // In this design, CustomVuMeter has its own timer, but we can sync global updates here if needed
    repaint();
}
