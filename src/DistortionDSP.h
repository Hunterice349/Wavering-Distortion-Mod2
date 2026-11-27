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

    // Параметры (обновляются из PluginProcessor)
    float drive = 0.5f;   // Сила перегруза
    float tone = 0.5f;    // Тембр (Low Pass Filter)
    float mix = 1.0f;     // Баланс Dry/Wet
    float body = 0.5f;    // Параметр "Тела" / Стерео-ширины (3D)

private:
    // Структура простого фильтра низких частот
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

    // Реалистичное мягкое клиппирование (Tanh)
    static inline float distortSample(float input, float driveGain) {
        if (driveGain <= 1.0f) return input;
        return std::tanh(input * driveGain);
    }
    
    // Защита от NaN и бесконечности
    static inline float safeOutput(float value) {
        if (std::isnan(value) || std::isinf(value)) return 0.0f;
        return juce::jlimit(-1.0f, 1.0f, value);
    }
};
