
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>

//==============================================================================
// Gothic Look And Feel
//==============================================================================

WaveringAudioProcessorEditor::GothicLookAndFeel::GothicLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId, graphite);
    setColour (juce::TextButton::buttonOnColourId, neonRed);
    setColour (juce::TextButton::textColourOnId, bgBlack);
    setColour (juce::TextButton::textColourOffId, textSilver);

    setColour (juce::Label::textColourId, textSilver);
    setColour (
        juce::Label::backgroundColourId,
        juce::Colours::transparentBlack);
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
    juce::Slider&)
{
    auto bounds = juce::Rectangle<float> (
        static_cast<float> (x),
        static_cast<float> (y),
        static_cast<float> (width),
        static_cast<float> (height)).reduced (3.0f);

    const float radius = juce::jmin (
        bounds.getWidth(),
        bounds.getHeight()) * 0.5f;

    const auto centre = bounds.getCentre();

    const float angle = rotaryStartAngle
                      + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float ringRadius = radius * 0.91f;
    const float lineWidth = juce::jmax (2.0f, radius * 0.09f);

    // Dark outer chassis.
    g.setColour (juce::Colours::black);
    g.fillEllipse (bounds);

    // Metallic outer rim.
    juce::ColourGradient rimGradient (
        juce::Colour (0xff777777),
        centre.x,
        bounds.getY(),
        juce::Colour (0xff111111),
        centre.x,
        bounds.getBottom(),
        false);

    g.setGradientFill (rimGradient);
    g.fillEllipse (bounds.reduced (1.0f));

    // Background arc.
    juce::Path backgroundArc;

    backgroundArc.addCentredArc (
        centre.x,
        centre.y,
        ringRadius,
        ringRadius,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    g.setColour (juce::Colour (0xff292929));

    g.strokePath (
        backgroundArc,
        juce::PathStrokeType (
            lineWidth,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Red neon value arc.
    juce::Path valueArc;

    valueArc.addCentredArc (
        centre.x,
        centre.y,
        ringRadius,
        ringRadius,
        0.0f,
        rotaryStartAngle,
        angle,
        true);

    g.setColour (neonRed.withAlpha (0.12f));
    g.strokePath (
        valueArc,
        juce::PathStrokeType (lineWidth + 9.0f));

    g.setColour (bloodRed.withAlpha (0.65f));
    g.strokePath (
        valueArc,
        juce::PathStrokeType (lineWidth + 3.0f));

    g.setColour (neonRed);

    g.strokePath (
        valueArc,
        juce::PathStrokeType (
            lineWidth,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Knob body.
    auto knobBounds = bounds.reduced (radius * 0.30f);

    juce::ColourGradient knobGradient (
        juce::Colour (0xff555555),
        knobBounds.getX(),
        knobBounds.getY(),
        juce::Colour (0xff101010),
        knobBounds.getRight(),
        knobBounds.getBottom(),
        false);

    g.setGradientFill (knobGradient);
    g.fillEllipse (knobBounds);

    // Inner metallic edge.
    g.setColour (juce::Colour (0xff858585).withAlpha (0.65f));
    g.drawEllipse (knobBounds, 1.2f);

    // Subtle inner highlight.
    // JUCE Graphics has no drawArc(), so use Path::addArc().
    juce::Path highlightArc;

    highlightArc.addArc (
        knobBounds.getX() + 3.0f,
        knobBounds.getY() + 3.0f,
        knobBounds.getWidth() - 6.0f,
        knobBounds.getHeight() - 6.0f,
        juce::MathConstants<float>::pi * 1.15f,
        juce::MathConstants<float>::pi * 1.85f,
        true);

    g.setColour (juce::Colours::white.withAlpha (0.08f));

    g.strokePath (
        highlightArc,
        juce::PathStrokeType (2.0f));

    // Indicator.
    const juce::Point<float> indicatorPoint (
        centre.x + std::cos (
            angle - juce::MathConstants<float>::halfPi)
            * radius * 0.52f,

        centre.y + std::sin (
            angle - juce::MathConstants<float>::halfPi)
            * radius * 0.52f);

    juce::Path indicator;

    indicator.startNewSubPath (centre);
    indicator.lineTo (indicatorPoint);

    g.setColour (neonRed.withAlpha (0.25f));
    g.strokePath (
        indicator,
        juce::PathStrokeType (6.0f));

    g.setColour (neonRed);

    g.strokePath (
        indicator,
        juce::PathStrokeType (
            2.5f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xffff6680));

    g.fillEllipse (
        centre.x - 2.5f,
        centre.y - 2.5f,
        5.0f,
        5.0f);
}

juce::Font WaveringAudioProcessorEditor::GothicLookAndFeel::getLabelFont (
    juce::Label&)
{
    return juce::Font (
        "Georgia",
        13.0f,
        juce::Font::bold);
}

void WaveringAudioProcessorEditor::GothicLookAndFeel::drawLabel (
    juce::Graphics& g,
    juce::Label& label)
{
    if (label.isBeingEdited())
        return;

    auto bounds = label.getLocalBounds();

    g.setColour (
        label.findColour (juce::Label::textColourId));

    g.setFont (getLabelFont (label));

    g.drawFittedText (
        label.getText(),
        bounds,
        label.getJustificationType(),
        1);
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

    auto setupKnob = [this] (PremiumKnob& knob)
    {
        knob.setSliderStyle (
            juce::Slider::RotaryHorizontalVerticalDrag);

        knob.setTextBoxStyle (
            juce::Slider::TextBoxBelow,
            false,
            58,
            20);

        knob.setColour (
            juce::Slider::textBoxTextColourId,
            gothicLF.textSilver);

        knob.setColour (
            juce::Slider::textBoxBackgroundColourId,
            juce::Colour (0xff090909));

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

    auto setupLabel = [this] (
        GothicLabel& label,
        const juce::String& text)
    {
        label.setText (
            text,
            juce::dontSendNotification);

        label.setJustificationType (
            juce::Justification::centred);

        label.setColour (
            juce::Label::textColourId,
            gothicLF.textSilver);

        label.setLookAndFeel (&gothicLF);
    };

    // Distortion controls.
    addAndMakeVisible (driveKnob);
    setupKnob (driveKnob);

    driveAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "drive",
            driveKnob));

    addAndMakeVisible (driveLbl);
    setupLabel (driveLbl, "DRIVE");

    addAndMakeVisible (toneKnob);
    setupKnob (toneKnob);

    toneAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "tone",
            toneKnob));

    addAndMakeVisible (toneLbl);
    setupLabel (toneLbl, "TONE");

    addAndMakeVisible (biasKnob);
    setupKnob (biasKnob);

    biasAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "bias",
            biasKnob));

    addAndMakeVisible (biasLbl);
    setupLabel (biasLbl, "BIAS");

    addAndMakeVisible (bodyKnob);
    setupKnob (bodyKnob);

    bodyAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "body",
            bodyKnob));

    addAndMakeVisible (bodyLbl);
    setupLabel (bodyLbl, "BODY");

    addAndMakeVisible (mixKnob);
    setupKnob (mixKnob);

    mixAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "mix",
            mixKnob));

    addAndMakeVisible (mixLbl);
    setupLabel (mixLbl, "MIX");

    addAndMakeVisible (distOnBtn);

    distOnBtn.setButtonText ("DISTORTION  ON");
    distOnBtn.setClickingTogglesState (true);

    distOnBtn.setColour (
        juce::TextButton::buttonColourId,
        gothicLF.graphite);

    distOnBtn.setColour (
        juce::TextButton::buttonOnColourId,
        gothicLF.neonRed);

    distOnAtt.reset (
        new juce::AudioProcessorValueTreeState::ButtonAttachment (
            p.apvts,
            "distortionEnabled",
            distOnBtn));

    // Waveguide controls.
    addAndMakeVisible (rateKnob);
    setupKnob (rateKnob);

    rateAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveRate",
            rateKnob));

    addAndMakeVisible (rateLbl);
    setupLabel (rateLbl, "RATE");

    addAndMakeVisible (resKnob);
    setupKnob (resKnob);

    resAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveResonance",
            resKnob));

    addAndMakeVisible (resLbl);
    setupLabel (resLbl, "RESONANCE");

    addAndMakeVisible (fbKnob);
    setupKnob (fbKnob);

    fbAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveFeedback",
            fbKnob));

    addAndMakeVisible (fbLbl);
    setupLabel (fbLbl, "FEEDBACK");

    addAndMakeVisible (dampKnob);
    setupKnob (dampKnob);

    dampAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveDamping",
            dampKnob));

    addAndMakeVisible (dampLbl);
    setupLabel (dampLbl, "DAMPING");

    addAndMakeVisible (charKnob);
    setupKnob (charKnob);

    charAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveCharacter",
            charKnob));

    addAndMakeVisible (charLbl);
    setupLabel (charLbl, "CHARACTER");

    addAndMakeVisible (waveMixKnob);
    setupKnob (waveMixKnob);

    waveMixAtt.reset (
        new juce::AudioProcessorValueTreeState::SliderAttachment (
            p.apvts,
            "waveMix",
            waveMixKnob));

    addAndMakeVisible (waveMixLbl);
    setupLabel (waveMixLbl, "WAVE MIX");

    addAndMakeVisible (waveOnBtn);

    waveOnBtn.setButtonText ("WAVEGUIDE  ON");
    waveOnBtn.setClickingTogglesState (true);

    waveOnBtn.setColour (
        juce::TextButton::buttonColourId,
        gothicLF.graphite);

    waveOnBtn.setColour (
        juce::TextButton::buttonOnColourId,
        gothicLF.neonRed);

    waveOnAtt.reset (
        new juce::AudioProcessorValueTreeState::ButtonAttachment (
            p.apvts,
            "waveguideEnabled",
            waveOnBtn));

    // Main title.
    addAndMakeVisible (titleLbl);

    titleLbl.setText (
        "WAVERING DISTORTION MOD2",
        juce::dontSendNotification);

    titleLbl.setFont (
        juce::Font (
            "Georgia",
            23.0f,
            juce::Font::bold));

    titleLbl.setJustificationType (
        juce::Justification::centred);

    titleLbl.setColour (
        juce::Label::textColourId,
        gothicLF.neonRed);

    titleLbl.setLookAndFeel (&gothicLF);

    setSize (1000, 650);

    startTimerHz (30);
}

