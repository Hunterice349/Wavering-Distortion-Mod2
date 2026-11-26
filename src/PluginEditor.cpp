#include "PluginEditor.h"

MetalKnob::MetalKnob (const juce::String& text) : labelText (text)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRange (0.0, 10.0, 0.01);
    setDoubleClickReturnValue (true, 0.0);
}

void MetalKnob::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float diameter = juce::jmax (12.0f, juce::jmin (bounds.getWidth(), bounds.getHeight() - 30.0f) - 12.0f);
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getY() + diameter * 0.5f + 6.0f;
    const juce::Rectangle<float> face (centreX - diameter * 0.5f, centreY - diameter * 0.5f, diameter, diameter);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (face.translated (2.0f, 4.0f).expanded (2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6edf1), face.getX(), face.getY(),
                                              juce::Colour (0xff30363b), face.getRight(), face.getBottom(), false));
    g.fillEllipse (face.expanded (3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffbbc5ca), centreX, face.getY(),
                                              juce::Colour (0xff59636a), centreX, face.getBottom(), false));
    g.fillEllipse (face);

    for (int tick = 0; tick <= 10; ++tick)
    {
        const float angle = juce::degreesToRadians (135.0f + 270.0f * static_cast<float> (tick) / 10.0f);
        const float outer = diameter * 0.5f + 7.0f;
        const float inner = outer - (tick % 5 == 0 ? 5.0f : 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.drawLine (centreX + inner * std::cos (angle), centreY + inner * std::sin (angle),
                    centreX + outer * std::cos (angle), centreY + outer * std::sin (angle), 1.2f);
    }

    const float angle = juce::degreesToRadians (135.0f + 270.0f * static_cast<float> (juce::jlimit (0.0, 1.0, getValue() / 10.0)));
    juce::Path arc;
    const auto arcBounds = face.expanded (8.0f);
    arc.addArc (arcBounds.getX(), arcBounds.getY(), arcBounds.getWidth(), arcBounds.getHeight(),
                juce::degreesToRadians (135.0f), angle, false);
    g.setColour (juce::Colour (0xffffb95b).withAlpha (0.9f));
    g.strokePath (arc, juce::PathStrokeType (2.0f));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine (centreX, centreY, centreX + diameter * 0.34f * std::cos (angle),
                centreY + diameter * 0.34f * std::sin (angle), 3.0f);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xfff0e5d2));
    g.drawText (labelText, juce::Rectangle<int> (0, getHeight() - 27, getWidth(), 13), juce::Justification::centred);
    g.setFont (juce::Font (11.0f));
    g.setColour (juce::Colour (0xffffc775));
    g.drawText (juce::String (getValue(), 1), juce::Rectangle<int> (0, getHeight() - 14, getWidth(), 13),
                juce::Justification::centred);
}

WaveformKnob::WaveformKnob()
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRange (0.0, 3.0, 1.0);
    setDoubleClickReturnValue (true, 0.0);
}

void WaveformKnob::paint (juce::Graphics& g)
{
    static const char* const names[] { "SINE", "TRIANGLE", "SQUARE", "SAW" };
    const auto bounds = getLocalBounds().toFloat();
    const float diameter = juce::jmax (12.0f, juce::jmin (bounds.getWidth(), bounds.getHeight() - 38.0f) - 12.0f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getY() + diameter * 0.5f + 6.0f;
    const float radius = diameter * 0.5f;
    const auto face = juce::Rectangle<float> (cx - radius, cy - radius, diameter, diameter);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (face.translated (2.0f, 4.0f).expanded (2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6edf1), face.getX(), face.getY(),
                                              juce::Colour (0xff30363b), face.getRight(), face.getBottom(), false));
    g.fillEllipse (face.expanded (3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffbbc5ca), cx, face.getY(),
                                              juce::Colour (0xff59636a), cx, face.getBottom(), false));
    g.fillEllipse (face);

    const int selected = juce::jlimit (0, 3, static_cast<int> (std::round (getValue())));
    for (int i = 0; i < 4; ++i)
    {
        const float a = juce::degreesToRadians (135.0f + 90.0f * static_cast<float> (i));
        const float outer = radius + 8.0f;
        const float inner = outer - (i == selected ? 7.0f : 4.0f);
        g.setColour (i == selected ? juce::Colour (0xffffbd68) : juce::Colours::black.withAlpha (0.75f));
        g.drawLine (cx + inner * std::cos (a), cy + inner * std::sin (a),
                    cx + outer * std::cos (a), cy + outer * std::sin (a), 2.0f);
    }

    const float pointerAngle = juce::degreesToRadians (135.0f + 90.0f * static_cast<float> (selected));
    g.setColour (juce::Colour (0xffffb95b));
    g.drawLine (cx, cy, cx + radius * 0.58f * std::cos (pointerAngle),
                cy + radius * 0.58f * std::sin (pointerAngle), 3.0f);
    g.setColour (juce::Colour (0xfff0e5d2));
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText ("WAVEFORM", juce::Rectangle<int> (0, getHeight() - 32, getWidth(), 13), juce::Justification::centred);
    g.setColour (juce::Colour (0xffffc775));
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText (names[selected], juce::Rectangle<int> (0, getHeight() - 17, getWidth(), 13), juce::Justification::centred);
}

