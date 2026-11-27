
#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr auto backgroundColour = juce::Colour::fromRGB(10, 10, 13);
    constexpr auto panelColour      = juce::Colour::fromRGB(20, 20, 25);
    constexpr auto redColour        = juce::Colour::fromRGB(235, 24, 48);
    constexpr auto darkRedColour    = juce::Colour::fromRGB(95, 8, 20);
    constexpr auto silverColour     = juce::Colour::fromRGB(155, 160, 170);
    constexpr auto textColour       = juce::Colour::fromRGB(220, 220, 225);

    constexpr float pi = 3.14159265358979323846f;
}

//==============================================================
// LOOK AND FEEL
//==============================================================

WaveringLookAndFeel::WaveringLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, textColour);
    setColour(juce::Slider::textBoxBackgroundColourId,
              juce::Colour::fromRGB(12, 12, 16));
    setColour(juce::Slider::textBoxOutlineColourId,
              juce::Colour::fromRGB(75, 75, 85));

    setColour(juce::TextButton::buttonColourId,
              juce::Colour::fromRGB(25, 25, 31));
    setColour(juce::TextButton::buttonOnColourId, darkRedColour);
    setColour(juce::TextButton::textColourOffId, silverColour);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void WaveringLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    juce::ignoreUnused(slider);

    const float size = static_cast<float>(
        juce::jmin(width, height) - 12);

    const float centreX = static_cast<float>(x) + width * 0.5f;
    const float centreY = static_cast<float>(y) + height * 0.5f - 5.0f;

    const float radius = size * 0.5f;
    const float angle = rotaryStartAngle
        + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto centre = juce::Point<float>(centreX, centreY);

    // Shadow beneath the knob.
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(centreX - radius + 2.0f,
                  centreY - radius + 7.0f,
                  size, size);

    // Outer metal housing.
    juce::ColourGradient outerGradient(
        juce::Colour::fromRGB(115, 118, 126),
        centreX, centreY - radius,
        juce::Colour::fromRGB(22, 23, 29),
        centreX, centreY + radius,
        false);

    g.setGradientFill(outerGradient);
    g.fillEllipse(centreX - radius,
                  centreY - radius,
                  size, size);

    // Outer bevel.
    g.setColour(juce::Colour::fromRGB(190, 192, 200)
                    .withAlpha(0.65f));
    g.drawEllipse(centreX - radius,
                  centreY - radius,
                  size, size, 1.4f);

    // Inner dark ring.
    const float innerRadius = radius * 0.88f;

    g.setColour(juce::Colour::fromRGB(8, 8, 12));
    g.fillEllipse(centreX - innerRadius,
                  centreY - innerRadius,
                  innerRadius * 2.0f,
                  innerRadius * 2.0f);

    // Red illuminated position arc.
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(
        centreX, centreY,
        radius * 0.96f, radius * 0.96f,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    g.setColour(darkRedColour.withAlpha(0.9f));
    g.strokePath(backgroundArc,
        juce::PathStrokeType(3.0f));

    juce::Path activeArc;
    activeArc.addCentredArc(
        centreX, centreY,
        radius * 0.96f, radius * 0.96f,
        0.0f,
        rotaryStartAngle,
        angle,
        true);

    // Neon glow.
    g.setColour(redColour.withAlpha(0.17f));
    g.strokePath(activeArc,
        juce::PathStrokeType(9.0f));

    g.setColour(redColour.withAlpha(0.95f));
    g.strokePath(activeArc,
        juce::PathStrokeType(2.8f));

    // Metallic knob face.
    const float faceRadius = radius * 0.76f;

    juce::ColourGradient faceGradient(
        juce::Colour::fromRGB(92, 94, 103),
        centreX - faceRadius * 0.4f,
        centreY - faceRadius,
        juce::Colour::fromRGB(24, 25, 31),
        centreX + faceRadius * 0.5f,
        centreY + faceRadius,
        false);

    g.setGradientFill(faceGradient);
    g.fillEllipse(centreX - faceRadius,
                  centreY - faceRadius,
                  faceRadius * 2.0f,
                  faceRadius * 2.0f);

    // Fine metallic rim.
    g.setColour(juce::Colour::fromRGB(145, 148, 157)
                    .withAlpha(0.75f));
    g.drawEllipse(centreX - faceRadius,
                  centreY - faceRadius,
                  faceRadius * 2.0f,
                  faceRadius * 2.0f,
                  1.0f);

    // Radial grip notches.
    const int notchCount = 32;

    for (int i = 0; i < notchCount; ++i)
    {
        const float notchAngle =
            (2.0f * pi * static_cast<float>(i))
            / static_cast<float>(notchCount);

        const float outer = radius * 0.84f;
        const float inner = radius * 0.77f;

        const juce::Point<float> p1(
            centreX + std::sin(notchAngle) * inner,
            centreY - std::cos(notchAngle) * inner);

        const juce::Point<float> p2(
            centreX + std::sin(notchAngle) * outer,
            centreY - std::cos(notchAngle) * outer);

        g.setColour(
            i % 4 == 0
                ? silverColour.withAlpha(0.8f)
                : juce::Colours::black.withAlpha(0.85f));

        g.drawLine(p1.x, p1.y, p2.x, p2.y,
                   i % 4 == 0 ? 1.5f : 1.0f);
    }

    // Central cap.
    const float capRadius = radius * 0.25f;

    juce::ColourGradient capGradient(
        juce::Colour::fromRGB(115, 117, 125),
        centreX, centreY - capRadius,
        juce::Colour::fromRGB(28, 29, 35),
        centreX, centreY + capRadius,
        false);

    g.setGradientFill(capGradient);
    g.fillEllipse(centreX - capRadius,
                  centreY - capRadius,
                  capRadius * 2.0f,
                  capRadius * 2.0f);

    g.setColour(silverColour.withAlpha(0.7f));
    g.drawEllipse(centreX - capRadius,
                  centreY - capRadius,
                  capRadius * 2.0f,
                  capRadius * 2.0f,
                  1.0f);

    // Red position indicator.
    const float indicatorLength = radius * 0.52f;

    const juce::Point<float> indicatorEnd(
        centreX + std::sin(angle) * indicatorLength,
        centreY - std::cos(angle) * indicatorLength);

    g.setColour(redColour.withAlpha(0.25f));
    g.drawLine(centreX, centreY,
               indicatorEnd.x, indicatorEnd.y, 6.0f);

    g.setColour(redColour);
    g.drawLine(centreX, centreY,
               indicatorEnd.x, indicatorEnd.y, 2.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.fillEllipse(indicatorEnd.x - 2.0f,
                  indicatorEnd.y - 2.0f,
                  4.0f, 4.0f);
}

//==============================================================
// EDITOR CONSTRUCTOR
//==============================================================

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor(
    WaveringAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setLookAndFeel(&lookAndFeel);

    setSize(1100, 700);
    setResizable(true, true);
    setResizeLimits(850, 570, 1600, 1000);

    for (int i = 0; i < parameterCount; ++i)
        setupSlider(i);

    distortionButton.setClickingTogglesState(true);
    waveguideButton.setClickingTogglesState(true);

    distortionButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour::fromRGB(35, 18, 24));

    waveguideButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour::fromRGB(35, 18, 24));

    distortionButton.setColour(
        juce::TextButton::buttonOnColourId, darkRedColour);

    waveguideButton.setColour(
        juce::TextButton::buttonOnColourId, darkRedColour);

    distortionButton.setColour(
        juce::TextButton::textColourOffId, textColour);

    waveguideButton.setColour(
        juce::TextButton::textColourOffId, textColour);

    distortionButton.setColour(
        juce::TextButton::textColourOnId, juce::Colours::white);

    waveguideButton.setColour(
        juce::TextButton::textColourOnId, juce::Colours::white);

    addAndMakeVisible(distortionButton);
    addAndMakeVisible(waveguideButton);

    distortionAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.apvts,
                "distortionEnabled",
                distortionButton);

    waveguideAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.apvts,
                "waveguideEnabled",
                waveguideButton);

    setOpaque(true);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================
