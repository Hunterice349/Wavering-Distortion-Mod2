
#include "PluginEditor.h"

#include <cmath>

//==============================================================================
// COLOUR PALETTE
//==============================================================================

namespace PremiumColours
{
    const juce::Colour black       { 0xff101315 };
    const juce::Colour darkMetal   { 0xff202629 };
    const juce::Colour metalLight  { 0xffcbd0d1 };
    const juce::Colour metalDark   { 0xff41494c };

    const juce::Colour walnutDark { 0xff29180f };
    const juce::Colour walnutMid  { 0xff68432a };
    const juce::Colour walnutLite { 0xff916744 };

    const juce::Colour amber      { 0xffffb84d };
    const juce::Colour amberLight { 0xffffd18a };

    const juce::Colour ivory      { 0xffeee6d8 };
}

//==============================================================================
// METAL KNOB
//==============================================================================

MetalKnob::MetalKnob(const juce::String& text)
    : labelText(text)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);

    setTextBoxStyle(
        juce::Slider::NoTextBox,
        false,
        0,
        0
    );

    setRange(0.0, 10.0, 0.01);

    setDoubleClickReturnValue(true, 0.0);

    setMouseCursor(
        juce::MouseCursor::PointingHandCursor
    );
}

//==============================================================================

void MetalKnob::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    const float diameter =
        juce::jmax(
            20.0f,
            juce::jmin(
                bounds.getWidth() - 12.0f,
                bounds.getHeight() - 42.0f
            )
        );

    const float centreX = bounds.getCentreX();

    const float centreY =
        bounds.getY() + diameter * 0.5f + 12.0f;

    const juce::Rectangle<float> face(
        centreX - diameter * 0.5f,
        centreY - diameter * 0.5f,
        diameter,
        diameter
    );

    const float proportion =
        static_cast<float>(
            juce::jlimit(
                0.0,
                1.0,
                valueToProportionOfLength(getValue())
            )
        );

    // Shadow.

    g.setColour(
        juce::Colours::black.withAlpha(0.75f)
    );

    g.fillEllipse(
        face.translated(2.0f, 6.0f).expanded(5.0f)
    );

    // Outer metal ring.

    g.setGradientFill(
        juce::ColourGradient(
            PremiumColours::metalLight,
            face.getX(),
            face.getY(),

            PremiumColours::metalDark,
            face.getRight(),
            face.getBottom(),
            false
        )
    );

    g.fillEllipse(face.expanded(5.0f));

    // Inner metallic body.

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xfff0f3f4),
            centreX,
            face.getY(),

            juce::Colour(0xff596267),
            centreX,
            face.getBottom(),
            false
        )
    );

    g.fillEllipse(face);

    // Fine circular border.

    g.setColour(
        juce::Colours::white.withAlpha(0.35f)
    );

    g.drawEllipse(
        face.reduced(2.0f),
        1.0f
    );

    // Knob position ticks.

    for (int tick = 0; tick <= 10; ++tick)
    {
        const float angle =
            juce::degreesToRadians(
                135.0f + 270.0f * tick / 10.0f
            );

        const float outer = diameter * 0.5f + 13.0f;

        const float inner =
            outer - (tick % 5 == 0 ? 7.0f : 4.0f);

        const float x1 =
            centreX + std::cos(angle) * inner;

        const float y1 =
            centreY + std::sin(angle) * inner;

        const float x2 =
            centreX + std::cos(angle) * outer;

        const float y2 =
            centreY + std::sin(angle) * outer;

        g.setColour(
            PremiumColours::amber.withAlpha(0.85f)
        );

        g.drawLine(
            x1,
            y1,
            x2,
            y2,
            tick % 5 == 0 ? 2.0f : 1.0f
        );
    }

    // Position indicator.

    const float angle =
        juce::degreesToRadians(
            135.0f + 270.0f * proportion
        );

    const float indicatorRadius =
        diameter * 0.35f;

    g.setColour(
        juce::Colours::black.withAlpha(0.8f)
    );

    g.drawLine(
        centreX,
        centreY,
        centreX + std::cos(angle) * indicatorRadius,
        centreY + std::sin(angle) * indicatorRadius,
        4.0f
    );

    // Centre cap.

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xffe6ebed),
            centreX,
            centreY - 5.0f,

            juce::Colour(0xff555d61),
            centreX,
            centreY + 5.0f,
            false
        )
    );

    g.fillEllipse(
        centreX - 6.0f,
        centreY - 6.0f,
        12.0f,
        12.0f
    );

    // Label.

    g.setFont(
        juce::Font(11.0f, juce::Font::bold)
    );

    g.setColour(PremiumColours::ivory);

    g.drawText(
        labelText,
        0,
        getHeight() - 25,
        getWidth(),
        14,
        juce::Justification::centred
    );

    // Numeric value.

    g.setFont(11.0f);

    g.setColour(PremiumColours::amberLight);

    g.drawText(
        juce::String(getValue(), 2),
        0,
        getHeight() - 12,
        getWidth(),
        12,
        juce::Justification::centred
    );
}

