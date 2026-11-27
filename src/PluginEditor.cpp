#include "PluginEditor.h"

#include <cmath>

//==============================================================================
// GOTHIC RED NEON COLOUR PALETTE
//==============================================================================

namespace PremiumColours
{
    const juce::Colour black { 0xff0a0a0a };
    const juce::Colour darkMetal { 0xff1a1a1a };
    const juce::Colour metalLight { 0xff8a8a8a };
    const juce::Colour metalDark { 0xff2a2a2a };
    const juce::Colour walnutDark { 0xff1a0f0a };
    const juce::Colour walnutMid { 0xff3a2518 };
    const juce::Colour walnutLite { 0xff5a3a28 };
    
    // RED NEON PALETTE
    const juce::Colour neonRed { 0xffff1a1a };
    const juce::Colour crimsonRed { 0xffdc143c };
    const juce::Colour deepRed { 0xff8b0000 };
    const juce::Colour redGlow { 0xffff4444 };
    const juce::Colour coldMetal { 0xff6a6a7a };
    
    const juce::Colour ivory { 0xffe8e8e8 };
}

//==============================================================================
// NUMERIC TEXT BOX FOR DIRECT VALUE INPUT
//==============================================================================

class MetalKnob::NumericTextBox : public juce::TextEditor
{
public:
    NumericTextBox(MetalKnob& ownerKnob) : owner(ownerKnob)
    {
        setMultiLine(false);
        setJustification(juce::Justification::centred);
        setInputRestrictions(10, "-0123456789.");
        
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::TextEditor::textColourId, PremiumColours::neonRed);
        setColour(juce::TextEditor::outlineColourId, PremiumColours::metalDark);
        setColour(juce::TextEditor::focusedOutlineColourId, PremiumColours::neonRed);
        setColour(juce::CaretComponent::caretColourId, PremiumColours::neonRed);
        
        setFont(juce::Font(13.0f, juce::Font::bold));
        
        onReturnKey = [this] { confirmEdit(); };
        onEscapeKey = [this] { cancelEdit(); };
        onFocusLost = [this] { confirmEdit(); };
    }
    
private:
    MetalKnob& owner;
    
    void confirmEdit()
    {
        const auto text = getText();
        const double value = text.getDoubleValue();
        
        if (value >= owner.minValue && value <= owner.maxValue)
        {
            owner.setValue(static_cast<float>(value));
            if (owner.onValueChange)
                owner.onValueChange(static_cast<float>(value));
        }
        
        updateDisplay();
        giveAwayKeyboardFocus();
    }
    
    void cancelEdit()
    {
        updateDisplay();
        giveAwayKeyboardFocus();
    }
    
    void updateDisplay()
    {
        setText(juce::String(owner.getValue(), 2), false);
    }
};

//==============================================================================
// METAL KNOB IMPLEMENTATION
//==============================================================================

MetalKnob::MetalKnob(const juce::String& text)
    : labelText(text)
{
    textBox = std::make_unique<NumericTextBox>(*this);
    addAndMakeVisible(textBox.get());
    
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MetalKnob::resized()
{
    auto bounds = getLocalBounds();
    textBox->setBounds(bounds.removeFromBottom(24).reduced(15, 0));
}

void MetalKnob::setValue(float newValue)
{
    currentValue = jlimit(minValue, maxValue, static_cast<double>(newValue));
    updateTextBox();
    repaint();
}

float MetalKnob::getValue() const
{
    return static_cast<float>(currentValue);
}

void MetalKnob::setRange(double min, double max, double intv)
{
    minValue = min;
    maxValue = max;
    interval = intv;
}

void MetalKnob::updateTextBox()
{
    if (textBox)
        textBox->setText(juce::String(currentValue, 2), false);
}

void MetalKnob::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && !textBox->getBounds().contains(e.getPosition()))
    {
        isDragging = true;
        lastY = static_cast<float>(e.y);
    }
}

void MetalKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging) return;
    
    const float deltaY = lastY - static_cast<float>(e.y);
    lastY = static_cast<float>(e.y);
    
    const double sensitivity = 0.05 * (maxValue - minValue);
    const double newValue = currentValue + (deltaY * sensitivity / 100.0);
    
    setValue(static_cast<float>(newValue));
    
    if (onValueChange)
        onValueChange(getValue());
}

void MetalKnob::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
}

void MetalKnob::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!textBox->getBounds().contains(e.getPosition()))
    {
        setValue(static_cast<float>(minValue));
        if (onValueChange)
            onValueChange(getValue());
    }
}

void MetalKnob::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float diameter = juce::jmax(
        20.0f,
        juce::jmin(
            bounds.getWidth() - 12.0f,
            bounds.getHeight() - 42.0f - 28.0f // Space for text box
        )
    );
    
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getY() + diameter * 0.5f + 12.0f;
    
    const juce::Rectangle<float> face(
        centreX - diameter * 0.5f,
        centreY - diameter * 0.5f,
        diameter,
        diameter
    );
    
    const float proportion = static_cast<float>(
        juce::jlimit(
            0.0,
            1.0,
            (currentValue - minValue) / (maxValue - minValue)
        )
    );
    
    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(face.translated(2.0f, 6.0f).expanded(5.0f));
    
    // Outer metal ring
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
    
    // Inner metallic body
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff3a3a3a),
            centreX,
            face.getY(),
            juce::Colour(0xff1a1a1a),
            centreX,
            face.getBottom(),
            false
        )
    );
    g.fillEllipse(face);
    
    // Fine circular border
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawEllipse(face.reduced(2.0f), 1.0f);
    
    // Knob position ticks - RED NEON
    for (int tick = 0; tick <= 10; ++tick)
    {
        const float angle = juce::degreesToRadians(135.0f + 270.0f * tick / 10.0f);
        const float outer = diameter * 0.5f + 13.0f;
        const float inner = outer - (tick % 5 == 0 ? 7.0f : 4.0f);
        
        const float x1 = centreX + std::cos(angle) * inner;
        const float y1 = centreY + std::sin(angle) * inner;
        const float x2 = centreX + std::cos(angle) * outer;
        const float y2 = centreY + std::sin(angle) * outer;
        
        g.setColour(PremiumColours::neonRed.withAlpha(0.95f));
        g.drawLine(x1, y1, x2, y2, tick % 5 == 0 ? 2.5f : 1.5f);
    }
    
    // Position indicator - RED NEON
    const float angle = juce::degreesToRadians(135.0f + 270.0f * proportion);
    const float indicatorRadius = diameter * 0.35f;
    
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.drawLine(
        centreX,
        centreY,
        centreX + std::cos(angle) * indicatorRadius,
        centreY + std::sin(angle) * indicatorRadius,
        4.5f
    );
    
    g.setColour(PremiumColours::neonRed);
    g.drawLine(
        centreX,
        centreY,
        centreX + std::cos(angle) * (indicatorRadius - 2.0f),
        centreY + std::sin(angle) * (indicatorRadius - 2.0f),
        3.0f
    );
    
    // Centre cap
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff2a2a2a),
            centreX,
            centreY - 5.0f,
            juce::Colour(0xff0a0a0a),
            centreX,
            centreY + 5.0f,
            false
        )
    );
    g.fillEllipse(centreX - 8.0f, centreY - 8.0f, 16.0f, 16.0f);
    
    // Label - GOTHIC STYLE
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.setColour(PremiumColours::ivory);
    
    auto labelBounds = bounds;
    const auto labelArea = labelBounds.removeFromBottom(16.0f).translated(0, -26.0f);
    g.drawText(labelText, labelArea, juce::Justification::centred);
}

//==============================================================================
// FOOTSWITCH IMPLEMENTATION
//==============================================================================

DistortionFootswitch::DistortionFootswitch()
    : Button("Distortion Footswitch")
{
    setClickingTogglesState(true);
}

