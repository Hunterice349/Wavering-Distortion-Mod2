#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
// GothicLookAndFeel Implementation
//==============================================================================

WaveringAudioProcessorEditor::GothicLookAndFeel::GothicLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId, graphite);
    setColour (juce::TextButton::buttonOnColourId, neonRed);
    setColour (juce::TextButton::textColourOnId, bgBlack);
    setColour (juce::TextButton::textColourOffId, textSilver);
}

void WaveringAudioProcessorEditor::GothicLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = graphite;
    auto fill = neonRed;
    auto indicator = bloodRed;

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (1.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmax (2.0f, radius * 0.08f);
    auto arcRadius = radius - lineW * 2.0f;

    // 1. Draw Base (Dark Graphite/Metal)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                 arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Draw Active Arc (Neon Red Glow)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, toAngle, true);
        
        // Neon Glow Effect
        g.setColour (fill.withAlpha (0.3f));
        g.strokePath (valueArc, juce::PathStrokeType (lineW + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Draw Knob Body (Photorealistic Metal Simulation)
    juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                   bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));

    juce::Path knobCircle;
    knobCircle.addEllipse (bounds.reduced (radius * 0.4f));
    
    // Metallic Gradient
    juce::ColourGradient grad (juce::Colours::white.withAlpha(0.1f), bounds.getCentreX(), bounds.getY(),
                               juce::Colours::black.withAlpha(0.5f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (knobCircle);
    
    // Border
    g.setColour (steel);
    g.strokePath (knobCircle, juce::PathStrokeType (1.0f));

    // 4. Indicator Line (Crimson Neon)
    juce::Path indicatorLine;
    indicatorLine.startNewSubPath (bounds.getCentre());
    indicatorLine.lineTo (thumbPoint);
    g.setColour (indicator);
    g.strokePath (indicatorLine, juce::PathStrokeType (2.0f));
}

juce::Font WaveringAudioProcessorEditor::GothicLookAndFeel::getLabelFont (juce::Label& label)
{
    return juce::Font ("Courier New", 14.0f, juce::Font::bold);
}

void WaveringAudioProcessorEditor::GothicLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto area = label.getLocalBounds().reduced (2);
        g.setColour (label.findColour (juce::Label::textColourId));
        g.setFont (getLabelFont (label));
        g.drawText (label.getText(), area, label.getJustificationType(), true);
        
        // Draw subtle border for input field
        g.setColour (steel);
        g.drawRect (area, 1.0f);
    }
}

