#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#include "DistortionDSP.h"
#include "WaveguideDSP.h"

class WaveringAudioProcessor : public juce::AudioProcessor
{
public:
    WaveringAudioProcessor();
    ~WaveringAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&
    ) override;

    juce::AudioProcessorEditor* createEditor() override;

    bool hasEditor() const override
    {
        return true;
    }

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

    int getNumPrograms() override
    {
        return 1;
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram(int index) override
    {
        juce::ignoreUnused(index);
    }

    const juce::String getProgramName(int index) override
    {
        return index == 0 ? juce::String("Default") : juce::String{};
    }

    void changeProgramName(
        int index,
        const juce::String& newName
    ) override
    {
        juce::ignoreUnused(index, newName);
    }

    void getStateInformation(
        juce::MemoryBlock& destData
    ) override;

    void setStateInformation(
        const void* data,
        int sizeInBytes
    ) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    juce::AudioProcessorValueTreeState apvts {
        *this,
        nullptr,
        "Parameters",
        createParameterLayout()
    };

    // VU meter: данные выходного сигнала.
    float getVuRms() const noexcept
    {
        return vuRms.load(std::memory_order_relaxed);
    }

    float getVuPeak() const noexcept
    {
        return vuPeak.load(std::memory_order_relaxed);
    }

private:
    DistortionDSP distortion;
    WaveguideDSP waveguide;

    double currentSampleRate = 44100.0;

    // Audio thread -> GUI thread.
    std::atomic<float> vuRms { 0.0f };
    std::atomic<float> vuPeak { 0.0f };

    DistortionDSP::Parameters
    readDistortionParameters() const noexcept;

    WaveguideDSP::Parameters
    readWaveguideParameters() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        WaveringAudioProcessor
    )
};