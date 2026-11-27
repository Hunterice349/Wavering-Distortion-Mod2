#pragma once

#include <JuceHeader.h>
#include "DistortionDSP.h" // <-- ВАЖНО: Замена старого заголовка

class WaveringDistortionModAudioProcessor : public juce::AudioProcessor {
public:
    WaveringDistortionModAudioProcessor();
    ~WaveringDistortionModAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Объявление параметров
    juce::AudioParameterFloat* driveParam = nullptr;
    juce::AudioParameterFloat* toneParam = nullptr;
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterFloat* bodyParam = nullptr;

private:
    DistortionDSP distortionDSP; // Экземпляр нашего DSP
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveringDistortionModAudioProcessor)
};
