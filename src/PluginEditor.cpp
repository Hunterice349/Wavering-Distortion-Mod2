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
    const float proportion = static_cast<float> (juce::jlimit (0.0, 1.0, getValue() / 10.0));

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

    const float angle = juce::degreesToRadians (135.0f + 270.0f * proportion);
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

DistortionSwitch::DistortionSwitch() : juce::Button ("Distortion on/off")
{
    setClickingTogglesState (true);
}

void DistortionSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();
    const bool enabled = getToggleState();
    const auto track = bounds.withHeight (juce::jmin (25.0f, bounds.getHeight()));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (track.translated (1.0f, 2.0f), 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5d9dc), track.getX(), track.getY(),
                                              juce::Colour (0xff41484d), track.getX(), track.getBottom(), false));
    g.fillRoundedRectangle (track, 7.0f);

    const float travel = track.getWidth() - 35.0f;
    const float x = enabled ? track.getX() + travel : track.getX() + 5.0f;
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
    g.drawText (enabled ? "DISTORTION  ON" : "DISTORTION  OFF",
                juce::Rectangle<float> (bounds.getX(), track.getBottom() + 2.0f,
                                        bounds.getWidth(), bounds.getHeight() - track.getHeight() - 2.0f),
                juce::Justification::centred);
}

WaveguideSwitch::WaveguideSwitch() : juce::Button ("Waveguide on/off")
{
    setClickingTogglesState (true);
}

void WaveguideSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto track = bounds.withHeight (juce::jmin (25.0f, bounds.getHeight()));
    const bool enabled = getToggleState();
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
    g.drawText (enabled ? "WAVEGUIDE  ON" : "WAVEGUIDE  OFF",
                juce::Rectangle<float> (bounds.getX(), track.getBottom() + 2.0f,
                                        bounds.getWidth(), bounds.getHeight() - track.getHeight() - 2.0f),
                juce::Justification::centred);
}

RingModSwitch::RingModSwitch() : juce::Button ("Ring Modulator on/off")
{
    setClickingTogglesState (true);
}

void RingModSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto track = bounds.withHeight (juce::jmin (25.0f, bounds.getHeight()));
    const bool enabled = getToggleState();
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
    g.drawText (enabled ? "RING MOD  ON" : "RING MOD  OFF",
                juce::Rectangle<float> (bounds.getX(), track.getBottom() + 2.0f,
                                        bounds.getWidth(), bounds.getHeight() - track.getHeight() - 2.0f),
                juce::Justification::centred);
}

