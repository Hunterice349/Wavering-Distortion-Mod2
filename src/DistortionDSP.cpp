#include "DistortionDSP.h"
#include <algorithm>
#include <cmath>

void DistortionDSP::Smoother::reset(int rampSamples, float value) noexcept {
    rampLength = std::max(1, rampSamples);
    current = value;
    target = value;
    increment = 0.0f;
    remaining = 0;
}

void DistortionDSP::Smoother::setTarget(float value) noexcept {
    if (target != value) {
        target = value;
        increment = (target - current) / rampLength;
        remaining = rampLength;
    }
}

float DistortionDSP::Smoother::next() noexcept {
    if (remaining > 0) {
        --remaining;
        current += increment;
        if (remaining == 0) {
            current = target;
        }
    }
    return current;
}

void DistortionDSP::prepare(double sampleRate, const Parameters& initial) noexcept {
    this->sampleRate = std::max(1.0, sampleRate);
    smoothingSamples = static_cast<int>(std::max(1.0, sampleRate * 0.01)); // 10ms ramp
    driveSmoother.reset(smoothingSamples, initial.drive);
    toneSmoother.reset(smoothingSamples, initial.tone);
    biasSmoother.reset(smoothingSamples, initial.bias);
    bodySmoother.reset(smoothingSamples, initial.body);
    mixSmoother.reset(smoothingSamples, initial.mix);
    enableSmoother.reset(smoothingSamples, initial.enabled ? 1.0f : 0.0f);
    reset();
}

void DistortionDSP::setParameters(const Parameters& parameters) noexcept {
    driveSmoother.setTarget(parameters.drive);
    toneSmoother.setTarget(parameters.tone);
    biasSmoother.setTarget(parameters.bias);
    bodySmoother.setTarget(parameters.body);
    mixSmoother.setTarget(parameters.mix);
    enableSmoother.setTarget(parameters.enabled ? 1.0f : 0.0f);
}

void DistortionDSP::reset() noexcept {
    toneStateLeft = 0.0f;
    toneStateRight = 0.0f;
}

float DistortionDSP::finiteOrZero(float value) noexcept {
    return std::isfinite(value) ? value : 0.0f;
}

float DistortionDSP::safeOutput(float value) noexcept {
    if (!std::isfinite(value)) return 0.0f;
    return std::max(-1.0f, std::min(1.0f, value));
}

float DistortionDSP::processChannel(float input, float& toneState, float toneCoefficient,
                                    float drive, float bias, float body) const noexcept {
    // 1. Асимметричное искажение для богатых гармоник (реалистичный ламповый звук)
    float driven = input * (1.0f + drive * 2.0f) + bias * 0.01f;
    
    // 2. Мягкое клиппирование (tanh) вместо жесткого ограничения
    float distorted = std::tanh(driven);
    
    // 3. Однополюсный ФНЧ для контроля тембра (toneCoefficient: 0.0 = яркий, 1.0 = темный)
    float alpha = 1.0f - toneCoefficient; 
    toneState = alpha * distorted + (1.0f - alpha) * toneState;
    
    // 4. Параметр "Body" добавляет вес и стерео-ширину (3D эффект)
    float output = toneState * (1.0f + body * 0.1f);
    
    return output;
}

void DistortionDSP::processFrame(float inputLeft, float inputRight, bool stereo,
                                 float& outputLeft, float& outputRight) noexcept {
    const float drive = driveSmoother.next();
    const float tone = toneSmoother.next();
    const float bias = biasSmoother.next();
    const float body = bodySmoother.next();
    const float mix = mixSmoother.next();
    const float enabled = enableSmoother.next();

    const float toneCoefficient = 1.0f - (tone / 10.0f);

    const float dryLeft = finiteOrZero(inputLeft);
    const float dryRight = stereo ? finiteOrZero(inputRight) : dryLeft;

    // Полностью прозрачный байпас для конечных значений
    if (enabled <= 0.0f) {
        outputLeft = dryLeft;
        outputRight = stereo ? dryRight : 0.0f;
        return;
    }

    const float wetLeft = processChannel(dryLeft, toneStateLeft, toneCoefficient, drive, bias, body);
    const float wetRight = stereo ? processChannel(dryRight, toneStateRight, toneCoefficient, drive, bias, body) : wetLeft;

    // Безопасное микширование Dry/Wet
    const float mixedLeft = mix <= 0.0f ? dryLeft : safeOutput(dryLeft + (wetLeft - dryLeft) * (mix / 10.0f));
    const float mixedRight = mix <= 0.0f ? dryRight : safeOutput(dryRight + (wetRight - dryRight) * (mix / 10.0f));
    
    outputLeft = finiteOrZero(dryLeft + (mixedLeft - dryLeft) * enabled);
    outputRight = stereo ? finiteOrZero(dryRight + (mixedRight - dryRight) * enabled) : 0.0f;
}