//==============================================================================
// Destructor
//==============================================================================

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    driveKnob.setLookAndFeel (nullptr);
    toneKnob.setLookAndFeel (nullptr);
    biasKnob.setLookAndFeel (nullptr);
    bodyKnob.setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr);

    rateKnob.setLookAndFeel (nullptr);
    resKnob.setLookAndFeel (nullptr);
    fbKnob.setLookAndFeel (nullptr);
    dampKnob.setLookAndFeel (nullptr);
    charKnob.setLookAndFeel (nullptr);
    waveMixKnob.setLookAndFeel (nullptr);

    driveLbl.setLookAndFeel (nullptr);
    toneLbl.setLookAndFeel (nullptr);
    biasLbl.setLookAndFeel (nullptr);
    bodyLbl.setLookAndFeel (nullptr);
    mixLbl.setLookAndFeel (nullptr);

    rateLbl.setLookAndFeel (nullptr);
    resLbl.setLookAndFeel (nullptr);
    fbLbl.setLookAndFeel (nullptr);
    dampLbl.setLookAndFeel (nullptr);
    charLbl.setLookAndFeel (nullptr);
    waveMixLbl.setLookAndFeel (nullptr);

    titleLbl.setLookAndFeel (nullptr);

    setLookAndFeel (nullptr);
}