// SLIDER SETUP
//==============================================================

void WaveringAudioProcessorEditor::setupSlider(int index)
{
    auto& slider = sliders[static_cast<size_t>(index)];
    auto& label = labels[static_cast<size_t>(index)];

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);

    // Editable numerical value below the knob.
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        76,
        23);

    slider.setDoubleClickReturnValue(true, 5.0);

    slider.setColour(
        juce::Slider::textBoxTextColourId, textColour);

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        juce::Colour::fromRGB(12, 12, 16));

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colour::fromRGB(75, 75, 85));

    slider.setColour(
        juce::Slider::textBoxHighlightColourId,
        darkRedColour);

    slider.setTooltip(parameterNames[static_cast<size_t>(index)]);

    addAndMakeVisible(slider);

    label.setText(
        parameterNames[static_cast<size_t>(index)],
        juce::dontSendNotification);

    label.setJustificationType(juce::Justification::centred);

    label.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(205, 205, 212));

    label.setFont(juce::Font(13.0f, juce::Font::bold));

    addAndMakeVisible(label);

    attachments.push_back(
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts,
                parameterIDs[static_cast<size_t>(index)],
                slider));
}

//==============================================================
// PANEL DRAWING
//==============================================================

void WaveringAudioProcessorEditor::drawPanel(
    juce::Graphics& g,
    juce::Rectangle<float> area,
    const juce::String& title)
{
    // Panel shadow.
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.fillRoundedRectangle(
        area.translated(0.0f, 5.0f), 12.0f);

    // Main panel surface.
    juce::ColourGradient panelGradient(
        juce::Colour::fromRGB(29, 29, 36),
        area.getX(), area.getY(),
        panelColour,
        area.getX(), area.getBottom(),
        false);

    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(area, 12.0f);

    // Outer metal edge.
    g.setColour(
        juce::Colour::fromRGB(85, 85, 95).withAlpha(0.55f));

    g.drawRoundedRectangle(area, 12.0f, 1.0f);

    // Inner top reflection.
    g.setColour(
        juce::Colours::white.withAlpha(0.045f));

    g.drawLine(
        area.getX() + 14.0f,
        area.getY() + 2.0f,
        area.getRight() - 14.0f,
        area.getY() + 2.0f,
        1.0f);

    // Panel title.
    auto titleArea = area.removeFromTop(48.0f);

    g.setColour(textColour);
    g.setFont(juce::Font(17.0f, juce::Font::bold));

    g.drawText(
        title,
        titleArea.toNearestInt(),
        juce::Justification::centred,
        false);

    // Red title separator.
    const float lineY = titleArea.getBottom() - 1.0f;

    g.setColour(redColour.withAlpha(0.12f));
    g.drawLine(
        area.getX() + 15.0f,
        lineY,
        area.getRight() - 15.0f,
        lineY,
        5.0f);

    g.setColour(redColour.withAlpha(0.8f));
    g.drawLine(
        area.getX() + 15.0f,
        lineY,
        area.getRight() - 15.0f,
        lineY,
        1.0f);
}

