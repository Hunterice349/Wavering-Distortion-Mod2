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

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmax (2.0f, radius * 0.08f);
    auto arcRadius = radius - lineW * 2.0f;

    // 1. Базовая дорожка (Темный графит)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                 arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Активная дуга (Неоновое красное свечение)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, toAngle, true);
        
        // Внешнее свечение
        g.setColour (fill.withAlpha (0.2f));
        g.strokePath (valueArc, juce::PathStrokeType (lineW + 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Внутреннее свечение
        g.setColour (fill.withAlpha (0.6f));
        g.strokePath (valueArc, juce::PathStrokeType (lineW + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Основная линия
        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Тело ручки (Фотореалистичный шлифованный металл)
    juce::Path knobCircle;
    knobCircle.addEllipse (bounds.reduced (radius * 0.35f));
    
    // Металлический градиент (сверху вниз)
    juce::ColourGradient grad (juce::Colours::white.withAlpha(0.15f), bounds.getCentreX(), bounds.getY(),
                               juce::Colours::black.withAlpha(0.6f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (knobCircle);
    
    // Металлическая окантовка
    g.setColour (steel);
    g.strokePath (knobCircle, juce::PathStrokeType (1.0f));

    // 4. Индикатор положения (Кроваво-красная неоновая линия)
    juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                   bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));

    juce::Path indicatorLine;
    indicatorLine.startNewSubPath (bounds.getCentre());
    indicatorLine.lineTo (thumbPoint);
    
    // Свечение индикатора
    g.setColour (indicator.withAlpha(0.5f));
    g.strokePath (indicatorLine, juce::PathStrokeType (4.0f));
    // Ядро индикатора
    g.setColour (neonRed);
    g.strokePath (indicatorLine, juce::PathStrokeType (2.0f));
    
    // Центральная точка
    g.setColour (neonRed);
    g.fillEllipse (bounds.getCentreX() - 2, bounds.getCentreY() - 2, 4, 4);
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
        
        // Тонкая рамка для поля ввода
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
    setLookAndFeel (&gothicLF);
    setOpaque (true);

    // Лямбда для быстрой настройки всех ручек
    auto setupKnob = [this] (PremiumKnob& knob) {
        knob.setColour (juce::Slider::textBoxTextColourId, gothicLF.textSilver);
        knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knob.setColour (juce::Slider::rotarySliderFillColourId, gothicLF.neonRed);
        knob.setColour (juce::Slider::rotarySliderOutlineColourId, gothicLF.graphite);
        knob.setLookAndFeel (&gothicLF);
    };

    // --- Distortion Section ---
    addAndMakeVisible (driveKnob); setupKnob(driveKnob);
    driveAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "drive", driveKnob));
    addAndMakeVisible (driveLbl); driveLbl.setText ("DRIVE", juce::dontSendNotification);

    addAndMakeVisible (toneKnob); setupKnob(toneKnob);
    toneAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "tone", toneKnob));
    addAndMakeVisible (toneLbl); toneLbl.setText ("TONE", juce::dontSendNotification);

    addAndMakeVisible (biasKnob); setupKnob(biasKnob);
    biasAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "bias", biasKnob));
    addAndMakeVisible (biasLbl); biasLbl.setText ("BIAS", juce::dontSendNotification);

    addAndMakeVisible (bodyKnob); setupKnob(bodyKnob);
    bodyAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "body", bodyKnob));
    addAndMakeVisible (bodyLbl); bodyLbl.setText ("BODY", juce::dontSendNotification);

    addAndMakeVisible (mixKnob); setupKnob(mixKnob);
    mixAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "mix", mixKnob));
    addAndMakeVisible (mixLbl); mixLbl.setText ("MIX", juce::dontSendNotification);

    addAndMakeVisible (distOnBtn);
    distOnBtn.setButtonText ("DISTORTION ON");
    distOnBtn.setClickingTogglesState (true);
    distOnAtt.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "distortionEnabled", distOnBtn));

    // --- Waveguide Section ---
    addAndMakeVisible (rateKnob); setupKnob(rateKnob);
    rateAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveRate", rateKnob));
    addAndMakeVisible (rateLbl); rateLbl.setText ("RATE", juce::dontSendNotification);

    addAndMakeVisible (resKnob); setupKnob(resKnob);
    resAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveResonance", resKnob));
    addAndMakeVisible (resLbl); resLbl.setText ("RESONANCE", juce::dontSendNotification);

    addAndMakeVisible (fbKnob); setupKnob(fbKnob);
    fbAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveFeedback", fbKnob));
    addAndMakeVisible (fbLbl); fbLbl.setText ("FEEDBACK", juce::dontSendNotification);

    addAndMakeVisible (dampKnob); setupKnob(dampKnob);
    dampAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveDamping", dampKnob));
    addAndMakeVisible (dampLbl); dampLbl.setText ("DAMPING", juce::dontSendNotification);

    addAndMakeVisible (charKnob); setupKnob(charKnob);
    charAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveCharacter", charKnob));
    addAndMakeVisible (charLbl); charLbl.setText ("CHARACTER", juce::dontSendNotification);

    addAndMakeVisible (waveMixKnob); setupKnob(waveMixKnob);
    waveMixAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "waveMix", waveMixKnob));
    addAndMakeVisible (waveMixLbl); waveMixLbl.setText ("WAVE MIX", juce::dontSendNotification);

    addAndMakeVisible (waveOnBtn);
    waveOnBtn.setButtonText ("WAVEGUIDE ON");
    waveOnBtn.setClickingTogglesState (true);
    waveOnAtt.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "waveguideEnabled", waveOnBtn));

    // --- Title ---
    addAndMakeVisible (titleLbl);
    titleLbl.setText ("WAVERING DISTORTION MOD2", juce::dontSendNotification);
    titleLbl.setFont (juce::Font ("Courier New", 22.0f, juce::Font::bold));
    titleLbl.setColour (juce::Label::textColourId, gothicLF.neonRed);

    setSize (900, 520);
    startTimerHz (30);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Фон: Глубокий черно-графитовый градиент
    juce::ColourGradient bgGrad (gothicLF.bgBlack, 0, 0,
                                 gothicLF.graphite, 0, (float) getHeight(), false);
    g.setFillType (bgGrad);
    g.fillAll();

    // 2. Металлические панели модулей
    auto drawPanel = [&g, &gothicLF] (juce::Rectangle<int> bounds, const juce::String& title)
    {
        juce::ColourGradient plateGrad (gothicLF.steel, bounds.getX(), bounds.getY(),
                                        gothicLF.graphite, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (plateGrad);
        g.fillRoundedRectangle (bounds.toFloat(), 10.0f);
        
        // Красная неоновая окантовка
        g.setColour (gothicLF.bloodRed.withAlpha(0.6f));
        g.drawRoundedRectangle (bounds.toFloat(), 10.0f, 2.0f);
        
        // Внутренняя тень для эффекта углубления
        g.setColour (juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle (bounds.reduced(2).toFloat(), 10.0f, 1.0f);
    };

    drawPanel (distortionArea, "DISTORTION MODULE");
    drawPanel (waveguideArea, "WAVEGUIDE MODULE");

    // 3. Декоративные винты по углам корпуса
    auto drawScrew = [&g] (int x, int y)
    {
        g.setColour (juce::Colours::darkgrey);
        g.fillEllipse (x-4, y-4, 8, 8);
        g.setColour (juce::Colours::silver);
        g.fillEllipse (x-2, y-2, 4, 4);
    };
    
    drawScrew (12, 12);
    drawScrew (getWidth()-12, 12);
    drawScrew (12, getHeight()-12);
    drawScrew (getWidth()-12, getHeight()-12);
}

void WaveringAudioProcessorEditor::resized()
{
    const int panelMargin = 20;
    const int knobSize = 70;
    const int labelHeight = 20;
    const int btnHeight = 30;
    
    // Разделение на две панели
    distortionArea = getLocalBounds().removeFromLeft (getWidth() / 2).reduced (panelMargin);
    waveguideArea = getLocalBounds().reduced (panelMargin);
    
    // Заголовок
    titleLbl.setBounds (getLocalBounds().removeFromTop (50).withTrimmedLeft(20));

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
    // Можно добавить легкую пульсацию неона, если потребуется
}