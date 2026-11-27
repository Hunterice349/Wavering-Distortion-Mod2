#include <JuceHeader.h>
#include "../src/DistortionDSP.h"
#include <fstream> // Важно для ofstream

// Тест 1: Проверка экстремальных значений
void testParameterExtremes() {
    DistortionDSP dsp;
    dsp.prepareToPlay(44100.0, 512);
    
    dsp.drive = 1.0f;
    dsp.tone = 0.0f;
    dsp.mix = 1.0f;
    dsp.body = 1.0f;
    
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    
    for (int i = 0; i < 512; ++i) {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.5f);
    }
    
    dsp.processBlock(buffer, midi);
    
    for (int ch = 0; ch < 2; ++ch) {
        for (int s = 0; s < 512; ++s) {
            jassert(!std::isnan(buffer.getSample(ch, s)));
            jassert(!std::isinf(buffer.getSample(ch, s)));
        }
    }
}

// Если есть другие тесты, убедитесь, что они имеют такую же структуру:
// void testName() { ... }