//==============================================================================
// Editor Implementation
//==============================================================================

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor (WaveringAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    // Set LookAndFeel
    setLookAndFeel (&gothicLF);
    setOpaque (true);

    // --- Distortion Section ---
    addAndMakeVisible (driveKnob);
    driveAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "drive", driveKnob));
    addAndMakeVisible (driveLbl);
    driveLbl.setText ("DRIVE", juce::dontSendNotification);

    addAndMakeVisible (toneKnob);
    toneAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "tone", toneKnob));
    addAndMakeVisible (toneLbl);
    toneLbl.setText ("TONE", juce::dontSendNotification);

    addAndMakeVisible (biasKnob);
    biasAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "bias", biasKnob));
    addAndMakeVisible (biasLbl);
    biasLbl.setText ("BIAS", juce::dontSendNotification);

    addAndMakeVisible (bodyKnob);
    bodyAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "body", bodyKnob));
    addAndMakeVisible (bodyLbl);
    bodyLbl.setText ("BODY", juce::dontSendNotification);

    addAndMakeVisible (mixKnob);
    mixAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "mix", mixKnob));
    addAndMakeVisible (mixLbl);
    mixLbl.setText ("MIX", juce::dontSendNotification);

    addAndMakeVisible (distOnBtn);
    distOnBtn.setButtonText ("DISTORTION ON");
    distOnBtn.setClickingTogglesState (true);
    distOnAtt.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "distortionEnabled", distOnBtn));

    // --- Waveguide Section ---
    addAndMakeVisible (rateKnob);
    rateAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveRate", rateKnob));
    addAndMakeVisible (rateLbl);
    rateLbl.setText ("RATE", juce::dontSendNotification);

    addAndMakeVisible (resKnob);
    resAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveResonance", resKnob));
    addAndMakeVisible (resLbl);
    resLbl.setText ("RESONANCE", juce::dontSendNotification);

    addAndMakeVisible (fbKnob);
    fbAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveFeedback", fbKnob));
    addAndMakeVisible (fbLbl);
    fbLbl.setText ("FEEDBACK", juce::dontSendNotification);

    addAndMakeVisible (dampKnob);
    dampAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveDamping", dampKnob));
    addAndMakeVisible (dampLbl);
    dampLbl.setText ("DAMPING", juce::dontSendNotification);

    addAndMakeVisible (charKnob);
    charAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveCharacter", charKnob));
    addAndMakeVisible (charLbl);
    charLbl.setText ("CHARACTER", juce::dontSendNotification);

    addAndMakeVisible (waveMixKnob);
    waveMixAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveMix", waveMixKnob));
    addAndMakeVisible (waveMixLbl);
    waveMixLbl.setText ("WAVE MIX", juce::dontSendNotification);

    addAndMakeVisible (waveOnBtn);
    waveOnBtn.setButtonText ("WAVEGUIDE ON");
    waveOnBtn.setClickingTogglesState (true);
    waveOnAtt.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "waveguideEnabled", waveOnBtn));

    // --- Title ---
    addAndMakeVisible (titleLbl);
    titleLbl.setText ("WAVERING DISTORTION MOD2", juce::dontSendNotification);
    titleLbl.setFont (juce::Font ("Courier New", 24.0f, juce::Font::bold));
    titleLbl.setColour (juce::Label::textColourId, gothicLF.neonRed);

    setSize (900, 500);
    startTimerHz (30);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Background: Deep Black/Graphite Gradient
    juce::ColourGradient bgGrad (gothicLF.bgBlack, 0, 0,
                                 gothicLF.graphite, 0, (float) getHeight(), false);
    g.setFillType (bgGrad);
    g.fillAll();

    // 2. Panel Backgrounds (Metallic Plates)
    auto drawPanel = [&g, &gothicLF] (juce::Rectangle<int> bounds, const juce::String& title)
    {
        // Plate Gradient
        juce::ColourGradient plateGrad (gothicLF.steel, bounds.getX(), bounds.getY(),
                                        gothicLF.graphite, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (plateGrad);
        g.fillRoundedRectangle (bounds.toFloat(), 10.0f);
        
        // Border
        g.setColour (gothicLF.bloodRed.withAlpha(0.5f));
        g.drawRoundedRectangle (bounds.toFloat(), 10.0f, 2.0f);
        
        // Inner Shadow/Bevel
        g.setColour (juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle (bounds.reduced(2).toFloat(), 10.0f, 1.0f);
    };

    // Distortion Panel
    drawPanel (distortionArea, "DISTORTION MODULE");
    // Waveguide Panel
    drawPanel (waveguideArea, "WAVEGUIDE MODULE");

    // 3. Screws (Decorative)
    auto drawScrew = [&g] (int x, int y)
    {
        g.setColour (juce::Colours::darkgrey);
        g.fillEllipse (x-4, y-4, 8, 8);
        g.setColour (juce::Colours::silver);
        g.fillEllipse (x-2, y-2, 4, 4);
    };
    
    drawScrew (10, 10);
    drawScrew (getWidth()-10, 10);
    drawScrew (10, getHeight()-10);
    drawScrew (getWidth()-10, getHeight()-10);
}

void WaveringAudioProcessorEditor::resized()
{
    // Layout Constants
    const int panelMargin = 20;
    const int knobSize = 70;
    const int labelHeight = 20;
    const int btnHeight = 30;
    
    // Define Panels
    distortionArea = getLocalBounds().removeFromLeft (getWidth() / 2).reduced (panelMargin);
    waveguideArea = getLocalBounds().reduced (panelMargin);
    
    // Title
    titleLbl.setBounds (getLocalBounds().removeFromTop (40));

    // --- Distortion Layout ---
    auto distRow = distortionArea.removeFromTop (knobSize + labelHeight + 10);
    driveKnob.setBounds (distRow.removeFromLeft (knobSize + 60));
    driveLbl.setBounds (driveKnob.getBounds().removeFromBottom (labelHeight));
    
    toneKnob.setBounds (distRow.removeFromLeft (knobSize + 60));
    toneLbl.setBounds (toneKnob.getBounds().removeFromBottom (labelHeight));
    
    biasKnob.setBounds (distRow.removeFromLeft (knobSize + 60));
    biasLbl.setBounds (biasKnob.getBounds().removeFromBottom (labelHeight));
    
    bodyKnob.setBounds (distRow.removeFromLeft (knobSize + 60));
    bodyLbl.setBounds (bodyKnob.getBounds().removeFromBottom (labelHeight));
    
    mixKnob.setBounds (distRow.removeFromLeft (knobSize + 60));
    mixLbl.setBounds (mixKnob.getBounds().removeFromBottom (labelHeight));

    distOnBtn.setBounds (distortionArea.removeFromBottom (btnHeight).reduced (20, 5));

    // --- Waveguide Layout ---
    auto waveRow = waveguideArea.removeFromTop (knobSize + labelHeight + 10);
    rateKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    rateLbl.setBounds (rateKnob.getBounds().removeFromBottom (labelHeight));
    
    resKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    resLbl.setBounds (resKnob.getBounds().removeFromBottom (labelHeight));
    
    fbKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    fbLbl.setBounds (fbKnob.getBounds().removeFromBottom (labelHeight));
    
    dampKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    dampLbl.setBounds (dampKnob.getBounds().removeFromBottom (labelHeight));
    
    charKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    charLbl.setBounds (charKnob.getBounds().removeFromBottom (labelHeight));
    
    waveMixKnob.setBounds (waveRow.removeFromLeft (knobSize + 60));
    waveMixLbl.setBounds (waveMixKnob.getBounds().removeFromBottom (labelHeight));

    waveOnBtn.setBounds (waveguideArea.removeFromBottom (btnHeight).reduced (20, 5));
}

void WaveringAudioProcessorEditor::timerCallback()
{
    // Optional: Add subtle pulsing to neon elements if desired
}