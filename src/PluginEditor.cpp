#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 0xff080808 };
    const juce::Colour panel { 0xff111111 };
    const juce::Colour metal { 0xff343434 };
    const juce::Colour red { 0xffff123c };
    const juce::Colour text { 0xffeeeeee };
}

WaveringLookAndFeel::WaveringLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxBackgroundColourId, 0xff090909);
    setColour(juce::Slider::textBoxOutlineColourId, red.withAlpha(0.65f));
    setColour(juce::Slider::textBoxHighlightColourId, red.withAlpha(0.3f));
}

void WaveringLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider&)
{
    juce::ignoreUnused(rotaryStartAngle, rotaryEndAngle);

    const float size = static_cast<float>(
        juce::jmin(width, height) - 18);

    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float radius = size * 0.5f;

    const auto centre = juce::Point<float>(cx, cy);

    // Deep shadow beneath the metal housing.
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillEllipse(cx - radius + 2.0f,
                  cy - radius + 6.0f,
                  size, size);

    // Outer machined metal ring.
    juce::ColourGradient outer(
        juce::Colour(0xff777777), cx, cy - radius,
        juce::Colour(0xff111111), cx, cy + radius,
        false);

    g.setGradientFill(outer);
    g.fillEllipse(cx - radius, cy - radius, size, size);

    // Recessed inner ring.
    const float innerRadius = radius * 0.88f;

    juce::ColourGradient ring(
        juce::Colour(0xff090909), cx, cy - innerRadius,
        juce::Colour(0xff555555), cx, cy + innerRadius,
        false);

    g.setGradientFill(ring);
    g.fillEllipse(cx - innerRadius, cy - innerRadius,
                  innerRadius * 2.0f, innerRadius * 2.0f);

    // Main knob body: metallic 3D shading.
    const float bodyRadius = radius * 0.77f;

    juce::ColourGradient body(
        juce::Colour(0xff777777), cx - bodyRadius * 0.45f,
        cy - bodyRadius,
        juce::Colour(0xff171717), cx + bodyRadius * 0.5f,
        cy + bodyRadius,
        false);

    g.setGradientFill(body);
    g.fillEllipse(cx - bodyRadius, cy - bodyRadius,
                  bodyRadius * 2.0f, bodyRadius * 2.0f);

    // Fine metallic edge.
    g.setColour(juce::Colour(0xffaaaaaa).withAlpha(0.55f));
    g.drawEllipse(cx - bodyRadius, cy - bodyRadius,
                  bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    // Machined grip marks around the knob.
    for (int i = 0; i < 32; ++i)
    {
        const float angle =
            juce::MathConstants<float>::twoPi *
            static_cast<float>(i) / 32.0f;

        const float r1 = radius * 0.91f;
        const float r2 = radius * 0.98f;

        const juce::Point<float> p1(
            cx + std::cos(angle) * r1,
            cy + std::sin(angle) * r1);

        const juce::Point<float> p2(
            cx + std::cos(angle) * r2,
            cy + std::sin(angle) * r2);

        g.setColour(juce::Colour(0xffc0c0c0).withAlpha(0.45f));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.2f);
    }

    // Red LED position arc.
    const float start = juce::MathConstants<float>::pi * 1.25f;
    const float end = juce::MathConstants<float>::pi * 2.75f;
    const float angle = start + sliderPos * (end - start);

    juce::Path arc;
    arc.addCentredArc(cx, cy, radius * 1.06f, radius * 1.06f,
                      0.0f, start, end, true);

    g.setColour(red.withAlpha(0.18f));
    g.strokePath(arc, juce::PathStrokeType(4.0f));

    juce::Path activeArc;
    activeArc.addCentredArc(cx, cy, radius * 1.06f, radius * 1.06f,
                            0.0f, start, angle, true);

    g.setColour(red.withAlpha(0.25f));
    g.strokePath(activeArc, juce::PathStrokeType(7.0f));

    g.setColour(red);
    g.strokePath(activeArc, juce::PathStrokeType(2.4f));

    // Pointer.
    const float pointerLength = bodyRadius * 0.72f;

    const juce::Point<float> pointerEnd(
        cx + std::cos(angle) * pointerLength,
        cy + std::sin(angle) * pointerLength);

    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawLine(cx + 1.0f, cy + 2.0f,
               pointerEnd.x + 1.0f, pointerEnd.y + 2.0f, 4.0f);

    g.setColour(red);
    g.drawLine(cx, cy, pointerEnd.x, pointerEnd.y, 2.2f);

    // Central polished cap.
    juce::ColourGradient cap(
        juce::Colour(0xffaaaaaa), cx, cy - 5.0f,
        juce::Colour(0xff202020), cx, cy + 7.0f,
        false);

    g.setGradientFill(cap);
    g.fillEllipse(cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);

    g.setColour(red.withAlpha(0.85f));
    g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
}

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor(
    WaveringAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1000, 650);
    setResizable(true, true);
    setResizeLimits(760, 520, 1500, 950);

    for (int i = 0; i < parameterCount; ++i)
        setupSlider(i);

    distortionButton.setClickingTogglesState(true);
    waveguideButton.setClickingTogglesState(true);

    distortionButton.setColour(
        juce::TextButton::buttonColourId, panel);
    waveguideButton.setColour(
        juce::TextButton::buttonColourId, panel);

    distortionButton.setColour(
        juce::TextButton::textColourOffId, text);
    waveguideButton.setColour(
        juce::TextButton::textColourOffId, text);

    distortionButton.setColour(
        juce::TextButton::textColourOnId, red);
    waveguideButton.setColour(
        juce::TextButton::textColourOnId, red);

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

    addAndMakeVisible(distortionButton);
    addAndMakeVisible(waveguideButton);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void WaveringAudioProcessorEditor::setupSlider(int index)
{
    auto& slider = sliders[static_cast<size_t>(index)];
    auto& label = labels[static_cast<size_t>(index)];

    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    // Editable numeric value.
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 76, 22);

    slider.setNumDecimalPlacesToDisplay(2);
    slider.setDoubleClickReturnValue(true, 0.0);

    slider.setColour(
        juce::Slider::textBoxTextColourId, text);

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        juce::Colour(0xff080808));

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        red.withAlpha(0.5f));

    addAndMakeVisible(slider);

    label.setText(parameterNames[static_cast<size_t>(index)],
                  juce::dontSendNotification);

    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font("Georgia", 14.0f,
                             juce::Font::bold));

    label.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(label);

    attachments.push_back(
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts,
                parameterIDs[static_cast<size_t>(index)],
                slider));
}

