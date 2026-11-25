#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class WaveringAudioProcessor : public juce::AudioProcessor
{
public:
    WaveringAudioProcessor();
    ~WaveringAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters",
                                               createParameterLayout() };

    // ---- связь DSP -> GUI (атомары, читаются из потока GUI) ----
    std::atomic<float> outputLevelDb { -80.0f };
    std::atomic<float> ringFreqHz    { 0.0f };

private:
    double sr = 44100.0;
    int maxDelaySamples = 0;

    // Waveguide Resonator (delay line + one-pole loss)
    std::vector<float> waveBufL, waveBufR;
    int waveWrite = 0, waveDelayLen = 100;
    float waveZ1L = 0.0f, waveZ1R = 0.0f;

    // Tone LP
    float toneZ1L = 0.0f, toneZ1R = 0.0f;

    // Ring Modulator
    double ringPhase = 0.0;

    // Сглаженные параметры
    float smDrive = 0.0f, smTone = 8000.0f, smWave = 220.0f, smRes = 0.0f;
    float smRing = 0.0f, smDepth = 0.5f, smMix = 0.5f, smOut = 0.0f;
    bool smBypass = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringAudioProcessor)
};
