#include "DistortionDSP.h"

void DistortionDSP::Smoother::reset(int rampSamples, float initial) noexcept
{
    rampLength = std::max(1, rampSamples);
    current = target = initial;
    increment = 0.0f;
    remaining = 0;
}

void DistortionDSP::Smoother::setTarget(float value) noexcept
{
    if (!std::isfinite(value))
        return;

    if (value == target)
        return;

    target = value;
    remaining = rampLength;
    increment = (target - current) / static_cast<float>(remaining);
}

float DistortionDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;

        if (--remaining == 0)
            current = target;
    }

    return current;
}

float DistortionDSP::finiteOrZero(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float DistortionDSP::safeOutput(float value) noexcept
{
    if (!std::isfinite(value))
        return 0.0f;

    return std::clamp(value, -1.0f, 1.0f);
}

void DistortionDSP::prepare(double newSampleRate,
                            const Parameters& initial) noexcept
{
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 1000.0
        ? std::clamp(newSampleRate, 8000.0, 384000.0)
        : 44100.0;

    const int rampSamples = std::max(
        1, static_cast<int>(sampleRate * 0.015));

    driveSmoother.reset(rampSamples,
        std::clamp(finiteOrZero(initial.drive), 0.0f, 10.0f));

    toneSmoother.reset(rampSamples,
        std::clamp(finiteOrZero(initial.tone), 0.0f, 10.0f));

    biasSmoother.reset(rampSamples,
        std::clamp(finiteOrZero(initial.bias), 0.0f, 10.0f));

    bodySmoother.reset(rampSamples,
        std::clamp(finiteOrZero(initial.body), 0.0f, 10.0f));

    mixSmoother.reset(rampSamples,
        std::clamp(finiteOrZero(initial.mix), 0.0f, 10.0f));

    enableSmoother.reset(rampSamples,
        initial.enabled ? 1.0f : 0.0f);

    reset();
}

void DistortionDSP::reset() noexcept
{
    toneStateLeft = 0.0f;
    toneStateRight = 0.0f;
}

void DistortionDSP::setParameters(const Parameters& parameters) noexcept
{
    driveSmoother.setTarget(
        std::clamp(finiteOrZero(parameters.drive), 0.0f, 10.0f));

    toneSmoother.setTarget(
        std::clamp(finiteOrZero(parameters.tone), 0.0f, 10.0f));

    biasSmoother.setTarget(
        std::clamp(finiteOrZero(parameters.bias), 0.0f, 10.0f));

    bodySmoother.setTarget(
        std::clamp(finiteOrZero(parameters.body), 0.0f, 10.0f));

    mixSmoother.setTarget(
        std::clamp(finiteOrZero(parameters.mix), 0.0f, 10.0f));

    enableSmoother.setTarget(parameters.enabled ? 1.0f : 0.0f);
}

float DistortionDSP::processChannel(float input,
                                    float& toneState,
                                    float toneCoefficient,
                                    float drive,
                                    float bias,
                                    float body) noexcept
{
    const float driveGain = 1.0f + drive * 2.0f;
    const float biasOffset = (bias - 5.0f) * 0.08f;

    const float driven = finiteOrZero(input * driveGain + biasOffset);
    const float saturated = std::tanh(driven);

    const float cutoff = 500.0f *
        std::pow(36.0f, 1.0f - toneCoefficient);

    const float coefficient = 1.0f - std::exp(
        -2.0f * 3.14159265358979323846f *
        std::min(cutoff, static_cast<float>(sampleRate * 0.45)) /
        static_cast<float>(sampleRate));

    toneState += coefficient * (saturated - toneState);
    toneState = finiteOrZero(toneState);

    const float bodyGain = 1.0f + (body - 5.0f) * 0.025f;

    return finiteOrZero(toneState * bodyGain);
}

void DistortionDSP::processFrame(float inputLeft,
                                 float inputRight,
                                 bool stereo,
                                 float& outputLeft,
                                 float& outputRight) noexcept
{
    const float drive = driveSmoother.next();
    const float tone = toneSmoother.next();
    const float bias = biasSmoother.next();
    const float body = bodySmoother.next();
    const float mix = mixSmoother.next() * 0.1f;
    const float enabled = enableSmoother.next();

    const float dryLeft = finiteOrZero(inputLeft);
    const float dryRight = stereo ? finiteOrZero(inputRight) : dryLeft;

    const float wetLeft = processChannel(
        dryLeft, toneStateLeft, tone * 0.1f,
        drive, bias, body);

    const float wetRight = stereo
        ? processChannel(dryRight, toneStateRight, tone * 0.1f,
                         drive, bias, body)
        : wetLeft;

    const float mixedLeft = dryLeft + (wetLeft - dryLeft) * mix;
    const float mixedRight = dryRight + (wetRight - dryRight) * mix;

    outputLeft = safeOutput(
        dryLeft + (mixedLeft - dryLeft) * enabled);

    outputRight = stereo
        ? safeOutput(dryRight + (mixedRight - dryRight) * enabled)
        : 0.0f;
}