//==============================================================================
// Paint
//==============================================================================

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Main chassis background.
    juce::ColourGradient background (
        juce::Colour (0xff050505),
        0.0f,
        0.0f,
        juce::Colour (0xff202020),
        0.0f,
        static_cast<float> (getHeight()),
        false);

    g.setGradientFill (background);
    g.fillAll();

    // Header screen.
    auto screen = getLocalBounds()
                    .removeFromTop (68)
                    .reduced (18, 8);

    g.setColour (juce::Colour (0xff080000));
    g.fillRoundedRectangle (screen.toFloat(), 7.0f);

    g.setColour (gothicLF.bloodRed);
    g.drawRoundedRectangle (screen.toFloat(), 7.0f, 1.5f);

    auto innerScreen = screen.reduced (4);

    g.setColour (gothicLF.neonRed.withAlpha (0.08f));
    g.fillRoundedRectangle (innerScreen.toFloat(), 5.0f);

    // Screen highlight.
    g.setColour (
        juce::Colour (0xffff334f).withAlpha (0.35f));

    g.drawHorizontalLine (
        screen.getY() + 2,
        static_cast<float> (screen.getX() + 12),
        static_cast<float> (screen.getRight() - 12));

    // Module panels.
    auto drawPanel = [this, &g] (
        juce::Rectangle<int> bounds,
        const juce::String& heading)
    {
        auto plate = bounds.toFloat();

        juce::ColourGradient panelGradient (
            juce::Colour (0xff292929),
            plate.getX(),
            plate.getY(),
            juce::Colour (0xff101010),
            plate.getX(),
            plate.getBottom(),
            false);

        g.setGradientFill (panelGradient);
        g.fillRoundedRectangle (plate, 10.0f);

        // Outer red edge.
        g.setColour (
            gothicLF.bloodRed.withAlpha (0.85f));

        g.drawRoundedRectangle (plate, 10.0f, 1.6f);

        // Inner edge.
        g.setColour (
            juce::Colours::white.withAlpha (0.045f));

        g.drawRoundedRectangle (
            plate.reduced (3.0f),
            8.0f,
            1.0f);

        // Heading.
        auto headingArea = bounds.reduced (12, 0)
                                .removeFromTop (34);

        g.setColour (gothicLF.neonRed);

        g.setFont (
            juce::Font (
                "Georgia",
                16.0f,
                juce::Font::bold));

        g.drawFittedText (
            heading,
            headingArea,
            juce::Justification::centred,
            1);

        // Separator.
        g.setColour (
            gothicLF.bloodRed.withAlpha (0.8f));

        g.drawHorizontalLine (
            headingArea.getBottom() + 2,
            static_cast<float> (bounds.getX() + 14),
            static_cast<float> (bounds.getRight() - 14));
    };

    drawPanel (
        distortionArea,
        "I  /  DISTORTION");

    drawPanel (
        waveguideArea,
        "II  /  WAVEGUIDE");

    // Chassis screws.
    auto drawScrew = [&g] (float x, float y)
    {
        g.setColour (juce::Colours::black);

        g.fillEllipse (
            x - 5.0f,
            y - 5.0f,
            10.0f,
            10.0f);

        g.setColour (juce::Colour (0xff777777));

        g.drawEllipse (
            x - 4.0f,
            y - 4.0f,
            8.0f,
            8.0f,
            1.0f);

        g.setColour (juce::Colour (0xffaaaaaa));

        g.drawLine (
            x - 2.0f,
            y,
            x + 2.0f,
            y,
            1.0f);
    };

    drawScrew (10.0f, 10.0f);

    drawScrew (
        static_cast<float> (getWidth() - 10),
        10.0f);

    drawScrew (
        10.0f,
        static_cast<float> (getHeight() - 10));

    drawScrew (
        static_cast<float> (getWidth() - 10),
        static_cast<float> (getHeight() - 10));
}