//==============================================================================
// MECHANICAL FOOTSWITCH
//==============================================================================

DistortionFootswitch::DistortionFootswitch()
    : juce::Button("Distortion Footswitch")
{
    setClickingTogglesState(true);

    setMouseCursor(
        juce::MouseCursor::PointingHandCursor
    );
}

//==============================================================================

void DistortionFootswitch::paintButton(
    juce::Graphics& g,
    bool highlighted,
    bool down)
{
    const auto bounds = getLocalBounds().toFloat();

    const float size =
        juce::jmin(
            bounds.getWidth(),
            bounds.getHeight()
        );

    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();

    const float radius = size * 0.34f;

    const bool enabled = getToggleState();

    const float pressOffset =
        down ? 3.0f : 0.0f;

    const float cy =
        centreY + pressOffset;

    // Shadow under the switch.

    g.setColour(
        juce::Colours::black.withAlpha(0.8f)
    );

    g.fillEllipse(
        centreX - radius - 5.0f,
        cy - radius + 7.0f,
        (radius + 5.0f) * 2.0f,
        (radius + 5.0f) * 2.0f
    );

    // Outer chrome ring.

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xfff5f7f8),
            centreX,
            cy - radius,

            juce::Colour(0xff42494d),
            centreX,
            cy + radius,
            false
        )
    );

    g.fillEllipse(
        centreX - radius,
        cy - radius,
        radius * 2.0f,
        radius * 2.0f
    );

    // Inner ring.

    g.setColour(
        juce::Colour(0xff252b2e)
    );

    g.fillEllipse(
        centreX - radius + 7.0f,
        cy - radius + 7.0f,
        radius * 2.0f - 14.0f,
        radius * 2.0f - 14.0f
    );

    // Mechanical metal button.

    const float innerRadius = radius - 13.0f;

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xfff3f5f5),
            centreX - 5.0f,
            cy - innerRadius,

            juce::Colour(0xff747e83),
            centreX + 5.0f,
            cy + innerRadius,
            false
        )
    );

    g.fillEllipse(
        centreX - innerRadius,
        cy - innerRadius,
        innerRadius * 2.0f,
        innerRadius * 2.0f
    );

    // Top reflection.

    g.setColour(
        juce::Colours::white.withAlpha(
            highlighted ? 0.45f : 0.25f
        )
    );

    g.fillEllipse(
        centreX - innerRadius * 0.48f,
        cy - innerRadius * 0.72f,
        innerRadius * 0.85f,
        innerRadius * 0.32f
    );

    // Amber LED.

    const float ledSize = 12.0f;

    const float ledX =
        centreX - ledSize * 0.5f;

    const float ledY =
        bounds.getY() + 2.0f;

    g.setColour(
        juce::Colours::black
    );

    g.fillEllipse(
        ledX - 3.0f,
        ledY - 3.0f,
        ledSize + 6.0f,
        ledSize + 6.0f
    );

    if (enabled)
    {
        g.setColour(
            PremiumColours::amber.withAlpha(0.25f)
        );

        g.fillEllipse(
            ledX - 7.0f,
            ledY - 7.0f,
            ledSize + 14.0f,
            ledSize + 14.0f
        );

        g.setColour(PremiumColours::amberLight);
    }
    else
    {
        g.setColour(
            juce::Colour(0xff422b1c)
        );
    }

    g.fillEllipse(
        ledX,
        ledY,
        ledSize,
        ledSize
    );

    // ON / OFF label.

    g.setFont(
        juce::Font(10.0f, juce::Font::bold)
    );

    g.setColour(
        enabled
            ? PremiumColours::amberLight
            : PremiumColours::metalLight
    );

    // IMPORTANT: bounds is const. Work on a mutable copy.
    auto labelBounds = bounds;

    const auto labelArea =
        labelBounds.removeFromBottom(16.0f);

    g.drawText(
        enabled ? "EFFECT ON" : "EFFECT OFF",
        labelArea,
        juce::Justification::centred
    );
}

