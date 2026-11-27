#include "DistortionDSP.h"

DistortionDSP::DistortionDSP() {
    toneStateLeft.reset();
    toneStateRight.reset();
}

void DistortionDSP::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    toneStateLeft.setCutoffFrequency((float)currentSampleRate, 20000.0f);
    toneStateRight.setCutoffFrequency((float)currentSampleRate, 20000.0f);
}

void DistortionDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numSamples == 0) return;

    float driveGain = juce::jmap(drive, 0.0f, 1.0f, 1.0f, 60.0f);
    float toneFreq = juce::jmap(tone, 0.0f, 1.0f, 150.0f, 18000.0f);
    float mixVal = juce::jlimit(0.0f, 1.0f, mix);
    float stereoWidth = juce::jlimit(0.0f, 1.0f, body);

    toneStateLeft.setCutoffFrequency((float)currentSampleRate, toneFreq);
    toneStateRight.setCutoffFrequency((float)currentSampleRate, toneFreq);

    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float drySample = channelData[sample];
            float wetSample = drySample;

            wetSample = distortSample(wetSample, driveGain);

            if (channel == 0) {
                wetSample = toneStateLeft.process(wetSample);
            } else {
                wetSample = toneStateRight.process(wetSample);
            }

            // 3D Effect: slight boost on right channel based on 'body' parameter
            if (stereoWidth > 0.0f && channel == 1) {
                wetSample *= (1.0f + (stereoWidth * 0.12f)); 
            }

            float finalSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);
            channelData[sample] = safeOutput(finalSample);
        }
    }
}

void DistortionDSP::reset() {
    toneStateLeft.reset();
    toneStateRight.reset();
}
