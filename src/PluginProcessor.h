
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DistortionDSP.h"
#include "WaveguideDSP.h"
#include "RingModulatorDSP.h"
#include "SpectralDelayDSP.h"

class WaveringAudioProcessor : public juce::AudioProcessor
{
public:
    WaveringAudioProcessor();
    ~WaveringAudioProcessor() override = default;

    // Audio processing
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages
    ) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;

    bool hasEditor() const override
    {
        return true;
    }

    // Plugin information
    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override
    {
        return false;
    }

    bool producesMidi() const override
    {
        return false;
    }

    bool isMidiEffect() const override
    {
        return false;
    }

    double getTailLengthSeconds() const override;

    // Program management
    int getNumPrograms() override
    {
        return 1;
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram (int index) override
    {
        juce::ignoreUnused (index);
    }

    const juce::String getProgramName (int index) override
    {
        if (index == 0)
            return "Default";

        return {};
    }

    void changeProgramName (
        int index,
        const juce::String& newName
    ) override
    {
        juce::ignoreUnused (index, newName);
    }

    // State management
    void getStateInformation (
        juce::MemoryBlock& destData
    ) override;

    void setStateInformation (
        const void* data,
        int sizeInBytes
    ) override;

    // Parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    juce::AudioProcessorValueTreeState apvts {
        *this,
        nullptr,
        "Parameters",
        createParameterLayout()
    };

private:
    // DSP modules
    DistortionDSP distortion;
    WaveguideDSP waveguide;
    RingModulatorDSP ringModulator;
    SpectralDelayDSP spectralDelay;

    // Audio configuration
    double currentSampleRate = 44100.0;

    // Parameter readers
    DistortionDSP::Parameters
    readDistortionParameters() const noexcept;

    WaveguideDSP::Parameters
    readWaveguideParameters() const noexcept;

    RingModulatorDSP::Parameters
    readRingModulatorParameters() const noexcept;

    SpectralDelayDSP::Parameters
    readSpectralDelayParameters() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        WaveringAudioProcessor
    )
};