//==============================================================
// MAIN PAINT
//==============================================================

void WaveringAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Main background.
    juce::ColourGradient backgroundGradient(
        juce::Colour::fromRGB(19, 19, 24),
        0.0f, 0.0f,
        backgroundColour,
        0.0f, bounds.getBottom(),
        false);

    g.setGradientFill(backgroundGradient);
    g.fillAll();

    // Subtle background technical lines.
    g.setColour(
        juce::Colours::white.withAlpha(0.018f));

    for (int i = 0; i < getWidth(); i += 32)
    {
        g.drawVerticalLine(
            i, 0.0f, static_cast<float>(getHeight()));
    }

    // Header.
    auto header = bounds.removeFromTop(95.0f);

    g.setColour(juce::Colour::fromRGB(235, 235, 240));
    g.setFont(juce::Font(29.0f, juce::Font::bold));

    g.drawText(
        "W A V E R I N G",
        header.withTrimmedTop(12.0f).toNearestInt(),
        juce::Justification::centred,
        false);

    g.setColour(redColour);
    g.setFont(juce::Font(11.0f, juce::Font::bold));

    g.drawText(
        "DISTORTION MODULATION SYSTEM",
        header.withTrimmedTop(55.0f).toNearestInt(),
        juce::Justification::centred,
        false);

    // Header red light.
    const float lightY = 87.0f;

    g.setColour(redColour.withAlpha(0.18f));
    g.fillEllipse(
        getWidth() * 0.5f - 35.0f,
        lightY - 5.0f,
        70.0f, 10.0f);

    g.setColour(redColour);
    g.drawLine(
        getWidth() * 0.5f - 35.0f,
        lightY,
        getWidth() * 0.5f + 35.0f,
        lightY,
        1.2f);

    // Main panels.
    auto content = getLocalBounds().toFloat();
    content.removeFromTop(105.0f);
    content.removeFromBottom(55.0f);
    content.reduce(18.0f, 0.0f);

    const float gap = 16.0f;
    const float panelWidth = (content.getWidth() - gap) * 0.5f;

    auto distortionArea = juce::Rectangle<float>(
        content.getX(),
        content.getY(),
        panelWidth,
        content.getHeight());

    auto waveguideArea = juce::Rectangle<float>(
        content.getX() + panelWidth + gap,
        content.getY(),
        panelWidth,
        content.getHeight());

    drawPanel(g, distortionArea, "01  /  DISTORTION");
    drawPanel(g, waveguideArea, "02  /  WAVEGUIDE RESONATOR");

    // Footer.
    g.setColour(
        juce::Colour::fromRGB(105, 105, 115));

    g.setFont(juce::Font(10.0f));

    g.drawText(
        "WAVERING AUDIO  |  PRECISION DSP  |  VST3",
        getLocalBounds().removeFromBottom(28),
        juce::Justification::centred,
        false);
}