//==============================================================================
// VU METER
//==============================================================================

PremiumVUMeter::PremiumVUMeter()
{
}

//==============================================================================

void PremiumVUMeter::setLevels(
    float rms,
    float peak)
{
    currentRms =
        juce::jlimit(0.0f, 8.0f, rms);

    currentPeak =
        juce::jlimit(0.0f, 8.0f, peak);

    // Smooth needle movement.

    const float db =
        juce::Decibels::gainToDecibels(
            currentRms,
            -100.0f
        );

    // Calibration: 0 VU = -18 dBFS.

    const float vuValue =
        juce::jlimit(-20.0f, 3.0f, db + 18.0f);

    const float target =
        (vuValue + 20.0f) / 23.0f;

    smoothedNeedle +=
        (target - smoothedNeedle) * 0.24f;

    const float peakDb =
        juce::Decibels::gainToDecibels(
            currentPeak,
            -100.0f
        );

    const float peakVu =
        juce::jlimit(-20.0f, 3.0f, peakDb + 18.0f);

    peakPosition =
        (peakVu + 20.0f) / 23.0f;
}

//==============================================================================

void PremiumVUMeter::drawScale(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    const float centreX =
        bounds.getCentreX();

    const float centreY =
        bounds.getBottom() * 0.83f;

    const float radius =
        bounds.getWidth() * 0.40f;

    for (int i = 0; i <= 23; ++i)
    {
        const float proportion =
            static_cast<float>(i) / 23.0f;

        const float angle =
            juce::degreesToRadians(
                225.0f + proportion * 90.0f
            );

        const bool major =
            (i % 5 == 0 || i == 23);

        const float outer =
            radius;

        const float inner =
            radius - (major ? 15.0f : 8.0f);

        const float x1 =
            centreX + std::cos(angle) * inner;

        const float y1 =
            centreY + std::sin(angle) * inner;

        const float x2 =
            centreX + std::cos(angle) * outer;

        const float y2 =
            centreY + std::sin(angle) * outer;

        g.setColour(
            i >= 20
                ? juce::Colour(0xffff6b43)
                : PremiumColours::amberLight
        );

        g.drawLine(
            x1,
            y1,
            x2,
            y2,
            major ? 2.0f : 1.0f
        );

        if (major)
        {
            const int vu =
                -20 + i;

            const float labelRadius =
                radius - 30.0f;

            const float tx =
                centreX + std::cos(angle) * labelRadius;

            const float ty =
                centreY + std::sin(angle) * labelRadius;

            g.setFont(
                juce::Font(10.0f, juce::Font::bold)
            );

            g.setColour(
                PremiumColours::ivory
            );

            g.drawText(
                juce::String(vu),
                juce::Rectangle<float>(
                    tx - 14.0f,
                    ty - 8.0f,
                    28.0f,
                    16.0f
                ),
                juce::Justification::centred
            );
        }
    }
}

//==============================================================================