void DistortionFootswitch::paintButton(
    juce::Graphics& g,
    bool highlighted,
    bool down)
{
    const auto bounds = getLocalBounds().toFloat();
    
    // Housing shadow
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillRoundedRectangle(bounds.translated(2.0f, 5.0f), 13.0f);
    
    // Housing chrome
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff4a4a4a),
            bounds.getX(),
            bounds.getY(),
            juce::Colour(0xff1a1a1a),
            bounds.getRight(),
            bounds.getBottom(),
            false
        )
    );
    g.fillRoundedRectangle(bounds, 13.0f);
    
    // Inner black frame
    auto inner = bounds.reduced(7.0f);
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(inner, 9.0f);
    
    // Button surface
    auto button = inner.reduced(8.0f);
    const float shrink = down ? 4.0f : 0.0f;
    button = button.reduced(shrink);
    
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff3a3a3a),
            button.getX(),
            button.getY(),
            juce::Colour(0xff1a1a1a),
            button.getRight(),
            button.getBottom(),
            false
        )
    );
    g.fillRoundedRectangle(button, 8.0f);
    
    // Button highlight
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.fillRoundedRectangle(
        button.reduced(3.0f).withHeight(button.getHeight() * 0.3f),
        6.0f
    );
    
    // LED indicator - RED NEON
    const float ledSize = 14.0f;
    const float ledX = bounds.getCentreX() - ledSize * 0.5f;
    const float ledY = bounds.getBottom() - 28.0f;
    
    const bool isOn = getToggleState();
    
    if (isOn)
    {
        // Glow effect
        g.setColour(PremiumColours::neonRed.withAlpha(0.3f));
        g.fillEllipse(ledX - 8.0f, ledY - 8.0f, ledSize + 16.0f, ledSize + 16.0f);
        
        // LED body
        g.setColour(PremiumColours::neonRed);
        g.fillEllipse(ledX, ledY, ledSize, ledSize);
        
        // LED highlight
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillEllipse(ledX + 3.0f, ledY + 2.0f, 4.0f, 4.0f);
    }
    else
    {
        // LED off
        g.setColour(juce::Colour(0xff3a0a0a));
        g.fillEllipse(ledX, ledY, ledSize, ledSize);
    }
    
    // Label
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(PremiumColours::ivory);
    g.drawText(
        isOn ? "ON" : "OFF",
        bounds.removeFromBottom(18.0f),
        juce::Justification::centred
    );
}

//==============================================================================
// VU METER IMPLEMENTATION
//==============================================================================

PremiumVUMeter::PremiumVUMeter()
{
    setOpaque(true);
}

void PremiumVUMeter::setLevels(float rms, float peak)
{
    currentRms = rms;
    currentPeak = peak;
    
    const float targetNeedle = juce::jlimit(0.0f, 1.0f, rms);
    smoothedNeedle += (targetNeedle - smoothedNeedle) * 0.15f;
    
    if (peak > peakPosition)
        peakPosition = peak;
    else
        peakPosition *= 0.995f;
}

void PremiumVUMeter::drawScale(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getBottom() * 0.83f;
    const float radius = bounds.getWidth() * 0.40f - 19.0f;
    
    // Scale markings - RED NEON
    for (int i = 0; i <= 20; ++i)
    {
        const float angle = juce::degreesToRadians(225.0f + i * 4.5f);
        const float outer = radius + 2.0f;
        const float inner = outer - (i % 5 == 0 ? 8.0f : 4.0f);
        
        const float x1 = centreX + std::cos(angle) * inner;
        const float y1 = centreY + std::sin(angle) * inner;
        const float x2 = centreX + std::cos(angle) * outer;
        const float y2 = centreY + std::sin(angle) * outer;
        
        g.setColour(PremiumColours::neonRed.withAlpha(i % 5 == 0 ? 1.0f : 0.7f));
        g.drawLine(x1, y1, x2, y2, i % 5 == 0 ? 2.0f : 1.0f);
        
        if (i % 5 == 0)
        {
            const float labelRadius = radius - 15.0f;
            const float lx = centreX + std::cos(angle) * labelRadius;
            const float ly = centreY + std::sin(angle) * labelRadius;
            
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.setColour(PremiumColours::ivory);
            
            const int value = (i <= 10) ? (i - 10) : (i - 10);
            g.drawText(
                juce::String(value),
                juce::Rectangle<float>(lx - 10.0f, ly - 8.0f, 20.0f, 16.0f),
                juce::Justification::centred
            );
        }
    }
}

void PremiumVUMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    
    // Housing shadow
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillRoundedRectangle(bounds.translated(2.0f, 5.0f), 13.0f);
    
    // Housing chrome
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff4a4a4a),
            bounds.getX(),
            bounds.getY(),
            juce::Colour(0xff1a1a1a),
            bounds.getRight(),
            bounds.getBottom(),
            false
        )
    );
    g.fillRoundedRectangle(bounds, 13.0f);
    
    // Inner black frame
    auto inner = bounds.reduced(7.0f);
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(inner, 9.0f);
    
    // Red neon display background
    auto display = inner.reduced(5.0f);
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff2a0a0a),
            display.getX(),
            display.getY(),
            juce::Colour(0xff0a0505),
            display.getX(),
            display.getBottom(),
            false
        )
    );
    g.fillRoundedRectangle(display, 6.0f);
    
    // Scale
    drawScale(g, display);
    
    const float centreX = display.getCentreX();
    const float centreY = display.getBottom() * 0.83f;
    const float radius = display.getWidth() * 0.40f - 19.0f;
    
    // Peak marker - RED
    const float peakAngle = juce::degreesToRadians(225.0f + peakPosition * 90.0f);
    const float peakX = centreX + std::cos(peakAngle) * radius;
    const float peakY = centreY + std::sin(peakAngle) * radius;
    
    g.setColour(PremiumColours::neonRed);
    g.fillEllipse(peakX - 3.0f, peakY - 3.0f, 6.0f, 6.0f);
    
    // Needle - RED NEON
    const float needleAngle = juce::degreesToRadians(225.0f + smoothedNeedle * 90.0f);
    const float needleX = centreX + std::cos(needleAngle) * radius;
    const float needleY = centreY + std::sin(needleAngle) * radius;
    
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.drawLine(centreX + 1.0f, centreY + 2.0f, needleX + 1.0f, needleY + 2.0f, 3.5f);
    
    g.setColour(PremiumColours::neonRed);
    g.drawLine(centreX, centreY, needleX, needleY, 2.5f);
    
    // Needle centre
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(centreX - 5.0f, centreY - 5.0f, 10.0f, 10.0f);
    
    // Glass reflection
    g.setColour(juce::Colours::white.withAlpha(0.055f));
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
    
    // Meter title - GOTHIC STYLE
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(PremiumColours::neonRed);
    g.drawText(
        "OUTPUT LEVEL",
        display.getX(),
        display.getY() + 5.0f,
        display.getWidth(),
        15.0f,
        juce::Justification::centred
    );
    
    g.setFont(8.0f);
    g.setColour(PremiumColours::ivory.withAlpha(0.8f));
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
// MAIN EDITOR IMPLEMENTATION
//==============================================================================

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor(
    WaveringAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setResizable(false, false);
    setSize(960, 620);
    
    // VU
    addAndMakeVisible(vuMeter);
    
    // Setup knobs with APVTS attachments
    const auto attachSlider = [this](const juce::String& parameter, MetalKnob& knob)
    {
        knob.setRange(0.0, 10.0, 0.01);
        
        knob.onValueChange = [this, parameter](float value)
        {
            if (auto* param = audioProcessor.apvts.getParameter(parameter))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };
        
        // Create slider attachment
        auto slider = std::make_unique<juce::Slider>();
        slider->setRange(0.0, 10.0, 0.01);
        slider->setValue(
            audioProcessor.apvts.getRawParameterValue(parameter)->load(),
            juce::dontSendNotification
        );
        
        knob.setValue(static_cast<float>(slider->getValue()));
        
        sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts,
                parameter,
                *slider
            )
        );
        
        // Sync knob with slider
        slider->onValueChange = [&knob]()
        {
            knob.setValue(static_cast<float>(knob.getValue()));
        };
    };
    
    attachSlider("drive", driveKnob);
    attachSlider("tone", toneKnob);
    attachSlider("bias", biasKnob);
    attachSlider("body", bodyKnob);
    attachSlider("mix", mixKnob);
    
    // Footswitch attachment
    footswitchAttachment = std::make_unique<
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

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    stopTimer();
}

