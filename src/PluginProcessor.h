
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

#include "DistortionDSP.h"
#include "WaveguideDSP.h"

class WaveringAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int visualizationBufferSize = 2048;

    struct VisualizationData
    {
        std::array<float, visualizationBufferSize> input {};
        std::array<float, visualizationBufferSize> output {};
        int numSamples = 0;
    };

    WaveringAudioProcessor();
    ~WaveringAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor();

    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }

    void setCurrentProgram (int index) override
    {
        juce::ignoreUnused (index);
    }

    const juce::String getProgramName (int index) override
    {
        return index == 0 ? juce::String ("Default") : juce::String {};
    }

    void changeProgramName (
        int index,
        const juce::String& newName) override
    {
        juce::ignoreUnused (index, newName);
    }

    void getStateInformation (juce::MemoryBlock& destData) override;

    void setStateInformation (
        const void* data,
        int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    juce::AudioProcessorValueTreeState apvts {
        *this,
        nullptr,
        "Parameters",
        createParameterLayout()
    };

    // Copies the most recent audio samples for the GUI.
    // This function does not lock the audio thread.
    void copyVisualizationData (
        VisualizationData& destination) const noexcept;

private:
    DistortionDSP distortion;
    WaveguideDSP waveguide;

    double currentSampleRate = 44100.0;

    DistortionDSP::Parameters readDistortionParameters() const noexcept;

    WaveguideDSP::Parameters readWaveguideParameters() const noexcept;

    // Lock-free audio visualization history.
    std::array<std::atomic<float>, visualizationBufferSize>
        visualizationInput {};

    std::array<std::atomic<float>, visualizationBufferSize>
        visualizationOutput {};

    std::atomic<unsigned int> visualizationWritePosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        WaveringAudioProcessor)
};