void PremiumVUMeter::paint(juce::Graphics& g)
{
    auto bounds =
        getLocalBounds().toFloat();

    // Outer meter housing.

    g.setColour(
        juce::Colours::black.withAlpha(0.9f)
    );

    g.fillRoundedRectangle(
        bounds.translated(2.0f, 5.0f),
        13.0f
    );

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xffc3c9cb),
            bounds.getX(),
            bounds.getY(),

            juce::Colour(0xff333a3e),
            bounds.getRight(),
            bounds.getBottom(),
            false
        )
    );

    g.fillRoundedRectangle(
        bounds,
        13.0f
    );

    // Inner black frame.

    auto inner =
        bounds.reduced(7.0f);

    g.setColour(
        juce::Colour(0xff090b0c)
    );

    g.fillRoundedRectangle(
        inner,
        9.0f
    );

    // Amber display.

    auto display =
        inner.reduced(5.0f);

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff574027),
            display.getX(),
            display.getY(),

            juce::Colour(0xff18130f),
            display.getX(),
            display.getBottom(),
            false
        )
    );

    g.fillRoundedRectangle(
        display,
        6.0f
    );

    // Scale.

    drawScale(g, display);

    const float centreX =
        display.getCentreX();

    const float centreY =
        display.getBottom() * 0.83f;

    const float radius =
        display.getWidth() * 0.40f - 19.0f;

    // Peak marker.

    const float peakAngle =
        juce::degreesToRadians(
            225.0f + peakPosition * 90.0f
        );

    const float peakX =
        centreX + std::cos(peakAngle) * radius;

    const float peakY =
        centreY + std::sin(peakAngle) * radius;

    g.setColour(
        juce::Colour(0xffff573f)
    );

    g.fillEllipse(
        peakX - 3.0f,
        peakY - 3.0f,
        6.0f,
        6.0f
    );

    // Needle.

    const float needleAngle =
        juce::degreesToRadians(
            225.0f + smoothedNeedle * 90.0f
        );

    const float needleX =
        centreX + std::cos(needleAngle) * radius;

    const float needleY =
        centreY + std::sin(needleAngle) * radius;

    g.setColour(
        juce::Colours::black.withAlpha(0.8f)
    );

    g.drawLine(
        centreX + 1.0f,
        centreY + 2.0f,
        needleX + 1.0f,
        needleY + 2.0f,
        3.5f
    );

    g.setColour(
        PremiumColours::amberLight
    );

    g.drawLine(
        centreX,
        centreY,
        needleX,
        needleY,
        2.0f
    );

    // Needle centre.

    g.setColour(
        juce::Colour(0xffe1e3e3)
    );

    g.fillEllipse(
        centreX - 5.0f,
        centreY - 5.0f,
        10.0f,
        10.0f
    );

    // Glass reflection.

    g.setColour(
        juce::Colours::white.withAlpha(0.055f)
    );

    juce::Path reflection;

    reflection.addTriangle(
        display.getX(),
        display.getY(),
        display.getRight() * 0.7f,
        display.getY(),
        display.getX(),
        display.getBottom() * 0.5f
    );

    g.fillPath(reflection);

    // Meter title.

    g.setFont(
        juce::Font(11.0f, juce::Font::bold)
    );

    g.setColour(
        PremiumColours::amberLight
    );

    g.drawText(
        "OUTPUT LEVEL",
        display.getX(),
        display.getY() + 5.0f,
        display.getWidth(),
        15.0f,
        juce::Justification::centred
    );

    g.setFont(8.0f);

    g.setColour(
        PremiumColours::ivory.withAlpha(0.8f)
    );

    g.drawText(
        "0 VU = -18 dBFS",
        display.getX(),
        display.getBottom() - 17.0f,
        display.getWidth(),
        12.0f,
        juce::Justification::centred
    );
}

