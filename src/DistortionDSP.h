#pragma once

#include <JuceHeader.h>

class DistortionDSP {
public:
    DistortionDSP();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void reset();

    // Parameters - Ensure these are set by PluginProcessor before calling processBlock
    float drive = 0.5f;
    float tone = 0.5f;
    float mix = 1.0f;
    float body = 0.0f; // Used as stereo width/enhancement

private:
    // Internal state for filters
    struct SimpleLPF {
        float alpha = 0.5f;
        float output = 0.0f;
        
        void setCutoffFrequency(float sampleRate, float cutoffHz) {
            float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoffHz);
            alpha = sampleRate / (sampleRate + 1.0f / rc);
        }
        
        float process(float input) {
            output = alpha * input + (1.0f - alpha) * output;
            return output;
        }
        
        void reset() { output = 0.0f; }
    };

    SimpleLPF toneStateLeft;
    SimpleLPF toneStateRight;
};