DistortionSwitch::DistortionSwitch() : juce::Button ("Distortion on/off") { setClickingTogglesState (true); }
WaveguideSwitch::WaveguideSwitch() : juce::Button ("Waveguide on/off") { setClickingTogglesState (true); }
RingModSwitch::RingModSwitch() : juce::Button ("Ring Modulator on/off") { setClickingTogglesState (true); }

namespace
{
void paintSwitch (juce::Graphics& g, juce::Component& component, bool enabled,
                  const juce::String& text, bool highlighted, bool down)
{
    const auto bounds = component.getLocalBounds().toFloat();
    const auto track = bounds.withHeight (juce::jmin (25.0f, bounds.getHeight()));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (track.translated (1.0f, 2.0f), 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5d9dc), track.getX(), track.getY(),
                                              juce::Colour (0xff41484d), track.getX(), track.getBottom(), false));
    g.fillRoundedRectangle (track, 7.0f);
    const float x = enabled ? track.getRight() - 35.0f : track.getX() + 5.0f;
    juce::Rectangle<float> toggle (x, track.getY() + 3.0f, 30.0f, track.getHeight() - 6.0f);
    if (down || highlighted)
        toggle = toggle.reduced (0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (toggle.translated (0.0f, 2.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6f7f5), toggle.getX(), toggle.getY(),
                                              juce::Colour (0xff737b80), toggle.getRight(), toggle.getBottom(), false));
    g.fillRoundedRectangle (toggle, 5.0f);
    g.setColour (enabled ? juce::Colour (0xff64ff9c) : juce::Colour (0xff572a26));
    g.fillEllipse (track.getCentreX() - 4.0f, track.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xfff4ead8));
    g.drawText (text, juce::Rectangle<float> (bounds.getX(), track.getBottom() + 2.0f,
                                               bounds.getWidth(), bounds.getHeight() - track.getHeight() - 2.0f),
                juce::Justification::centred);
}
}

void DistortionSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    paintSwitch (g, *this, getToggleState(), getToggleState() ? "DISTORTION  ON" : "DISTORTION  OFF", highlighted, down);
}
void WaveguideSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    paintSwitch (g, *this, getToggleState(), getToggleState() ? "WAVEGUIDE  ON" : "WAVEGUIDE  OFF", highlighted, down);
}
void RingModSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    paintSwitch (g, *this, getToggleState(), getToggleState() ? "RING MOD  ON" : "RING MOD  OFF", highlighted, down);
}

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor (WaveringAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    setResizable (false, false);
    addAndMakeVisible (scaledContent);
    scaledContent.paintContent = [this] (juce::Graphics& g) { paintDesign (g); };

    scaleLabel.setText ("UI SCALE", juce::dontSendNotification);
    scaleLabel.setJustificationType (juce::Justification::centredRight);
    scaleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff0e5d2));
    scaleLabel.setFont (juce::Font (9.0f, juce::Font::bold));
    scaleSelector.addItem ("50%", 1);
    scaleSelector.addItem ("100%", 2);
    scaleSelector.addItem ("200%", 3);
    scaleSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff30363b));
    scaleSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffffc775));
    scaleSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xffa8b0b3));
    scaleSelector.setJustificationType (juce::Justification::centred);
    scaledContent.addAndMakeVisible (scaleLabel);
    scaledContent.addAndMakeVisible (scaleSelector);

    const auto attachSlider = [this] (const juce::String& parameter, juce::Slider& slider)
    {
        sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                                    (audioProcessor.apvts, parameter, slider));
    };
    attachSlider ("drive", driveKnob);
    attachSlider ("tone", toneKnob);
    attachSlider ("bias", biasKnob);
    attachSlider ("body", bodyKnob);
    attachSlider ("mix", mixKnob);
    switchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                       (audioProcessor.apvts, "distortionEnabled", distortionSwitch);

    attachSlider ("waveRate", rateKnob);
    attachSlider ("waveResonance", resonanceKnob);
    attachSlider ("waveFeedback", feedbackKnob);
    attachSlider ("waveDamping", dampingKnob);
    attachSlider ("waveCharacter", characterKnob);
    attachSlider ("waveMix", waveMixKnob);
    waveguideSwitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                                (audioProcessor.apvts, "waveguideEnabled", waveguideSwitch);

    attachSlider ("ringRate", ringRateKnob);
    attachSlider ("ringDepth", ringDepthKnob);
    attachSlider ("ringFine", ringFineKnob);
    attachSlider ("ringMix", ringMixKnob);
    ringWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                             (audioProcessor.apvts, "ringWaveform", ringWaveformSelector);
    ringModSwitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                              (audioProcessor.apvts, "ringEnabled", ringModSwitch);

    for (auto* control : std::array<juce::Component*, 5> { &driveKnob, &toneKnob, &biasKnob, &bodyKnob, &mixKnob })
        scaledContent.addAndMakeVisible (*control);
    scaledContent.addAndMakeVisible (distortionSwitch);
    for (auto* control : std::array<juce::Component*, 6> { &rateKnob, &resonanceKnob, &feedbackKnob, &dampingKnob,
                                                           &characterKnob, &waveMixKnob })
        scaledContent.addAndMakeVisible (*control);
    scaledContent.addAndMakeVisible (waveguideSwitch);
    scaledContent.addAndMakeVisible (ringRateKnob);
    scaledContent.addAndMakeVisible (ringDepthKnob);
    scaledContent.addAndMakeVisible (ringWaveformSelector);
    scaledContent.addAndMakeVisible (ringFineKnob);
    scaledContent.addAndMakeVisible (ringMixKnob);
    scaledContent.addAndMakeVisible (ringModSwitch);

    const auto storedChoice = audioProcessor.apvts.getRawParameterValue ("uiScale")->load();
    const int selectedItemId = juce::jlimit (1, 3, static_cast<int> (std::lround (storedChoice)) + 1);
    scaleSelector.setSelectedId (selectedItemId, juce::dontSendNotification);
    scaleSelector.addListener (this);
    audioProcessor.apvts.addParameterListener ("uiScale", this);
    pendingScaleChoice.store (selectedItemId - 1);

    applyScaleMode (selectedItemId);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener ("uiScale", this);
    scaleSelector.removeListener (this);
    cancelPendingUpdate();
}

void WaveringAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox != &scaleSelector)
        return;

    const int itemId = juce::jlimit (1, 3, scaleSelector.getSelectedId());
    if (auto* parameter = audioProcessor.apvts.getParameter ("uiScale"))
    {
        const float choice = static_cast<float> (itemId - 1);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (choice));
        parameter->endChangeGesture();
    }

    applyScaleMode (itemId);
}

void WaveringAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "uiScale")
    {
        pendingScaleChoice.store (juce::jlimit (0, 2, static_cast<int> (std::lround (newValue))));
        triggerAsyncUpdate();
    }
}

void WaveringAudioProcessorEditor::handleAsyncUpdate()
{
    const int itemId = pendingScaleChoice.load() + 1;
    if (scaleSelector.getSelectedId() != itemId)
        scaleSelector.setSelectedId (itemId, juce::dontSendNotification);
    applyScaleMode (itemId);
}

void WaveringAudioProcessorEditor::applyScaleMode (int selectedItemId)
{
    const int itemId = juce::jlimit (1, 3, selectedItemId);
    const float scale = itemId == 1 ? 0.5f : (itemId == 3 ? 2.0f : 1.0f);

    scaledContent.setTransform (juce::AffineTransform::scale (scale));
    setSize (juce::roundToInt (designWidth * scale), juce::roundToInt (designHeight * scale));
}

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff211b16));
}