//==============================================================================
// MAIN EDITOR
//==============================================================================

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor(
    WaveringAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setResizable(false, false);

    setSize(960, 620);

    // VU.

    addAndMakeVisible(vuMeter);

    // Distortion knobs.

    const auto attachSlider =
        [this](const juce::String& parameter,
               juce::Slider& slider)
        {
            sliderAttachments.push_back(
                std::make_unique<
                    juce::AudioProcessorValueTreeState::SliderAttachment
                >(
                    audioProcessor.apvts,
                    parameter,
                    slider
                )
            );
        };

    attachSlider("drive", driveKnob);
    attachSlider("tone", toneKnob);
    attachSlider("bias", biasKnob);
    attachSlider("body", bodyKnob);
    attachSlider("mix", mixKnob);

    // Footswitch attachment.

    footswitchAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment
        >(
            audioProcessor.apvts,
            "distortionEnabled",
            footswitch
        );

    addAndMakeVisible(driveKnob);
    addAndMakeVisible(toneKnob);
    addAndMakeVisible(biasKnob);
    addAndMakeVisible(bodyKnob);
    addAndMakeVisible(mixKnob);

    addAndMakeVisible(footswitch);

    startTimerHz(50);
}

//==============================================================================

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================

void WaveringAudioProcessorEditor::timerCallback()
{
    const float rms =
        audioProcessor.getVuRms();

    const float peak =
        audioProcessor.getVuPeak();

    vuMeter.setLevels(rms, peak);

    vuMeter.repaint();
}

//==============================================================================
// WOOD PANELS
//==============================================================================

void WaveringAudioProcessorEditor::drawWoodPanels(
    juce::Graphics& g)
{
    const int panelWidth = 66;

    // Left walnut panel.

    juce::ColourGradient leftWood(
        PremiumColours::walnutLite,
        0.0f,
        0.0f,

        PremiumColours::walnutDark,
        static_cast<float>(panelWidth),
        0.0f,
        false
    );

    g.setGradientFill(leftWood);

    g.fillRect(
        0,
        0,
        panelWidth,
        getHeight()
    );

    // Right walnut panel.

    juce::ColourGradient rightWood(
        PremiumColours::walnutDark,
        static_cast<float>(getWidth() - panelWidth),
        0.0f,

        PremiumColours::walnutLite,
        static_cast<float>(getWidth()),
        0.0f,
        false
    );

    g.setGradientFill(rightWood);

    g.fillRect(
        getWidth() - panelWidth,
        0,
        panelWidth,
        getHeight()
    );

    // Wood grain.

    for (int i = 0; i < 30; ++i)
    {
        const float y =
            static_cast<float>(i * 23);

        g.setColour(
            juce::Colours::black.withAlpha(0.09f)
        );

        g.drawLine(
            0.0f,
            y,
            static_cast<float>(panelWidth),
            y + 7.0f,
            1.0f
        );

        g.drawLine(
            static_cast<float>(getWidth() - panelWidth),
            y + 5.0f,
            static_cast<float>(getWidth()),
            y,
            1.0f
        );
    }

    // Side panel highlights.

    g.setColour(
        juce::Colours::white.withAlpha(0.12f)
    );

    g.drawLine(
        static_cast<float>(panelWidth - 2),
        0.0f,
        static_cast<float>(panelWidth - 2),
        static_cast<float>(getHeight()),
        2.0f
    );

    g.drawLine(
        static_cast<float>(getWidth() - panelWidth + 2),
        0.0f,
        static_cast<float>(getWidth() - panelWidth + 2),
        static_cast<float>(getHeight()),
        2.0f
    );
}

//==============================================================================
// METAL PANEL
//==============================================================================

void WaveringAudioProcessorEditor::drawMetalPanel(
    juce::Graphics& g)
{
    auto panel =
        getLocalBounds().toFloat().reduced(66.0f, 14.0f);

    g.setColour(
        juce::Colours::black.withAlpha(0.7f)
    );

    g.fillRoundedRectangle(
        panel.translated(0.0f, 5.0f),
        12.0f
    );

    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff343b3e),
            panel.getX(),
            panel.getY(),

            juce::Colour(0xff111719),
            panel.getRight(),
            panel.getBottom(),
            false
        )
    );

    g.fillRoundedRectangle(
        panel,
        12.0f
    );

    g.setColour(
        PremiumColours::metalLight.withAlpha(0.25f)
    );

    g.drawRoundedRectangle(
        panel.reduced(1.0f),
        12.0f,
        1.0f
    );

    // Brushed metal lines.

    g.setColour(
        juce::Colours::white.withAlpha(0.025f)
    );

    for (int y = 0; y < getHeight(); y += 3)
    {
        g.drawLine(
            panel.getX() + 5.0f,
            static_cast<float>(y),
            panel.getRight() - 5.0f,
            static_cast<float>(y),
            0.5f
        );
    }
}