//==============================================================================
// Responsive Layout
//==============================================================================

void WaveringAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (76);

    titleLbl.setBounds (
        header.reduced (25, 4));

    bounds.reduce (16, 5);

    const int gap = 14;

    const int panelWidth =
        (bounds.getWidth() - gap) / 2;

    distortionArea = bounds.removeFromLeft (panelWidth);

    bounds.removeFromLeft (gap);

    waveguideArea = bounds;

    auto layoutControls = [] (
        juce::Rectangle<int> panel,
        int controlCount,
        juce::Slider** sliders,
        juce::Label** labels,
        juce::TextButton& button)
    {
        auto content = panel.reduced (10, 43);

        const int buttonHeight = 34;

        auto buttonArea = content.removeFromBottom (
            buttonHeight);

        button.setBounds (
            buttonArea.reduced (8, 2));

        content.removeFromBottom (12);

        const int columnWidth =
            content.getWidth() / controlCount;

        const int labelHeight = 25;

        auto labelRow = content.removeFromTop (
            labelHeight);

        auto knobRow = content;

        for (int i = 0; i < controlCount; ++i)
        {
            const int x = i * columnWidth;

            auto cell = juce::Rectangle<int> (
                content.getX() + x,
                content.getY(),
                columnWidth,
                content.getHeight());

            auto labelCell = juce::Rectangle<int> (
                labelRow.getX() + x,
                labelRow.getY(),
                columnWidth,
                labelRow.getHeight());

            labels[i]->setBounds (
                labelCell.reduced (1, 0));

            const int knobWidth =
                juce::jmin (72, columnWidth - 4);

            const int knobHeight =
                juce::jmin (100, knobRow.getHeight());

            sliders[i]->setBounds (
                cell.getCentreX() - knobWidth / 2,
                knobRow.getY() + 2,
                knobWidth,
                knobHeight);
        }
    };

    juce::Slider* distortionSliders[] =
    {
        &driveKnob,
        &toneKnob,
        &biasKnob,
        &bodyKnob,
        &mixKnob
    };

    juce::Label* distortionLabels[] =
    {
        &driveLbl,
        &toneLbl,
        &biasLbl,
        &bodyLbl,
        &mixLbl
    };

    layoutControls (
        distortionArea,
        5,
        distortionSliders,
        distortionLabels,
        distOnBtn);

    juce::Slider* waveguideSliders[] =
    {
        &rateKnob,
        &resKnob,
        &fbKnob,
        &dampKnob,
        &charKnob,
        &waveMixKnob
    };

    juce::Label* waveguideLabels[] =
    {
        &rateLbl,
        &resLbl,
        &fbLbl,
        &dampLbl,
        &charLbl,
        &waveMixLbl
    };

    layoutControls (
        waveguideArea,
        6,
        waveguideSliders,
        waveguideLabels,
        waveOnBtn);
}

//==============================================================================
// Timer
//==============================================================================

void WaveringAudioProcessorEditor::timerCallback()
{
    repaint();
}