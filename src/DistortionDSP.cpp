#include "DistortionDSP.h"
#include <cmath>
#include <algorithm>

// Helper function to clamp values to prevent NaN/Inf
static inline float safeOutput(float value) {
    if (std::isnan(value) || std::isinf(value)) return 0.0f;
    return std::max(-1.0f, std::min(1.0f, value));
}

// Realistic Wave Shaper using tanh for soft clipping
static inline float distortSample(float input, float drive) {
    // Drive should be >= 1.0 for effect
    if (drive <= 1.0f) return input;
    
    // Apply drive gain
    float driven = input * drive;
    
    // Soft clipping using tanh (realistic analog saturation)
    float distorted = std::tanh(driven);
    
    return distorted;
}

// Simple One-Pole Low Pass Filter for Tone Control
class SimpleLPF {
public:
    void setCutoffFrequency(float sampleRate, float cutoffHz) {
        float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoffHz);
        alpha = sampleRate / (sampleRate + 1.0f / rc);
    }

    float process(float input) {
        output = alpha * input + (1.0f - alpha) * output;
        return output;
    }

    void reset() { output = 0.0f; }

private:
    float alpha = 0.5f;
    float output = 0.0f;
};

DistortionDSP::DistortionDSP() {
    // Initialize states
    toneStateLeft.reset();
    toneStateRight.reset();
}

void DistortionDSP::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Prepare any internal buffers if needed
    toneStateLeft.setCutoffFrequency((float)sampleRate, 20000.0f); // Default open
    toneStateRight.setCutoffFrequency((float)sampleRate, 20000.0f);
}

void DistortionDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Get parameters from your processor state (assuming these are passed in or member variables)
    // You need to ensure these members exist in DistortionDSP.h or are passed as arguments
    // For this example, I assume they are member variables updated by PluginProcessor
    
    float driveGain = juce::jmap(drive, 0.0f, 1.0f, 1.0f, 50.0f); // Map 0-1 to 1x-50x gain
    float toneFreq = juce::jmap(tone, 0.0f, 1.0f, 200.0f, 20000.0f); // Map 0-1 to 200Hz-20kHz
    float mixVal = mix; // 0.0 = dry, 1.0 = wet
    float stereoWidth = body; // Using 'body' as stereo width parameter as per your errors

    // Update filter coefficients
    toneStateLeft.setCutoffFrequency(44100.0f, toneFreq); // Replace with actual sample rate if available
    toneStateRight.setCutoffFrequency(44100.0f, toneFreq);

    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float drySample = channelData[sample];
            float wetSample = drySample;

            // Apply Distortion
            wetSample = distortSample(wetSample, driveGain);

            // Apply Tone Filter
            if (channel == 0) {
                wetSample = toneStateLeft.process(wetSample);
            } else {
                wetSample = toneStateRight.process(wetSample);
            }

            // Enhance 3D/Stereo Effect
            // Slightly differentiate left/right processing if stereo width > 0
            if (stereoWidth > 0.0f && channel == 1) {
                // Simple phase/delay simulation for width
                wetSample *= (1.0f + (stereoWidth * 0.1f)); 
            }

            // Mix Dry/Wet
            float finalSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);

            // Safety clamp
            channelData[sample] = safeOutput(finalSample);
        }
    }
}

void DistortionDSP::reset() {
    toneStateLeft.reset();
    toneStateRight.reset();
}
