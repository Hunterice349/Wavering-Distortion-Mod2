
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cmath>

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
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPos,
    const float rotaryStartAngle,
    const float rotaryEndAngle,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height)
                      .toFloat().reduced (3.0f);

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float lineW = juce::jmax (2.0f, radius * 0.075f);
    const float arcRadius = juce::jmax (1.0f, radius - lineW * 2.0f);

    const float angle = rotaryStartAngle
                      + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto centre = bounds.getCentre();

    // Dark rotary track.
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (
        centre.x, centre.y,
        arcRadius, arcRadius,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    g.setColour (graphite.brighter (0.15f));
    g.strokePath (
        backgroundArc,
        juce::PathStrokeType (
            lineW,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Red neon active arc.
    if (slider.isEnabled() && sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (
            centre.x, centre.y,
            arcRadius, arcRadius,
            0.0f,
            rotaryStartAngle,
            angle,
            true);

        g.setColour (neonRed.withAlpha (0.16f));
        g.strokePath (
            valueArc,
            juce::PathStrokeType (lineW + 9.0f));

        g.setColour (neonRed.withAlpha (0.4f));
        g.strokePath (
            valueArc,
            juce::PathStrokeType (lineW + 4.0f));

        g.setColour (neonRed);
        g.strokePath (
            valueArc,
            juce::PathStrokeType (
                lineW,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
    }

    // Metallic knob body.
    const auto knobBounds = bounds.reduced (radius * 0.34f);

    juce::Path knob;
    knob.addEllipse (knobBounds);

    juce::ColourGradient metalGradient (
        juce::Colour (0xff555555),
        knobBounds.getCentreX(),
        knobBounds.getY(),
        juce::Colour (0xff101010),
        knobBounds.getCentreX(),
        knobBounds.getBottom(),
        false);

    g.setGradientFill (metalGradient);
    g.fillPath (knob);

    // Metallic outer rim.
    g.setColour (steel.brighter (0.35f));
    g.strokePath (knob, juce::PathStrokeType (1.4f));

    // Inner dark surface.
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillEllipse (knobBounds.reduced (3.0f));

    // Indicator.
    const float indicatorRadius = knobBounds.getWidth() * 0.34f;

    juce::Point<float> indicatorPoint (
        centre.x + indicatorRadius * std::cos (
            angle - juce::MathConstants<float>::halfPi),
        centre.y + indicatorRadius * std::sin (
            angle - juce::MathConstants<float>::halfPi));

    juce::Path indicator;
    indicator.startNewSubPath (centre);
    indicator.lineTo (indicatorPoint);

    g.setColour (bloodRed.withAlpha (0.55f));
    g.strokePath (indicator, juce::PathStrokeType (5.0f));

    g.setColour (neonRed);
    g.strokePath (
        indicator,
        juce::PathStrokeType (
            2.2f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (neonRed);
    g.fillEllipse (
        centre.x - 2.5f,
        centre.y - 2.5f,
        5.0f,
        5.0f);

    // Small metallic highlight.
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (
        knobBounds.getX() + 7.0f,
        knobBounds.getY() + 5.0f,
        knobBounds.getRight() - 7.0f,
        knobBounds.getY() + 5.0f,
        1.0f);
}

juce::Font WaveringAudioProcessorEditor::GothicLookAndFeel::getLabelFont (
    juce::Label& label)
{
    return label.getFont();
}

void WaveringAudioProcessorEditor::GothicLookAndFeel::drawLabel (
    juce::Graphics& g,
    juce::Label& label)
{
    if (! label.isBeingEdited())
    {
        auto area = label.getLocalBounds().reduced (2);

        g.setColour (label.findColour (juce::Label::textColourId));
        g.setFont (label.getFont());

        g.drawText (
            label.getText(),
            area,
            label.getJustificationType(),
            true);
    }
}

//==============================================================================
// Editor Constructor
//==============================================================================

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor (
    WaveringAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    setLookAndFeel (&gothicLF);
    setOpaque (true);

    // Configure rotary controls.
    auto setupKnob = [this] (PremiumKnob& knob)
    {
        knob.setSliderStyle (
            juce::Slider::RotaryHorizontalVerticalDrag);

        // Editable numerical values.
        knob.setTextBoxStyle (
            juce::Slider::TextBoxBelow,
            false,
            60,
            20);

        knob.setColour (
            juce::Slider::textBoxTextColourId,
            gothicLF.textSilver);

        knob.setColour (
            juce::Slider::textBoxBackgroundColourId,
            juce::Colour (0xff101010));

        knob.setColour (
            juce::Slider::textBoxOutlineColourId,
            gothicLF.bloodRed);

        knob.setColour (
            juce::Slider::rotarySliderFillColourId,
            gothicLF.neonRed);

        knob.setColour (
            juce::Slider::rotarySliderOutlineColourId,
            gothicLF.graphite);

        knob.setLookAndFeel (&gothicLF);
    };

    // Configure labels.
    auto setupLabel = [this] (GothicLabel& label)
    {
        label.setJustificationType (
            juce::Justification::centred);

        label.setFont (
            juce::Font (
                "Courier New",
                12.0f,
                juce::Font::bold));

        label.setColour (
            juce::Label::textColourId,
            gothicLF.textSilver);
    };

    //==========================================================================
    // DISTORTION
    //==========================================================================

    addAndMakeVisible (driveKnob);
    setupKnob (driveKnob);
    driveAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "drive", driveKnob));

    addAndMakeVisible (driveLbl);
    driveLbl.setText ("DRIVE", juce::dontSendNotification);
    setupLabel (driveLbl);

    addAndMakeVisible (toneKnob);
    setupKnob (toneKnob);
    toneAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "tone", toneKnob));

    addAndMakeVisible (toneLbl);
    toneLbl.setText ("TONE", juce::dontSendNotification);
    setupLabel (toneLbl);

    addAndMakeVisible (biasKnob);
    setupKnob (biasKnob);
    biasAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "bias", biasKnob));

    addAndMakeVisible (biasLbl);
    biasLbl.setText ("BIAS", juce::dontSendNotification);
    setupLabel (biasLbl);

    addAndMakeVisible (bodyKnob);
    setupKnob (bodyKnob);
    bodyAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "body", bodyKnob));

    addAndMakeVisible (bodyLbl);
    bodyLbl.setText ("BODY", juce::dontSendNotification);
    setupLabel (bodyLbl);

    addAndMakeVisible (mixKnob);
    setupKnob (mixKnob);
    mixAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "mix", mixKnob));

    addAndMakeVisible (mixLbl);
    mixLbl.setText ("MIX", juce::dontSendNotification);
    setupLabel (mixLbl);

    addAndMakeVisible (distOnBtn);
    distOnBtn.setButtonText ("DISTORTION ON");
    distOnBtn.setClickingTogglesState (true);
    distOnBtn.setColour (
        juce::TextButton::buttonColourId,
        gothicLF.graphite);
    distOnBtn.setColour (
        juce::TextButton::buttonOnColourId,
        gothicLF.neonRed);
    distOnBtn.setColour (
        juce::TextButton::textColourOffId,
        gothicLF.textSilver);
    distOnBtn.setColour (
        juce::TextButton::textColourOnId,
        gothicLF.bgBlack);

    distOnAtt.reset (
        new juce::AudioProcessorValueTreeState::ButtonAttachment (
            p.apvts, "distortionEnabled", distOnBtn));

    //==========================================================================
    // WAVEGUIDE
    //==========================================================================

    addAndMakeVisible (rateKnob);
    setupKnob (rateKnob);
    rateAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveRate", rateKnob));

    addAndMakeVisible (rateLbl);
    rateLbl.setText ("RATE", juce::dontSendNotification);
    setupLabel (rateLbl);

    addAndMakeVisible (resKnob);
    setupKnob (resKnob);
    resAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveResonance", resKnob));

    addAndMakeVisible (resLbl);
    resLbl.setText ("RESONANCE", juce::dontSendNotification);
    setupLabel (resLbl);

    addAndMakeVisible (fbKnob);
    setupKnob (fbKnob);
    fbAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveFeedback", fbKnob));

    addAndMakeVisible (fbLbl);
    fbLbl.setText ("FEEDBACK", juce::dontSendNotification);
    setupLabel (fbLbl);

    addAndMakeVisible (dampKnob);
    setupKnob (dampKnob);
    dampAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveDamping", dampKnob));

    addAndMakeVisible (dampLbl);
    dampLbl.setText ("DAMPING", juce::dontSendNotification);
    setupLabel (dampLbl);

    addAndMakeVisible (charKnob);
    setupKnob (charKnob);
    charAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveCharacter", charKnob));

    addAndMakeVisible (charLbl);
    charLbl.setText ("CHARACTER", juce::dontSendNotification);
    setupLabel (charLbl);

    addAndMakeVisible (waveMixKnob);
    setupKnob (waveMixKnob);
    waveMixAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts, "waveMix", waveMixKnob));

    addAndMakeVisible (waveMixLbl);
    waveMixLbl.setText ("WAVE MIX", juce::dontSendNotification);
    setupLabel (waveMixLbl);

    addAndMakeVisible (waveOnBtn);
    waveOnBtn.setButtonText ("WAVEGUIDE ON");
    waveOnBtn.setClickingTogglesState (true);
    waveOnBtn.setColour (
        juce::TextButton::buttonColourId,
        gothicLF.graphite);
    waveOnBtn.setColour (
        juce::TextButton::buttonOnColourId,
        gothicLF.neonRed);
    waveOnBtn.setColour (
        juce::TextButton::textColourOffId,
        gothicLF.textSilver);
    waveOnBtn.setColour (
        juce::TextButton::textColourOnId,
        gothicLF.bgBlack);

    waveOnAtt.reset (
        new juce::AudioProcessorValueTreeState::ButtonAttachment (
            p.apvts, "waveguideEnabled", waveOnBtn));

    //==========================================================================
    // TITLE
    //==========================================================================

    addAndMakeVisible (titleLbl);

    titleLbl.setText (
        "WAVERING DISTORTION MOD2",
        juce::dontSendNotification);

    titleLbl.setFont (
        juce::Font (
            "Courier New",
            22.0f,
            juce::Font::bold));

    titleLbl.setColour (
        juce::Label::textColourId,
        gothicLF.neonRed);

    titleLbl.setJustificationType (
        juce::Justification::centred);

    setSize (900, 520);

    startTimerHz (30);
}

