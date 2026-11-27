#include "DistortionDSP.h"

DistortionDSP::DistortionDSP() {
    toneStateLeft.reset();
    toneStateRight.reset();
}

void DistortionDSP::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    // Инициализируем фильтры в открытом состоянии (20 кГц)
    toneStateLeft.setCutoffFrequency((float)currentSampleRate, 20000.0f);
    toneStateRight.setCutoffFrequency((float)currentSampleRate, 20000.0f);
}

void DistortionDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numSamples == 0) return;

    // Маппинг параметров в полезные диапазоны
    float driveGain = juce::jmap(drive, 0.0f, 1.0f, 1.0f, 60.0f); // Усиление до 60x для мощного перегруза
    float toneFreq = juce::jmap(tone, 0.0f, 1.0f, 150.0f, 18000.0f); // От глубокого баса до ярких верхов
    float mixVal = juce::jlimit(0.0f, 1.0f, mix);
    float stereoWidth = juce::jlimit(0.0f, 1.0f, body);

    // Обновляем коэффициенты фильтра
    toneStateLeft.setCutoffFrequency((float)currentSampleRate, toneFreq);
    toneStateRight.setCutoffFrequency((float)currentSampleRate, toneFreq);

    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float drySample = channelData[sample];
            float wetSample = drySample;

            // 1. Применяем дисторшн (вейвшейпинг)
            wetSample = distortSample(wetSample, driveGain);

            // 2. Применяем фильтр тона
            if (channel == 0) {
                wetSample = toneStateLeft.process(wetSample);
            } else {
                wetSample = toneStateRight.process(wetSample);
            }

            // 3. Усиливаем 3D/Стерео эффект
            // Для правого канала добавляем легкую амплитудную модуляцию на основе параметра body
            if (stereoWidth > 0.0f && channel == 1) {
                wetSample *= (1.0f + (stereoWidth * 0.12f)); 
            }

            // 4. Микшируем Dry/Wet сигналы
            float finalSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);

            // 5. Безопасное ограничение и запись
            channelData[sample] = safeOutput(finalSample);
        }
    }
}

void DistortionDSP::reset() {
    toneStateLeft.reset();
    toneStateRight.reset();
}
