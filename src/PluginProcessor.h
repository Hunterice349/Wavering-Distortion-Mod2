
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DistortionDSP.h"
#include "WaveguideDSP.h"

class WaveringAudioProcessor : public juce::AudioProcessor
{
public:
    WaveringAudioProcessor();
    ~WaveringAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor();

    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    // Dynamic Waveguide tail calculation.
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    const juce::String getProgramName (int index) override
    {
        return index == 0 ? juce::String ("Default") : juce::String{};
    }
    void changeProgramName (int index, const juce::String& newName) override
    {
        juce::ignoreUnused (index, newName);
    }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts {
        *this, nullptr, "Parameters", createParameterLayout()
    };

private:
    DistortionDSP distortion;
    WaveguideDSP waveguide;

    double currentSampleRate = 44100.0;

    DistortionDSP::Parameters readDistortionParameters() const noexcept;
    WaveguideDSP::Parameters readWaveguideParameters() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessor)
};