void WaveringAudioProcessorEditor::drawPanel(
    juce::Graphics& g,
    juce::Rectangle<float> area,
    const juce::String& title)
{
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(area.translated(0.0f, 5.0f), 12.0f);

    juce::ColourGradient gradient(
        juce::Colour(0xff242424),
        area.getX(), area.getY(),
        juce::Colour(0xff0a0a0a),
        area.getX(), area.getBottom(),
        false);

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(area, 12.0f);

    g.setColour(juce::Colour(0xff555555));
    g.drawRoundedRectangle(area, 12.0f, 1.2f);

    g.setColour(red.withAlpha(0.85f));
    g.drawHorizontalLine(
        static_cast<int>(area.getY() + 38.0f),
        area.getX() + 15.0f,
        area.getRight() - 15.0f);

    g.setColour(text);
    g.setFont(juce::Font("Georgia", 19.0f,
                         juce::Font::bold));

    g.drawText(title,
               area.removeFromTop(36.0f),
               juce::Justification::centred);
}

void WaveringAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient base(
        juce::Colour(0xff242424), 0.0f, 0.0f,
        juce::Colour(0xff050505), 0.0f,
        static_cast<float>(getHeight()),
        false);

    g.setGradientFill(base);
    g.fillRect(getLocalBounds());

    // Outer chassis.
    auto chassis = getLocalBounds().toFloat().reduced(9.0f);

    g.setColour(juce::Colour(0xff555555));
    g.drawRoundedRectangle(chassis, 15.0f, 2.0f);

    g.setColour(juce::Colour(0xff000000));
    g.drawRoundedRectangle(chassis.reduced(4.0f), 12.0f, 1.0f);

    // Header.
    auto header = getLocalBounds().removeFromTop(90);

    g.setColour(text);
    g.setFont(juce::Font("Georgia", 31.0f,
                         juce::Font::bold));

    g.drawText("WAVERING",
               header.removeFromTop(43),
               juce::Justification::centred);

    g.setColour(red);
    g.setFont(juce::Font("Georgia", 15.0f,
                         juce::Font::bold));

    g.drawText("DISTORTION MOD  //  ANALOGUE FORCE",
               header,
               juce::Justification::centred);

    // Neon separator.
    g.setColour(red.withAlpha(0.8f));
    g.drawHorizontalLine(91, 28.0f,
                         static_cast<float>(getWidth() - 28));
}

void WaveringAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(105);

    auto moduleArea = area;

    const int gap = 14;
    const int panelWidth = (moduleArea.getWidth() - gap) / 2;

    auto distortionArea =
        moduleArea.removeFromLeft(panelWidth);

    moduleArea.removeFromLeft(gap);

    auto waveguideArea = moduleArea;

    drawPanel; // Intentionally no drawing here; paint handles the background.

    auto layoutModule = [this](juce::Rectangle<int> bounds,
                               int firstIndex,
                               int count)
    {
        const int columns = 3;
        const int rows = (count + columns - 1) / columns;

        auto grid = bounds.reduced(12);
        grid.removeFromTop(45);

        const int cellWidth = grid.getWidth() / columns;
        const int cellHeight = grid.getHeight() / rows;

        for (int i = 0; i < count; ++i)
        {
            const int column = i % columns;
            const int row = i / columns;

            auto cell = juce::Rectangle<int>(
                grid.getX() + column * cellWidth,
                grid.getY() + row * cellHeight,
                cellWidth, cellHeight);

            const int index = firstIndex + i;

            labels[static_cast<size_t>(index)].setBounds(
                cell.removeFromTop(23));

            sliders[static_cast<size_t>(index)].setBounds(
                cell.reduced(5));
        }
    };

    layoutModule(distortionArea, 0, 5);
    layoutModule(waveguideArea, 5, 6);

    const int buttonY = getHeight() - 48;

    distortionButton.setBounds(
        35, buttonY, 170, 30);

    waveguideButton.setBounds(
        getWidth() - 205, buttonY, 170, 30);
}