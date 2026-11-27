#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

class DistortionDSP {
public:
    DistortionDSP();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void reset();

    // Параметры (должны обновляться из PluginProcessor перед обработкой)
    float drive = 0.5f;   // 0.0 (чистый) до 1.0 (максимальный перегруз)
    float tone = 0.5f;    // 0.0 (темный) до 1.0 (яркий)
    float mix = 1.0f;     // 0.0 (Dry) до 1.0 (Wet)
    float body = 0.5f;    // Используется для усиления 3D/стерео-эффекта

private:
    // Простой однополюсный фильтр низких частот (LPF) для контроля тона
    struct SimpleLPF {
        float alpha = 0.5f;
        float output = 0.0f;
        
        void setCutoffFrequency(float sampleRate, float cutoffHz) {
            if (sampleRate <= 0.0f) return;
            float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoffHz);
            alpha = sampleRate / (sampleRate + 1.0f / rc);
            alpha = juce::jlimit(0.0f, 1.0f, alpha);
        }
        
        float process(float input) {
            output = alpha * input + (1.0f - alpha) * output;
            return output;
        }
        
        void reset() { output = 0.0f; }
    };

    SimpleLPF toneStateLeft;
    SimpleLPF toneStateRight;
    double currentSampleRate = 44100.0;

    // Реалистичное искажение (мягкое клиппирование, как в аналоговых педалях)
    static inline float distortSample(float input, float driveGain) {
        if (driveGain <= 1.0f) return input;
        float driven = input * driveGain;
        return std::tanh(driven); // tanh дает гладкое, музыкальное насыщение
    }
    
    // Защита от NaN и бесконечности (Inf)
    static inline float safeOutput(float value) {
        if (std::isnan(value) || std::isinf(value)) return 0.0f;
        return juce::jlimit(-1.0f, 1.0f, value);
    }
};