//==============================================================================
// Destructor
//==============================================================================

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
// Paint
//==============================================================================

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Main black / graphite background.
    juce::ColourGradient background (
        gothicLF.bgBlack,
        0.0f,
        0.0f,
        gothicLF.graphite,
        0.0f,
        static_cast<float> (getHeight()),
        false);

    g.setGradientFill (background);
    g.fillAll();

    // Subtle background border.
    g.setColour (gothicLF.bloodRed.withAlpha (0.35f));
    g.drawRect (getLocalBounds(), 1);

    // Module panels.
    auto drawPanel = [this, &g] (
        juce::Rectangle<int> bounds,
        const juce::String& panelTitle)
    {
        juce::ColourGradient plateGradient (
            gothicLF.steel,
            static_cast<float> (bounds.getX()),
            static_cast<float> (bounds.getY()),
            gothicLF.bgBlack,
            static_cast<float> (bounds.getX()),
            static_cast<float> (bounds.getBottom()),
            false);

        g.setGradientFill (plateGradient);

        g.fillRoundedRectangle (
            bounds.toFloat(),
            9.0f);

        // Outer red edge.
        g.setColour (
            gothicLF.bloodRed.withAlpha (0.85f));

        g.drawRoundedRectangle (
            bounds.toFloat(),
            9.0f,
            1.6f);

        // Inner metallic edge.
        g.setColour (
            gothicLF.steel.brighter (0.35f));

        g.drawRoundedRectangle (
            bounds.reduced (3).toFloat(),
            7.0f,
            0.8f);

        // Panel heading.
        auto heading = bounds.reduced (12);
        heading.removeFromBottom (bounds.getHeight() - 34);
        heading.setHeight (24);

        g.setColour (gothicLF.neonRed);
        g.setFont (
            juce::Font (
                "Courier New",
                13.0f,
                juce::Font::bold));

        g.drawText (
            panelTitle,
            heading,
            juce::Justification::centred,
            false);

        // Separator under heading.
        const int separatorY = bounds.getY() + 39;

        g.setColour (
            gothicLF.bloodRed.withAlpha (0.7f));

        g.drawLine (
            static_cast<float> (bounds.getX() + 14),
            static_cast<float> (separatorY),
            static_cast<float> (bounds.getRight() - 14),
            static_cast<float> (separatorY),
            1.0f);
    };

    drawPanel (
        distortionArea,
        "I   /   DISTORTION");

    drawPanel (
        waveguideArea,
        "II   /   WAVEGUIDE");

    // Decorative metallic screws.
    auto drawScrew = [&g] (int x, int y)
    {
        g.setColour (juce::Colour (0xff080808));
        g.fillEllipse (
            static_cast<float> (x - 5),
            static_cast<float> (y - 5),
            10.0f,
            10.0f);

        g.setColour (juce::Colour (0xff777777));
        g.drawEllipse (
            static_cast<float> (x - 4),
            static_cast<float> (y - 4),
            8.0f,
            8.0f,
            1.0f);

        g.setColour (juce::Colour (0xffaaaaaa));
        g.drawLine (
            static_cast<float> (x - 2),
            static_cast<float> (y + 2),
            static_cast<float> (x + 2),
            static_cast<float> (y - 2),
            1.0f);
    };

    drawScrew (12, 12);
    drawScrew (getWidth() - 12, 12);
    drawScrew (12, getHeight() - 12);
    drawScrew (getWidth() - 12, getHeight() - 12);
}

