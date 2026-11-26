#include "RingModulatorDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;
}

void RingModulatorDSP::Smoother::reset (int rampSamples, float initial) noexcept
{
    rampLength = std::max (1, rampSamples);
    current = target = initial;
    increment = 0.0f;
    remaining = 0;
}

void RingModulatorDSP::Smoother::setTarget (float value) noexcept
{
    if (value == target)
        return;

    target = value;
    remaining = rampLength;
    increment = (target - current) / static_cast<float> (remaining);
}

float RingModulatorDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;
        if (--remaining == 0)
            current = target;
    }
    return current;
}

float RingModulatorDSP::finiteOrZero (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

float RingModulatorDSP::safeOutput (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

float RingModulatorDSP::polyBlep (float phaseValue, float phaseIncrement) noexcept
{
    if (phaseIncrement <= 0.0f)
        return 0.0f;

    if (phaseValue < phaseIncrement)
    {
        const float t = phaseValue / phaseIncrement;
        return t + t - t * t - 1.0f;
    }

    if (phaseValue > 1.0f - phaseIncrement)
    {
        const float t = (phaseValue - 1.0f) / phaseIncrement;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}

float RingModulatorDSP::oscillator (int waveform, float phaseValue, float phaseIncrement) noexcept
{
    switch (waveform)
    {
        case 1:
            return 1.0f - 4.0f * std::abs (phaseValue - 0.5f);
        case 2:
        {
            float value = phaseValue < 0.5f ? 1.0f : -1.0f;
            value += polyBlep (phaseValue, phaseIncrement);
            float shifted = phaseValue + 0.5f;
            if (shifted >= 1.0f)
                shifted -= 1.0f;
            value -= polyBlep (shifted, phaseIncrement);
            return value;
        }
        case 3:
            return 2.0f * phaseValue - 1.0f - polyBlep (phaseValue, phaseIncrement);
        case 0:
        default:
            return std::sin (2.0f * pi * phaseValue);
    }
}

void RingModulatorDSP::prepare (double newSampleRate, const Parameters& initial) noexcept
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1.0
                   ? std::clamp (newSampleRate, 8000.0, 384000.0) : 44100.0;
    const int rampSamples = std::max (1, static_cast<int> (std::lround (sampleRate * 0.015)));
    rateSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.rate), 0.0f, 10.0f));
    depthSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.depth), 0.0f, 10.0f));
    waveformSmoother.reset (rampSamples, static_cast<float> (std::clamp (initial.waveform, 0, 3)));
    fineSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.fine), 0.0f, 10.0f));
    mixSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.reset (rampSamples, initial.enabled ? 1.0f : 0.0f);
    reset();
}

void RingModulatorDSP::reset() noexcept
{
    phase = 0.0f;
}

void RingModulatorDSP::setParameters (const Parameters& parameters) noexcept
{
    rateSmoother.setTarget (std::clamp (finiteOrZero (parameters.rate), 0.0f, 10.0f));
    depthSmoother.setTarget (std::clamp (finiteOrZero (parameters.depth), 0.0f, 10.0f));
    waveformSmoother.setTarget (static_cast<float> (std::clamp (parameters.waveform, 0, 3)));
    fineSmoother.setTarget (std::clamp (finiteOrZero (parameters.fine), 0.0f, 10.0f));
    mixSmoother.setTarget (std::clamp (finiteOrZero (parameters.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.setTarget (parameters.enabled ? 1.0f : 0.0f);
}

void RingModulatorDSP::processFrame (float inputLeft, float inputRight, bool stereo,
                                     float& outputLeft, float& outputRight) noexcept
{
    const float rate = rateSmoother.next();
    const float depth = depthSmoother.next() * 0.1f;
    const float waveformPosition = std::clamp (waveformSmoother.next(), 0.0f, 3.0f);
    const float fine = fineSmoother.next();
    const float mix = mixSmoother.next();
    const float enabled = enableSmoother.next();

    if (enabled <= 0.0f)
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
        return;
    }

    // At these settings the effect is mathematically a bypass. Do not route the
    // signal through any arithmetic, so clean input remains exactly unchanged.
    if (mix <= 0.0f || depth <= 0.0f || rate <= 0.0f)
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
        return;
    }

    const float dryLeft = finiteOrZero (inputLeft);
    const float dryRight = stereo ? finiteOrZero (inputRight) : dryLeft;

    // RATE maps exponentially to 0.1..2000 Hz. FINE spans exactly -100..+100
    // cents across its 0..10 range (5 is neutral), without resetting phase.
    const float baseFrequency = 0.1f * std::pow (20000.0f, rate * 0.1f);
    const float cents = (fine - 5.0f) * 20.0f;
    const float tunedFrequency = baseFrequency * std::exp2 (cents / 1200.0f);
    const float maximumFrequency = static_cast<float> (sampleRate * 0.45);
    const float frequency = std::clamp (finiteOrZero (tunedFrequency), 0.0f, maximumFrequency);
    const float phaseIncrement = frequency / static_cast<float> (sampleRate);
    const float lowerIndexFloat = std::floor (waveformPosition);
    const int lowerIndex = static_cast<int> (lowerIndexFloat);
    const int upperIndex = std::min (3, lowerIndex + 1);
    const float waveformBlend = waveformPosition - lowerIndexFloat;
    const float lowerOsc = oscillator (lowerIndex, phase, phaseIncrement);
    const float upperOsc = oscillator (upperIndex, phase, phaseIncrement);
    const float carrier = lowerOsc + (upperOsc - lowerOsc) * waveformBlend;
    const float modulation = 1.0f - depth + depth * carrier;
    const float wetLeft = dryLeft * modulation;
    const float wetRight = dryRight * modulation;
    const float mixedLeft = dryLeft * (1.0f - mix) + wetLeft * mix;
    const float mixedRight = dryRight * (1.0f - mix) + wetRight * mix;
    outputLeft = safeOutput (dryLeft + (mixedLeft - dryLeft) * enabled);
    outputRight = stereo ? safeOutput (dryRight + (mixedRight - dryRight) * enabled) : 0.0f;

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
}