//==============================================================
// COMPONENT LAYOUT
//==============================================================

void WaveringAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    area.removeFromTop(105);
    area.removeFromBottom(55);
    area.reduce(18, 0);

    constexpr int panelGap = 16;

    const int panelWidth =
        (area.getWidth() - panelGap) / 2;

    auto distortionArea =
        area.removeFromLeft(panelWidth);

    area.removeFromLeft(panelGap);

    auto waveguideArea = area;

    distortionArea.reduce(12, 12);
    waveguideArea.reduce(12, 12);

    // Reserve panel heading space.
    distortionArea.removeFromTop(58);
    waveguideArea.removeFromTop(58);

    // Bottom space for labels and editable values.
    distortionArea.removeFromBottom(12);
    waveguideArea.removeFromBottom(12);

    // Distortion: 5 controls.
    const int distortionCount = 5;
    const int distortionCellWidth =
        distortionArea.getWidth() / distortionCount;

    for (int i = 0; i < distortionCount; ++i)
    {
        auto cell = distortionArea.withX(
            distortionArea.getX() + i * distortionCellWidth)
            .withWidth(distortionCellWidth);

        labels[static_cast<size_t>(i)].setBounds(
            cell.removeFromBottom(25));

        sliders[static_cast<size_t>(i)].setBounds(
            cell.reduced(3, 0));
    }

    // Waveguide: 6 controls.
    const int waveguideCount = 6;
    const int waveguideCellWidth =
        waveguideArea.getWidth() / waveguideCount;

    for (int i = 0; i < waveguideCount; ++i)
    {
        auto cell = waveguideArea.withX(
            waveguideArea.getX() + i * waveguideCellWidth)
            .withWidth(waveguideCellWidth);

        const int sliderIndex = i + distortionCount;

        labels[static_cast<size_t>(sliderIndex)].setBounds(
            cell.removeFromBottom(25));

        sliders[static_cast<size_t>(sliderIndex)].setBounds(
            cell.reduced(3, 0));
    }

    // Module switches.
    distortionButton.setBounds(
        25,
        getHeight() - 47,
        180,
        32);

    waveguideButton.setBounds(
        getWidth() - 205,
        getHeight() - 47,
        180,
        32);
}