void WaveringAudioProcessorEditor::timerCallback()
{
    const float rms = audioProcessor.getVuRms();
    const float peak = audioProcessor.getVuPeak();
    
    vuMeter.setLevels(rms, peak);
    vuMeter.repaint();
}

//==============================================================================
// WOOD PANELS
//==============================================================================

void WaveringAudioProcessorEditor::drawWoodPanels(juce::Graphics& g)
{
    const int panelWidth = 66;
    
    // Left walnut panel
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
    g.fillRect(0, 0, panelWidth, getHeight());
    
    // Right walnut panel
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
    g.fillRect(getWidth() - panelWidth, 0, panelWidth, getHeight());
    
    // Wood grain
    for (int i = 0; i < 30; ++i)
    {
        const float y = static_cast<float>(i * 23);
        
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.drawLine(0.0f, y, static_cast<float>(panelWidth), y + 7.0f, 1.0f);
        g.drawLine(
            static_cast<float>(getWidth() - panelWidth),
            y + 5.0f,
            static_cast<float>(getWidth()),
            y,
            1.0f
        );
    }
    
    // Side panel highlights
    g.setColour(juce::Colours::white.withAlpha(0.12f));
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

void WaveringAudioProcessorEditor::drawMetalPanel(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat().reduced(66.0f, 14.0f);
    
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(panel.translated(0.0f, 5.0f), 12.0f);
    
    g.setGradientFill(
        juce::ColourGradient(
            juce::Colour(0xff2a2a2a),
            panel.getX(),
            panel.getY(),
            juce::Colour(0xff0a0a0a),
            panel.getRight(),
            panel.getBottom(),
            false
        )
    );
    g.fillRoundedRectangle(panel, 12.0f);
    
    g.setColour(PremiumColours::metalLight.withAlpha(0.25f));
    g.drawRoundedRectangle(panel.reduced(1.0f), 12.0f, 1.0f);
    
    // Brushed metal lines
    g.setColour(juce::Colours::white.withAlpha(0.025f));
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
// GOTHIC ORNAMENTS
//==============================================================================

void WaveringAudioProcessorEditor::drawGothicOrnaments(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(66.0f, 14.0f);
    
    // Corner ornaments - RED NEON
    g.setColour(PremiumColours::neonRed.withAlpha(0.6f));
    
    // Top left
    g.drawLine(bounds.getX() + 10.0f, bounds.getY() + 10.0f, 
               bounds.getX() + 40.0f, bounds.getY() + 10.0f, 2.0f);
    g.drawLine(bounds.getX() + 10.0f, bounds.getY() + 10.0f, 
               bounds.getX() + 10.0f, bounds.getY() + 40.0f, 2.0f);
    
    // Top right
    g.drawLine(bounds.getRight() - 10.0f, bounds.getY() + 10.0f, 
               bounds.getRight() - 40.0f, bounds.getY() + 10.0f, 2.0f);
    g.drawLine(bounds.getRight() - 10.0f, bounds.getY() + 10.0f, 
               bounds.getRight() - 10.0f, bounds.getY() + 40.0f, 2.0f);
    
    // Bottom left
    g.drawLine(bounds.getX() + 10.0f, bounds.getBottom() - 10.0f, 
               bounds.getX() + 40.0f, bounds.getBottom() - 10.0f, 2.0f);
    g.drawLine(bounds.getX() + 10.0f, bounds.getBottom() - 10.0f, 
               bounds.getX() + 10.0f, bounds.getBottom() - 40.0f, 2.0f);
    
    // Bottom right
    g.drawLine(bounds.getRight() - 10.0f, bounds.getBottom() - 10.0f, 
               bounds.getRight() - 40.0f, bounds.getBottom() - 10.0f, 2.0f);
    g.drawLine(bounds.getRight() - 10.0f, bounds.getBottom() - 10.0f, 
               bounds.getRight() - 10.0f, bounds.getBottom() - 40.0f, 2.0f);
    
    // Decorative screws - RED NEON accent
    const float screwSize = 8.0f;
    const auto drawScrew = [&](float x, float y)
    {
        g.setColour(PremiumColours::metalDark);
        g.fillEllipse(x - screwSize/2, y - screwSize/2, screwSize, screwSize);
        g.setColour(PremiumColours::neonRed.withAlpha(0.8f));
        g.drawLine(x - 3.0f, y, x + 3.0f, y, 1.5f);
        g.drawLine(x, y - 3.0f, x, y + 3.0f, 1.5f);
    };
    
    drawScrew(bounds.getX() + 25.0f, bounds.getY() + 25.0f);
    drawScrew(bounds.getRight() - 25.0f, bounds.getY() + 25.0f);
    drawScrew(bounds.getX() + 25.0f, bounds.getBottom() - 25.0f);
    drawScrew(bounds.getRight() - 25.0f, bounds.getBottom() - 25.0f);
}

//==============================================================================
// HEADER
//==============================================================================

void WaveringAudioProcessorEditor::drawHeader(juce::Graphics& g)
{
    // Title - GOTHIC STYLE
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.setColour(PremiumColours::ivory);
    g.drawText(
        "WAVERING DISTORTION MOD",
        100,
        27,
        getWidth() - 200,
        35,
        juce::Justification::centred
    );
    
    // Subtitle - RED NEON
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(PremiumColours::neonRed);
    g.drawText(
        "GOTHIC INDUSTRIAL DISTORTION SYSTEM",
        100,
        63,
        getWidth() - 200,
        18,
        juce::Justification::centred
    );
    
    g.setColour(PremiumColours::metalLight.withAlpha(0.25f));
    g.drawLine(105.0f, 88.0f, static_cast<float>(getWidth() - 105), 88.0f, 1.0f);
}

//==============================================================================
// MAIN PAINT
//==============================================================================

void WaveringAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(PremiumColours::black);
    
    drawWoodPanels(g);
    drawMetalPanel(g);
    drawGothicOrnaments(g);
    drawHeader(g);
    
    // VU housing label - RED NEON
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(PremiumColours::neonRed);
    g.drawText(
        "ANALOG OUTPUT METER",
        300,
        95,
        360,
        14,
        juce::Justification::centred
    );
    
    // Control panel label - GOTHIC STYLE
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(PremiumColours::ivory.withAlpha(0.9f));
    g.drawText(
        "DISTORTION CONTROLS",
        100,
        330,
        getWidth() - 200,
        16,
        juce::Justification::centred
    );
    
    // Footer - RED NEON
    g.setFont(9.0f);
    g.setColour(PremiumColours::neonRed.withAlpha(0.7f));
    g.drawText(
        "HUNTERICE / GOTHIC PREMIUM SERIES",
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
    // VU meter
    vuMeter.setBounds(getWidth() / 2 - 180, 112, 360, 190);
    
    // Five distortion controls with numeric input
    constexpr int knobWidth = 132;
    constexpr int knobHeight = 180; // Increased height for text box
    constexpr int gap = 14;
    constexpr int totalWidth = knobWidth * 5 + gap * 4;
    const int startX = (getWidth() - totalWidth) / 2;
    
    int x = startX;
    
    driveKnob.setBounds(x, 350, knobWidth, knobHeight);
    x += knobWidth + gap;
    
    toneKnob.setBounds(x, 350, knobWidth, knobHeight);
    x += knobWidth + gap;
    
    biasKnob.setBounds(x, 350, knobWidth, knobHeight);
    x += knobWidth + gap;
    
    bodyKnob.setBounds(x, 350, knobWidth, knobHeight);
    x += knobWidth + gap;
    
    mixKnob.setBounds(x, 350, knobWidth, knobHeight);
    
    // Mechanical footswitch
    footswitch.setBounds(getWidth() / 2 - 55, 540, 110, 78);
}