/*
  ==============================================================================
    PluginProcessor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ... существующий код ...

void WaveringDistortionModAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // --- СУЩЕСТВУЮЩАЯ ЛОГИКА DSP ---
    // (Здесь вызывается ваш DistortionDSP::process или аналогичный код)
    // distortionDSP.process(buffer); 

    // --- РАСЧЕТ УРОВНЯ ДЛЯ VU (ПОСЛЕ ОБРАБОТКИ СИГНАЛА) ---
    float currentRMS = 0.0f;
    float currentPeak = 0.0f;

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        
        // Находим пик на канале
        float channelPeak = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float absVal = std::abs(channelData[i]);
            if (absVal > channelPeak)
                channelPeak = absVal;
        }
        
        if (channelPeak > currentPeak)
            currentPeak = channelPeak;

        // RMS для канала
        float channelRMS = buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
        // Усредняем RMS по всем каналам (простое приближение)
        currentRMS += channelRMS * channelRMS;
    }

    if (totalNumOutputChannels > 0)
        currentRMS = std::sqrt(currentRMS / totalNumOutputChannels);

    // Атомарная запись (безопасно для аудио-потока)
    rmsLevel.store(currentRMS);
    peakLevel.store(currentPeak);

    // ... остальной код ...
}