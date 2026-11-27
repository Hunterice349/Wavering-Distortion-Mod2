/*
  ==============================================================================
    PluginProcessor.h
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DistortionDSP.h" // Предполагаем, что этот файл существует

class WaveringDistortionModAudioProcessor : public juce::AudioProcessor
{
public:
    // ... существующий конструктор и деструктор ...

    // --- НОВЫЕ МЕТОДЫ ДЛЯ VU METER ---
    float getRMSLevel() const { return rmsLevel.load(); }
    float getPeakLevel() const { return peakLevel.load(); }

private:
    // --- СУЩЕСТВУЮЩИЕ ЧЛЕНЫ КЛАССА ---
    // (APVTS, DSP объекты и т.д.)
    
    // --- НОВЫЕ ЧЛЕНЫ ДЛЯ VU ---
    std::atomic<float> rmsLevel { 0.0f };
    std::atomic<float> peakLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringDistortionModAudioProcessor)
};