//==============================================================================
// HEADER
//==============================================================================

void WaveringAudioProcessorEditor::drawHeader(
    juce::Graphics& g)
{
    g.setFont(
        juce::Font(25.0f, juce::Font::bold)
    );

    g.setColour(
        PremiumColours::ivory
    );

    g.drawText(
        "WAVERING DISTORTION MOD",
        100,
        27,
        getWidth() - 200,
        35,
        juce::Justification::centred
    );

    g.setFont(
        juce::Font(10.0f, juce::Font::bold)
    );

    g.setColour(
        PremiumColours::amber
    );

    g.drawText(
        "ANALOG-INSPIRED DISTORTION SYSTEM",
        100,
        63,
        getWidth() - 200,
        18,
        juce::Justification::centred
    );

    g.setColour(
        PremiumColours::metalLight.withAlpha(0.25f)
    );

    g.drawLine(
        105.0f,
        88.0f,
        static_cast<float>(getWidth() - 105),
        88.0f,
        1.0f
    );
}

//==============================================================================
// MAIN PAINT
//==============================================================================

void WaveringAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(PremiumColours::black);

    drawWoodPanels(g);

    drawMetalPanel(g);

    drawHeader(g);

    // VU housing label.

    g.setFont(
        juce::Font(9.0f, juce::Font::bold)
    );

    g.setColour(
        PremiumColours::amber
    );

    g.drawText(
        "ANALOG OUTPUT METER",
        300,
        95,
        360,
        14,
        juce::Justification::centred
    );

    // Control panel label.

    g.setFont(
        juce::Font(10.0f, juce::Font::bold)
    );

    g.setColour(
        PremiumColours::ivory.withAlpha(0.9f)
    );

    g.drawText(
        "DISTORTION CONTROLS",
        100,
        330,
        getWidth() - 200,
        16,
        juce::Justification::centred
    );

    // Footer.

    g.setFont(9.0f);

    g.setColour(
        PremiumColours::metalLight.withAlpha(0.55f)
    );

    g.drawText(
        "HUNTERICE  /  PREMIUM ANALOG SERIES",
        100,
        595,
        getWidth() - 200,
        12,
        juce::Justification::centred
    );
}

//==============================================================================
// RESIZED
//==============================================================================

void WaveringAudioProcessorEditor::resized()
{
    // VU meter.

    vuMeter.setBounds(
        getWidth() / 2 - 180,
        112,
        360,
        190
    );

    // Five distortion controls.

    constexpr int knobWidth = 132;
    constexpr int knobHeight = 155;
    constexpr int gap = 14;

    constexpr int totalWidth =
        knobWidth * 5 + gap * 4;

    const int startX =
        (getWidth() - totalWidth) / 2;

    int x = startX;

    driveKnob.setBounds(
        x, 350, knobWidth, knobHeight
    );

    x += knobWidth + gap;

    toneKnob.setBounds(
        x, 350, knobWidth, knobHeight
    );

    x += knobWidth + gap;

    biasKnob.setBounds(
        x, 350, knobWidth, knobHeight
    );

    x += knobWidth + gap;

    bodyKnob.setBounds(
        x, 350, knobWidth, knobHeight
    );

    x += knobWidth + gap;

    mixKnob.setBounds(
        x, 350, knobWidth, knobHeight
    );

    // Mechanical footswitch.

    footswitch.setBounds(
        getWidth() / 2 - 55,
        510,
        110,
        78
    );
}