//==============================================================================
// Layout
//==============================================================================

void WaveringAudioProcessorEditor::resized()
{
    const int outerMargin = 18;
    const int panelGap = 12;
    const int panelTop = 64;
    const int panelBottom = 14;

    // Header.
    titleLbl.setBounds (
        25,
        12,
        getWidth() - 50,
        40);

    // Two independent panels.
    auto available = getLocalBounds()
                         .withTrimmedTop (panelTop)
                         .withTrimmedBottom (panelBottom)
                         .reduced (outerMargin, 0);

    const int panelWidth =
        (available.getWidth() - panelGap) / 2;

    distortionArea = available.removeFromLeft (panelWidth);

    available.removeFromLeft (panelGap);

    waveguideArea = available;

    // Reusable control placement.
    auto placeControl = [] (
        PremiumKnob& knob,
        GothicLabel& label,
        juce::Rectangle<int> cell)
    {
        auto labelArea = cell.removeFromTop (22);

        label.setBounds (
            labelArea.reduced (2, 0));

        const int sliderWidth = 82;
        const int sliderHeight = 108;

        auto sliderArea = cell;

        auto sliderBounds = juce::Rectangle<int> (
            0,
            0,
            sliderWidth,
            sliderHeight);

        sliderBounds.setCentre (
            sliderArea.getCentre());

        knob.setBounds (sliderBounds);
    };

    // Prepare inner module areas.
    auto makeControlArea = [] (
        juce::Rectangle<int> panel)
    {
        panel = panel.reduced (12);
        panel.removeFromTop (43);
        panel.removeFromBottom (42);
        return panel;
    };

    //==========================================================================
    // DISTORTION GRID: 3 + 2 CONTROLS
    //==========================================================================

    auto distControls = makeControlArea (distortionArea);

    const int distCellWidth =
        distControls.getWidth() / 3;

    const int distCellHeight =
        distControls.getHeight() / 2;

    auto distCell = [&] (int column, int row)
    {
        return juce::Rectangle<int> (
            distControls.getX() + column * distCellWidth,
            distControls.getY() + row * distCellHeight,
            distCellWidth,
            distCellHeight);
    };

    placeControl (driveKnob, driveLbl, distCell (0, 0));
    placeControl (toneKnob, toneLbl, distCell (1, 0));
    placeControl (biasKnob, biasLbl, distCell (2, 0));

    placeControl (bodyKnob, bodyLbl, distCell (0, 1));
    placeControl (mixKnob, mixLbl, distCell (1, 1));

    distOnBtn.setBounds (
        distortionArea.getX() + 20,
        distortionArea.getBottom() - 38,
        distortionArea.getWidth() - 40,
        26);

    //==========================================================================
    // WAVEGUIDE GRID: 3 + 3 CONTROLS
    //==========================================================================

    auto waveControls = makeControlArea (waveguideArea);

    const int waveCellWidth =
        waveControls.getWidth() / 3;

    const int waveCellHeight =
        waveControls.getHeight() / 2;

    auto waveCell = [&] (int column, int row)
    {
        return juce::Rectangle<int> (
            waveControls.getX() + column * waveCellWidth,
            waveControls.getY() + row * waveCellHeight,
            waveCellWidth,
            waveCellHeight);
    };

    placeControl (rateKnob, rateLbl, waveCell (0, 0));
    placeControl (resKnob, resLbl, waveCell (1, 0));
    placeControl (fbKnob, fbLbl, waveCell (2, 0));

    placeControl (dampKnob, dampLbl, waveCell (0, 1));
    placeControl (charKnob, charLbl, waveCell (1, 1));
    placeControl (waveMixKnob, waveMixLbl, waveCell (2, 1));

    waveOnBtn.setBounds (
        waveguideArea.getX() + 20,
        waveguideArea.getBottom() - 38,
        waveguideArea.getWidth() - 40,
        26);
}

//==============================================================================
// Timer
//==============================================================================

void WaveringAudioProcessorEditor::timerCallback()
{
    // Reserved for future interface animation.
}