WaveringAudioProcessorEditor::WaveringAudioProcessorEditor (WaveringAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    setResizable (false, false);
    setSize (760, 920);

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
    ringWaveformSelector.addItemList ({ "Sine", "Triangle", "Square", "Saw" }, 1);
    ringWaveformSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff30363b));
    ringWaveformSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffffc775));
    ringWaveformSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xffa8b0b3));
    ringWaveformSelector.setJustificationType (juce::Justification::centred);
    ringWaveformLabel.setText ("WAVEFORM", juce::dontSendNotification);
    ringWaveformLabel.setJustificationType (juce::Justification::centred);
    ringWaveformLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff0e5d2));
    ringWaveformLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    ringWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
                             (audioProcessor.apvts, "ringWaveform", ringWaveformSelector);
    ringModSwitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                              (audioProcessor.apvts, "ringEnabled", ringModSwitch);

    addAndMakeVisible (driveKnob);
    addAndMakeVisible (toneKnob);
    addAndMakeVisible (biasKnob);
    addAndMakeVisible (bodyKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (distortionSwitch);
    addAndMakeVisible (rateKnob);
    addAndMakeVisible (resonanceKnob);
    addAndMakeVisible (feedbackKnob);
    addAndMakeVisible (dampingKnob);
    addAndMakeVisible (characterKnob);
    addAndMakeVisible (waveMixKnob);
    addAndMakeVisible (waveguideSwitch);
    addAndMakeVisible (ringRateKnob);
    addAndMakeVisible (ringDepthKnob);
    addAndMakeVisible (ringFineKnob);
    addAndMakeVisible (ringMixKnob);
    addAndMakeVisible (ringWaveformLabel);
    addAndMakeVisible (ringWaveformSelector);
    addAndMakeVisible (ringModSwitch);
}

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (juce::Colour (0xff211b16));
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

    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xfff0e7d8));
    g.drawText ("HUNTERICE  /  WAVERING DISTORTION", 35, 38, getWidth() - 70, 30,
                juce::Justification::centred);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffffbd68));
    g.drawText ("ANALOG-INSPIRED SATURATION  •  MODULAR EFFECTS", 35, 70, getWidth() - 70, 18,
                juce::Justification::centred);

    const juce::Rectangle<float> distortionPanel (38.0f, 112.0f, static_cast<float> (getWidth() - 76), 185.0f);
    const juce::Rectangle<float> waveguidePanel (38.0f, 370.0f, static_cast<float> (getWidth() - 76), 185.0f);
    const juce::Rectangle<float> ringPanel (38.0f, 628.0f, static_cast<float> (getWidth() - 76), 185.0f);
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
    g.drawText ("DISTORTION", 45, 98, getWidth() - 90, 12, juce::Justification::centred);
    g.drawText ("WAVEGUIDE RESONATOR", 45, 356, getWidth() - 90, 12, juce::Justification::centred);
    g.drawText ("RING MODULATOR", 45, 614, getWidth() - 90, 12, juce::Justification::centred);
    g.setFont (juce::Font (9.0f));
    g.setColour (juce::Colour (0xffaeb6b7));
    g.drawText ("MODULAR EFFECT CHAIN  •  DISTORTION → WAVEGUIDE → RING MOD", 40, 890,
                getWidth() - 80, 14, juce::Justification::centred);
}

void WaveringAudioProcessorEditor::resized()
{
    constexpr int knobWidth = 100;
    constexpr int knobHeight = 142;
    constexpr int gap = 16;
    constexpr int totalWidth = knobWidth * 6 + gap * 5;
    const int startX = (getWidth() - totalWidth) / 2;

    int x = (getWidth() - (112 * 5 + 18 * 4)) / 2;
    driveKnob.setBounds (x, 132, 112, 142); x += 130;
    toneKnob.setBounds (x, 132, 112, 142); x += 130;
    biasKnob.setBounds (x, 132, 112, 142); x += 130;
    bodyKnob.setBounds (x, 132, 112, 142); x += 130;
    mixKnob.setBounds (x, 132, 112, 142);
    distortionSwitch.setBounds (getWidth() / 2 - 88, 303, 176, 44);

    x = startX;
    rateKnob.setBounds (x, 390, knobWidth, knobHeight); x += knobWidth + gap;
    resonanceKnob.setBounds (x, 390, knobWidth, knobHeight); x += knobWidth + gap;
    feedbackKnob.setBounds (x, 390, knobWidth, knobHeight); x += knobWidth + gap;
    dampingKnob.setBounds (x, 390, knobWidth, knobHeight); x += knobWidth + gap;
    characterKnob.setBounds (x, 390, knobWidth, knobHeight); x += knobWidth + gap;
    waveMixKnob.setBounds (x, 390, knobWidth, knobHeight);
    waveguideSwitch.setBounds (getWidth() / 2 - 88, 561, 176, 44);

    constexpr int ringKnobWidth = 112;
    constexpr int ringGap = 18;
    const int ringStart = (getWidth() - (ringKnobWidth * 5 + ringGap * 4)) / 2;
    x = ringStart;
    ringRateKnob.setBounds (x, 648, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringDepthKnob.setBounds (x, 648, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringWaveformLabel.setBounds (x, 656, ringKnobWidth, 20);
    ringWaveformSelector.setBounds (x + 5, 684, ringKnobWidth - 10, 30);
    x += ringKnobWidth + ringGap;
    ringFineKnob.setBounds (x, 648, ringKnobWidth, knobHeight); x += ringKnobWidth + ringGap;
    ringMixKnob.setBounds (x, 648, ringKnobWidth, knobHeight);
    ringModSwitch.setBounds (getWidth() / 2 - 88, 819, 176, 44);
}
