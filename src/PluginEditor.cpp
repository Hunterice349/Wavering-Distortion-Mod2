/*
  ==============================================================================
    PluginEditor.cpp
    WAVERING DISTORTION MOD - Premium GUI Implementation
  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

// ==============================================================================
// VU METER IMPLEMENTATION
// ==============================================================================

void WaveringDistortionModAudioProcessorEditor::VUMeterComponent::updateLevels(float rms, float peak)
{
    // Простое сглаживание для стрелки можно добавить здесь, но пока берем сырые данные
    // Для плавности лучше использовать interpolation в paint или отдельном таймере компонента
    currentRms = rms;
}

void WaveringDistortionModAudioProcessorEditor::VUMeterComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 1. Фон (темное стекло/металл)
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillRoundedRectangle(bounds, 15.0f);
    
    // 2. Рамка
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 15.0f, 2.0f);
    
    // 3. Шкала
    float centerX = bounds.getWidth() / 2.0f;
    float centerY = bounds.getHeight() * 0.85f;
    float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(10.0f);
    
    // Рисуем деления (-20dB to +3dB)
    for (int db = -20; db <= 3; db += 1)
    {
        // Маппинг: -20dB -> -60 deg, 0dB -> 0 deg, +3dB -> +10 deg
        float norm = (float)(db + 20) / 23.0f; 
        float angleDeg = -60.0f + (norm * 70.0f);
        float angleRad = juce::degreesToRadians(angleDeg);
        
        float x1 = centerX + std::cos(angleRad) * (radius - 5);
        float y1 = centerY - std::sin(angleRad) * (radius - 5);
        float x2 = centerX + std::cos(angleRad) * radius;
        float y2 = centerY - std::sin(angleRad) * radius;
        
        g.drawLine(x1, y1, x2, y2, db % 5 == 0 ? 2.0f : 1.0f);
        
        if (db % 5 == 0)
        {
            float tx = centerX + std::cos(angleRad) * (radius - 20);
            float ty = centerY - std::sin(angleRad) * (radius - 20);
            g.drawText(juce::String(db), juce::Rectangle<float>(tx-10, ty-10, 20, 20), juce::Justification::centred);
        }
    }
    
    // Красная зона
    g.setColour(juce::Colours::red.withAlpha(0.2f));
    g.fillArc(centerX, centerY, radius*2, radius*2, 
              juce::degreesToRadians(-5.0f), juce::degreesToRadians(15.0f), true);

    // 4. Стрелка
    // Конвертация RMS (0..1) в dB
    float dbFS = juce::Decibels::gainToDecibels(currentRms, -100.0f);
    
    // Маппинг dB в угол
    float targetNorm = (dbFS + 20.0f) / 23.0f;
    targetNorm = jlimit(0.0f, 1.0f, targetNorm);
    
    float targetAngleDeg = -60.0f + (targetNorm * 70.0f);
    
    // Инерция стрелки
    needlePos = needlePos + (targetAngleDeg - needlePos) * 0.15f;
    
    float needleRad = juce::degreesToRadians(needlePos);
    
    g.setColour(juce::Colours::black);
    g.fillEllipse(centerX - 6, centerY - 6, 12, 12);
    
    g.setColour(juce::Colours::orangeRed);
    g.drawLine(centerX, centerY, 
               centerX + std::cos(needleRad) * (radius - 2), 
               centerY - std::sin(needleRad) * (radius - 2), 
               2.0f);
}

// ==============================================================================
// PREMIUM KNOB IMPLEMENTATION
// ==============================================================================

WaveringDistortionModAudioProcessorEditor::PremiumKnob::PremiumKnob(juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& lbl)
    : attachment(*state.getParameter(paramID), [this](float) { repaint(); }),
      label(lbl)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WaveringDistortionModAudioProcessorEditor::PremiumKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float diameter = jmin(bounds.getWidth(), bounds.getHeight());
    juce::Rectangle<float> knobRect(bounds.getX() + (bounds.getWidth()-diameter)/2, 
                                    bounds.getY() + (bounds.getHeight()-diameter)/2, 
                                    diameter, diameter);
                                    
    // Тень
    g.setDropShadow(juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 8, juce::Point<int>(0, 4)));
    g.fillEllipse(knobRect);
    g.dropShadow(); 
    
    // Металлический корпус (градиент)
    juce::ColourGradient metalGrad(juce::Colours::silver, knobRect.getX(), knobRect.getY(),
                                   juce::Colours::darkgrey, knobRect.getX(), knobRect.getBottom(), true);
    g.setGradientFill(metalGrad);
    g.fillEllipse(knobRect.reduced(2));
    
    // Получаем значение (0.0 - 1.0)
    // Используем attachment.parameter->getValue(), так как attachment сам по себе не имеет getValue() в старых версиях JUCE
    float value = attachment.parameter->getValue();
    
    // Угол (-135 to +135)
    float angle = -135.0f + (value * 270.0f);
    float rad = juce::degreesToRadians(angle);
    
    // Индикатор
    float indRadius = diameter * 0.35f;
    float ix = knobRect.getCentreX() + std::cos(rad) * indRadius;
    float iy = knobRect.getCentreY() - std::sin(rad) * indRadius;
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(ix - 3, iy - 3, 6, 6);
    
    // Подпись
    g.setColour(juce::Colours::silver);
    g.setFont(12.0f);
    
    // FIX C2662: Создаем копию bounds для манипуляций
    auto textBounds = bounds;
    auto labelArea = textBounds.removeFromBottom(16.0f);
    g.drawText(label, labelArea, juce::Justification::centred);
}

void WaveringDistortionModAudioProcessorEditor::PremiumKnob::mouseDown(const juce::MouseEvent& e)
{
    isDragging = true;
    lastY = (float)e.y;
}

void WaveringDistortionModAudioProcessorEditor::PremiumKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging) return;
    
    float deltaY = lastY - (float)e.y;
    lastY = (float)e.y;
    
    // Чувствительность
    float sensitivity = 0.005f;
    float currentValue = attachment.parameter->getValue();
    float newValue = currentValue + (deltaY * sensitivity);
    newValue = jlimit(0.0f, 1.0f, newValue);
    
    attachment.setValueAsPartOfGesture(newValue);
    repaint();
}

void WaveringDistortionModAudioProcessorEditor::PremiumKnob::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
}

// ==============================================================================
// POWER FOOTSWITCH IMPLEMENTATION
// ==============================================================================

WaveringDistortionModAudioProcessorEditor::PowerFootswitch::PowerFootswitch(juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
    : attachment(*state.getParameter(paramID), [this](float value) { 
        ledOn = value > 0.5f; 
        repaint(); 
    })
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Хромированный корпус
    juce::ColourGradient chromeGrad(juce::Colours::silver, bounds.getX(), bounds.getY(),
                                    juce::Colours::grey, bounds.getX(), bounds.getBottom(), true);
    g.setGradientFill(chromeGrad);
    
    // Эффект нажатия
    float shrink = isPressed ? 4.0f : 0.0f;
    juce::Rectangle<float> switchRect = bounds.reduced(shrink);
    
    g.fillEllipse(switchRect);
    
    // Блик
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.fillEllipse(switchRect.getX() + switchRect.getWidth()*0.2, 
                  switchRect.getY() + switchRect.getHeight()*0.1, 
                  switchRect.getWidth()*0.3, 
                  switchRect.getHeight()*0.2);
                  
    // LED
    float ledSize = 12.0f;
    juce::Rectangle<float> ledRect(bounds.getCentreX() - ledSize/2, bounds.getBottom() - 30, ledSize, ledSize);
    
    if (ledOn)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.9f));
        g.setDropShadow(juce::DropShadow(juce::Colours::orange, 15, juce::Point<int>(0,0)));
    }
    else
    {
        g.setColour(juce::Colours::darkred);
    }
    
    g.fillEllipse(ledRect);
    g.setDropShadow(juce::DropShadow()); // Reset
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::mouseDown(const juce::MouseEvent& e)
{
    isPressed = true;
    repaint();
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::mouseUp(const juce::MouseEvent& e)
{
    isPressed = false;
    // Latching logic
    float currentState = attachment.parameter->getValue();
    float newState = currentState > 0.5f ? 0.0f : 1.0f;
    attachment.setValueAsPartOfGesture(newState);
    repaint();
}

// ==============================================================================
// MAIN EDITOR IMPLEMENTATION
// ==============================================================================

WaveringDistortionModAudioProcessorEditor::WaveringDistortionModAudioProcessorEditor (WaveringDistortionModAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Инициализация компонентов
    vuMeter = std::make_unique<VUMeterComponent>(audioProcessor);
    addAndMakeVisible(vuMeter.get());
    
    // ВАЖНО: Убедитесь, что ID "distortionEnabled" совпадает с вашим PluginProcessor
    footSwitch = std::make_unique<PowerFootswitch>(audioProcessor.treeState, "distortionEnabled"); 
    addAndMakeVisible(footSwitch.get());
    
    // Knobs (ID должны совпадать с PluginProcessor)
    knobDrive = std::make_unique<PremiumKnob>(audioProcessor.treeState, "drive", "DRIVE");
    addAndMakeVisible(knobDrive.get());
    
    knobTone = std::make_unique<PremiumKnob>(audioProcessor.treeState, "tone", "TONE");
    addAndMakeVisible(knobTone.get());
    
    knobBias = std::make_unique<PremiumKnob>(audioProcessor.treeState, "bias", "BIAS");
    addAndMakeVisible(knobBias.get());
    
    knobBody = std::make_unique<PremiumKnob>(audioProcessor.treeState, "body", "BODY");
    addAndMakeVisible(knobBody.get());
    
    knobMix = std::make_unique<PremiumKnob>(audioProcessor.treeState, "mix", "MIX");
    addAndMakeVisible(knobMix.get());
    
    setSize (960, 620);
    startTimerHz(60);
}

WaveringDistortionModAudioProcessorEditor::~WaveringDistortionModAudioProcessorEditor()
{
    stopTimer();
}

void WaveringDistortionModAudioProcessorEditor::timerCallback()
{
    if (vuMeter)
    {
        float rms = audioProcessor.getRMSLevel();
        vuMeter->updateLevels(rms, 0.0f);
        vuMeter->repaint();
    }
}

void WaveringDistortionModAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // 1. Деревянные боковины
    juce::Colour woodDark = juce::Colour::fromRGB(60, 40, 20);
    juce::Colour woodLight = juce::Colour::fromRGB(100, 70, 40);
    juce::ColourGradient woodGrad(woodDark, 0, 0, woodLight, bounds.getWidth(), 0, false);
    
    g.setGradientFill(woodGrad);
    g.fillRect(0, 0, 80, bounds.getHeight());
    g.fillRect(bounds.getWidth() - 80, 0, 80, bounds.getHeight());
    
    // 2. Основная панель (Черный матовый металл)
    juce::Rectangle<int> mainPanel = bounds.reduced(80, 20);
    g.setColour(juce::Colour::fromRGB(25, 25, 30));
    g.fillRect(mainPanel);
    
    // Текстура (шум)
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (int i = 0; i < mainPanel.getHeight(); i+=2)
    {
        g.drawLine(mainPanel.getX(), mainPanel.getY() + i, mainPanel.getRight(), mainPanel.getY() + i, 1.0f);
    }
    
    // 3. Название
    g.setColour(juce::Colours::silver);
    g.setFont(juce::Font("Arial Bold", 20.0f, juce::Font::plain));
    g.drawText("WAVERING DISTORTION MOD", mainPanel.removeFromTop(40), juce::Justification::centred);
}

void WaveringDistortionModAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(80, 20);
    
    // VU Meter
    auto vuArea = bounds.removeFromTop(250).reduced(150, 20);
    vuMeter->setBounds(vuArea);
    
    // Knobs Row
    auto knobsArea = bounds.removeFromTop(150);
    int knobWidth = knobsArea.getWidth() / 5;
    
    knobDrive->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobTone->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobBias->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobBody->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobMix->setBounds(knobsArea.removeFromLeft(knobWidth));
    
    // Footswitch
    auto footArea = bounds.removeFromBottom(120).reduced(250, 20);
    footSwitch->setBounds(footArea);
}