void WaveringAudioProcessorEditor::paintDesign (juce::Graphics& g)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (designWidth), static_cast<float> (designHeight));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffaeb5b8), 0.0f, 0.0f,
                                              juce::Colour (0xff4a5053), 0.0f, bounds.getHeight(), false));
    g.fillRoundedRectangle (bounds.reduced (12.0f), 13.0f);

    const auto face = bounds.reduced (20.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (face.translated (0.0f, 3.0f), 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff353b3e), face.getX(), face.getY(),
                                              juce::Colour (0xff171b1d), face.getRight(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 10.0f);
    g.setColour (juce::Colour (0xffd3d9d9).withAlpha (0.25f));
    g.drawRoundedRectangle (face.reduced (1.0f), 10.0f, 1.0f);

    // Reserve the header's right side for the scale selector, so title and
    // subtitle remain legible without overlapping at every selected scale.
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xfff0e7d8));
    g.drawText ("HUNTERICE  /  WAVERING DISTORTION", 35, 25, 510, 30, juce::Justification::centred);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffffbd68));
    g.drawText ("ANALOG-INSPIRED SATURATION  •  MODULAR EFFECTS", 35, 55, 510, 18,
                juce::Justification::centred);

    const juce::Rectangle<float> distortionPanel (38.0f, 95.0f, designWidth - 76.0f, 165.0f);
    const juce::Rectangle<float> waveguidePanel (38.0f, 335.0f, designWidth - 76.0f, 165.0f);
    const juce::Rectangle<float> ringPanel (38.0f, 575.0f, designWidth - 76.0f, 165.0f);
    g.setColour (juce::Colours::black.withAlpha (0.38f));
    g.fillRoundedRectangle (distortionPanel, 8.0f);
    g.fillRoundedRectangle (waveguidePanel, 8.0f);
    g.fillRoundedRectangle (ringPanel, 8.0f);
    g.setColour (juce::Colour (0xffa8b0b3).withAlpha (0.22f));
    g.drawRoundedRectangle (distortionPanel, 8.0f, 1.0f);
    g.drawRoundedRectangle (waveguidePanel, 8.0f, 1.0f);
    g.drawRoundedRectangle (ringPanel, 8.0f, 1.0f);

    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffffbd68));
    g.drawText ("DISTORTION", 45, 80, designWidth - 90, 12, juce::Justification::centred);
    g.drawText ("WAVEGUIDE RESONATOR", 45, 320, designWidth - 90, 12, juce::Justification::centred);
    g.drawText ("RING MODULATOR", 45, 560, designWidth - 90, 12, juce::Justification::centred);
    g.setFont (juce::Font (9.0f));
    g.setColour (juce::Colour (0xffaeb6b7));
    g.drawText ("MODULAR EFFECT CHAIN  •  DISTORTION → WAVEGUIDE → RING MOD", 40, 798,
                designWidth - 80, 14, juce::Justification::centred);
}

void WaveringAudioProcessorEditor::resized()
{
    scaledContent.setBounds (0, 0, designWidth, designHeight);
    constexpr int knobWidth = 100;
    constexpr int knobHeight = 142;
    constexpr int gap = 16;
    constexpr int totalWidth = knobWidth * 6 + gap * 5;
    const int startX = (designWidth - totalWidth) / 2;

    int x = (designWidth - (112 * 5 + 18 * 4)) / 2;
    driveKnob.setBounds (x, 103, 112, 142); x += 130;
    toneKnob.setBounds (x, 103, 112, 142); x += 130;
    biasKnob.setBounds (x, 103, 112, 142); x += 130;
    bodyKnob.setBounds (x, 103, 112, 142); x += 130;
    mixKnob.setBounds (x, 103, 112, 142);
    distortionSwitch.setBounds (designWidth / 2 - 88, 260, 176, 44);

    x = startX;
    rateKnob.setBounds (x, 343, knobWidth, knobHeight); x += knobWidth + gap;
    resonanceKnob.setBounds (x, 343, knobWidth, knobHeight); x += knobWidth + gap;
    feedbackKnob.setBounds (x, 343, knobWidth, knobHeight); x += knobWidth + gap;
    dampingKnob.setBounds (x, 343, knobWidth, knobHeight); x += knobWidth + gap;
    characterKnob.setBounds (x, 343, knobWidth, knobHeight); x += knobWidth + gap;
    waveMixKnob.setBounds (x, 343, knobWidth, knobHeight);
    waveguideSwitch.setBounds (designWidth / 2 - 88, 500, 176, 44);

    constexpr int ringKnobWidth = 112;
    constexpr int ringGap = 18;
    const int ringStart = (designWidth - (ringKnobWidth * 5 + ringGap * 4)) / 2;
    x = ringStart;
    ringRateKnob.setBounds (x, 583, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringDepthKnob.setBounds (x, 583, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringWaveformSelector.setBounds (x, 583, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringFineKnob.setBounds (x, 583, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringMixKnob.setBounds (x, 583, ringKnobWidth, knobHeight);
    ringModSwitch.setBounds (designWidth / 2 - 88, 745, 176, 44);

    // Place the scale controls in the reserved top-right header area.
    scaleLabel.setBounds (560, 20, 72, 22);
    scaleSelector.setBounds (636, 18, 94, 26);
}
