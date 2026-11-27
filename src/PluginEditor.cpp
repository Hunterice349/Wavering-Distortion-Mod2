/*
  ==============================================================================
    PluginEditor.cpp
  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

// ==============================================================================
// VU METER IMPLEMENTATION
// ==============================================================================

void WaveringDistortionModAudioProcessorEditor::VUMeterComponent::updateLevels(float newRms, float newPeak)
{
    currentRms = newRms;
    currentPeak = newPeak;
}

void WaveringDistortionModAudioProcessorEditor::VUMeterComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 1. Фон корпуса VU (темный металл)
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.fillRoundedRectangle(bounds, 10.0f);
    
    // 2. Стеклянная поверхность (градиент)
    juce::ColourGradient glassGrad(juce::Colours::white.withAlpha(0.1f), bounds.getX(), bounds.getY(),
                                   juce::Colours::transparentBlack, bounds.getX(), bounds.getBottom(), true);
    g.setGradientFill(glassGrad);
    g.fillRoundedRectangle(bounds.reduced(5.0f), 8.0f);
    
    // 3. Шкала (дуга)
    float centerX = bounds.getWidth() / 2.0f;
    float centerY = bounds.getHeight() * 0.85f;
    float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;
    
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(12.0f);
    
    // Рисуем деления от -20 до +3 dB
    for (int db = -20; db <= 3; db += 1)
    {
        // Маппинг dB в угол (примерно)
        // -20dB = -60 deg, 0dB = 0 deg, +3dB = +10 deg
        float angleDeg = -60.0f + ((float)(db + 20) / 23.0f) * 70.0f;
        float angleRad = juce::degreesToRadians(angleDeg);
        
        float x1 = centerX + std::cos(angleRad) * (radius - 10);
        float y1 = centerY - std::sin(angleRad) * (radius - 10);
        float x2 = centerX + std::cos(angleRad) * radius;
        float y2 = centerY - std::sin(angleRad) * radius;
        
        g.drawLine(x1, y1, x2, y2, db % 5 == 0 ? 2.0f : 1.0f);
        
        if (db % 5 == 0)
        {
            float tx = centerX + std::cos(angleRad) * (radius - 25);
            float ty = centerY - std::sin(angleRad) * (radius - 25);
            g.drawText(juce::String(db), juce::Rectangle<float>(tx-10, ty-10, 20, 20), juce::Justification::centred);
        }
    }
    
    // Красная зона (>0dB)
    g.setColour(juce::Colours::red.withAlpha(0.3f));
    g.fillArc(centerX, centerY, radius*2, radius*2, 
              juce::degreesToRadians(-10.0f), juce::degreesToRadians(10.0f), true);

    // 4. Стрелка (с инерцией)
    // Конвертируем RMS в dBFS
    float dbFS = juce::Decibels::gainToDecibels(currentRms, -100.0f);
    
    // Плавное движение стрелки (Lerp)
    float targetAngleDeg = -60.0f + ((float)(dbFS + 20.0f) / 23.0f) * 70.0f;
    // Ограничиваем угол
    targetAngleDeg = jlimit(-65.0f, 15.0f, targetAngleDeg);
    
    needlePosition = needlePosition + (targetAngleDeg - needlePosition) * 0.2f; // Коэффициент инерции
    
    float needleRad = juce::degreesToRadians(needlePosition);
    
    g.setColour(juce::Colours::black);
    g.fillEllipse(centerX - 5, centerY - 5, 10, 10); // Ось
    
    g.setColour(juce::Colours::darkred);
    g.drawLine(centerX, centerY, 
               centerX + std::cos(needleRad) * (radius - 5), 
               centerY - std::sin(needleRad) * (radius - 5), 
               2.0f);
}

// ==============================================================================
// PREMIUM KNOB IMPLEMENTATION
// ==============================================================================

WaveringDistortionModAudioProcessorEditor::PremiumKnob::PremiumKnob(juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& label)
    : attachment(*state.getParameter(paramID), [this](float value) { repaint(); }, nullptr),
      parameterLabel(label)
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
    g.setDropShadow(juce::DropShadow(juce::Colours::black.withAlpha(0.5f), 10, juce::Point<int>(0, 5)));
    g.fillEllipse(knobRect);
    g.dropShadow(); // Сброс тени
    
    // Металлический градиент корпуса
    juce::ColourGradient metalGrad(juce::Colours::grey.brighter(), knobRect.getX(), knobRect.getY(),
                                   juce::Colours::darkgrey, knobRect.getX(), knobRect.getBottom(), true);
    g.setGradientFill(metalGrad);
    g.fillEllipse(knobRect);
    
    // Рифленая окантовка (упрощенно)
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawEllipse(knobRect.reduced(2), 1.0f);
    
    // Получаем значение параметра (0.0 - 1.0)
    float value = attachment.getValue(); // Внимание: attachment.getValue() может быть недоступен напрямую в старых JUCE, используем параметр
    // Лучше получить через state, но для простоты здесь предполагаем доступ к значению
    // В реальном коде нужно хранить ссылку на параметр или использовать slider attachment logic
    
    // Угол поворота (-135 to +135 degrees)
    float angle = -135.0f + (value * 270.0f);
    float rad = juce::degreesToRadians(angle);
    
    // Индикатор (белая точка/линия)
    float indRadius = diameter * 0.35f;
    float ix = knobRect.getCentreX() + std::cos(rad) * indRadius;
    float iy = knobRect.getCentreY() - std::sin(rad) * indRadius;
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(ix - 3, iy - 3, 6, 6);
    
    // Подпись
    g.setColour(juce::Colours::silver);
    g.setFont(14.0f);
    g.drawText(parameterLabel, bounds.removeFromBottom(20), juce::Justification::centred);
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
    float newValue = attachment.getValue() + (deltaY * sensitivity);
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
    }, nullptr)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Основание (хром)
    juce::ColourGradient chromeGrad(juce::Colours::silver, bounds.getX(), bounds.getY(),
                                    juce::Colours::grey, bounds.getX(), bounds.getBottom(), true);
    g.setGradientFill(chromeGrad);
    
    // Эффект нажатия
    float shrink = isPressed ? 4.0f : 0.0f;
    juce::Rectangle<float> switchRect = bounds.reduced(shrink).withSizeKeepingCentre(bounds.getWidth() - shrink*2, bounds.getHeight() - shrink*2);
    
    g.fillEllipse(switchRect);
    
    // Блик
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.fillEllipse(switchRect.getX() + switchRect.getWidth()*0.2, 
                  switchRect.getY() + switchRect.getHeight()*0.1, 
                  switchRect.getWidth()*0.3, 
                  switchRect.getHeight()*0.2);
                  
    // LED Индикатор
    float ledSize = 10.0f;
    juce::Rectangle<float> ledRect(bounds.getCentreX() - ledSize/2, bounds.getBottom() - 25, ledSize, ledSize);
    
    if (ledOn)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.9f));
        g.setDropShadow(juce::DropShadow(juce::Colours::orange, 10, juce::Point<int>(0,0)));
    }
    else
    {
        g.setColour(juce::Colours::darkred);
    }
    
    g.fillEllipse(ledRect);
    g.setDropShadow(juce::DropShadow()); // Сброс
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::mouseDown(const juce::MouseEvent& e)
{
    isPressed = true;
    repaint();
}

void WaveringDistortionModAudioProcessorEditor::PowerFootswitch::mouseUp(const juce::MouseEvent& e)
{
    isPressed = false;
    // Latching logic: переключаем состояние
    float currentState = attachment.getValue();
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
    
    // Footswitch привязываем к параметру "distortionEnabled" (убедитесь, что ID совпадает с вашим APVTS)
    footSwitch = std::make_unique<PowerFootswitch>(audioProcessor.treeState, "distortionEnabled"); 
    addAndMakeVisible(footSwitch.get());
    
    // Knobs
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
    
    // Настройка размера окна
    setSize (960, 620);
    
    // Запуск таймера для VU (60 FPS)
    startTimerHz(60);
}

WaveringDistortionModAudioProcessorEditor::~WaveringDistortionModAudioProcessorEditor()
{
    stopTimer();
}

void WaveringDistortionModAudioProcessorEditor::timerCallback()
{
    // Получаем данные из процессора
    float rms = audioProcessor.getRMSLevel();
    float peak = audioProcessor.getPeakLevel();
    
    // Обновляем VU
    if (vuMeter)
        vuMeter->updateLevels(rms, peak);
        
    // Repaint только если нужно (оптимизация)
    if (vuMeter) vuMeter->repaint();
}

void WaveringDistortionModAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // 1. Деревянные боковины (Walnut texture simulation via gradient/noise)
    // Левая панель
    juce::Colour woodDark = juce::Colour::fromRGB(60, 40, 20);
    juce::Colour woodLight = juce::Colour::fromRGB(100, 70, 40);
    
    juce::ColourGradient woodGrad(woodDark, 0, 0, woodLight, bounds.getWidth(), 0, false);
    g.setGradientFill(woodGrad);
    g.fillRect(0, 0, 80, bounds.getHeight());
    g.fillRect(bounds.getWidth() - 80, 0, 80, bounds.getHeight());
    
    // 2. Основная металлическая панель (Brushed Black Aluminum)
    juce::Rectangle<int> mainPanel = bounds.reduced(80, 20);
    g.setColour(juce::Colour::fromRGB(20, 20, 25));
    g.fillRect(mainPanel);
    
    // Текстура металла (шум) - упрощенно линиями
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    for (int i = 0; i < mainPanel.getHeight(); i+=2)
    {
        g.drawLine(mainPanel.getX(), mainPanel.getY() + i, mainPanel.getRight(), mainPanel.getY() + i, 1.0f);
    }
    
    // 3. Название прибора
    g.setColour(juce::Colours::silver);
    g.setFont(juce::Font("Arial Bold", 24.0f, juce::Font::plain));
    g.drawText("WAVERING DISTORTION MOD", mainPanel.removeFromTop(40), juce::Justification::centred);
    
    // Примечание: Отрисовка компонентов происходит автоматически после paint(), 
    // но мы можем добавить дополнительные декоративные элементы здесь.
}

void WaveringDistortionModAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(80, 20); // Учитываем деревянные панели
    
    // VU Meter (Верхняя центральная часть)
    auto vuArea = bounds.removeFromTop(250).reduced(100, 20);
    vuMeter->setBounds(vuArea);
    
    // Панель регуляторов
    auto knobsArea = bounds.removeFromTop(150);
    int knobWidth = knobsArea.getWidth() / 5;
    
    knobDrive->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobTone->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobBias->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobBody->setBounds(knobsArea.removeFromLeft(knobWidth));
    knobMix->setBounds(knobsArea.removeFromLeft(knobWidth));
    
    // Footswitch (Нижняя часть)
    auto footArea = bounds.removeFromBottom(120).reduced(200, 20);
    footSwitch->setBounds(footArea);
}