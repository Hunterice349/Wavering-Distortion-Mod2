#include "DistortionDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;
}

void DistortionDSP::Smoother::reset (int rampSamples, float value) noexcept
{
    rampLength = std::max (1, rampSamples);
    current = target = value;
    increment = 0.0f;
    remaining = 0;
}

void DistortionDSP::Smoother::setTarget (float value) noexcept
{
    if (value == target)
        return;

    target = value;
    remaining = rampLength;
    increment = (target - current) / static_cast<float> (remaining);
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

void DistortionDSP::prepare (double newSampleRate, const Parameters& initial) noexcept
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1.0 ? newSampleRate : 44100.0;
    smoothingSamples = std::max (1, static_cast<int> (std::lround (sampleRate * 0.012)));

    driveSmoother.reset (smoothingSamples, std::clamp (finiteOrZero (initial.drive), 0.0f, 10.0f));
    toneSmoother.reset (smoothingSamples, std::clamp (finiteOrZero (initial.tone), 0.0f, 10.0f));
    biasSmoother.reset (smoothingSamples, std::clamp (finiteOrZero (initial.bias), 0.0f, 10.0f));
    bodySmoother.reset (smoothingSamples, std::clamp (finiteOrZero (initial.body), 0.0f, 10.0f));
    mixSmoother.reset (smoothingSamples, std::clamp (finiteOrZero (initial.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.reset (smoothingSamples, initial.enabled ? 1.0f : 0.0f);
    reset();
}

void DistortionDSP::setParameters (const Parameters& parameters) noexcept
{
    driveSmoother.setTarget (std::clamp (finiteOrZero (parameters.drive), 0.0f, 10.0f));
    toneSmoother.setTarget (std::clamp (finiteOrZero (parameters.tone), 0.0f, 10.0f));
    biasSmoother.setTarget (std::clamp (finiteOrZero (parameters.bias), 0.0f, 10.0f));
    bodySmoother.setTarget (std::clamp (finiteOrZero (parameters.body), 0.0f, 10.0f));
    mixSmoother.setTarget (std::clamp (finiteOrZero (parameters.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.setTarget (parameters.enabled ? 1.0f : 0.0f);
}

void DistortionDSP::reset() noexcept
{
    toneStateLeft = toneStateRight = 0.0f;
}

float DistortionDSP::finiteOrZero (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

float DistortionDSP::safeOutput (float value) noexcept
{
    if (! std::isfinite (value))
        return 0.0f;

    const float magnitude = std::abs (value);
    if (magnitude <= 0.98f)
        return value;

    const float limited = 0.98f + 0.02f * std::tanh ((magnitude - 0.98f) / 0.02f);
    return std::copysign (limited, value);
}

float DistortionDSP::processChannel (float input, float& toneState, float toneCoefficient,
                                     float drive, float bias, float body) const noexcept
{
    const float boundedInput = std::clamp (finiteOrZero (input), -8.0f, 8.0f);
    float shaped = boundedInput;

    // At zero drive the waveshaper is deliberately bypassed. Tone remains an
    // independent post-shaper control, while no nonlinear distortion is added.
    if (drive > 1.0e-5f)
    {
        const float driveAmount = drive * 0.1f;
        const float gain = 1.0f + 24.0f * std::pow (driveAmount, 1.6f);
        const float asymmetry = (bias - 5.0f) * 0.12f;
        const float bodyBlend = body * 0.1f;

        const auto curve = [bodyBlend] (float x) noexcept
        {
            const float smooth = std::tanh (x);
            const float dense = x / (1.0f + std::abs (x));
            return smooth + bodyBlend * (dense - smooth);
        };

        // Subtract the curve's zero-input value to avoid bias-induced DC.
        const float biasOffset = curve (asymmetry);
        const float positive = curve (gain + asymmetry) - biasOffset;
        const float negative = biasOffset - curve (-gain + asymmetry);
        const float normaliser = std::max (0.25f, 0.5f * (std::abs (positive) + std::abs (negative)));
        shaped = (curve (boundedInput * gain + asymmetry) - biasOffset) / normaliser;
    }

    toneState += toneCoefficient * (shaped - toneState);
    if (! std::isfinite (toneState))
        toneState = 0.0f;

    return safeOutput (toneState);
}

void DistortionDSP::processFrame (float inputLeft, float inputRight, bool stereo,
                                  float& outputLeft, float& outputRight) noexcept
{
    const float drive = driveSmoother.next();
    const float tone = toneSmoother.next();
    const float bias = biasSmoother.next();
    const float body = bodySmoother.next();
    const float mix = mixSmoother.next();
    const float enabled = enableSmoother.next();

    const float toneNormalised = tone * 0.1f;
    const float cutoff = 350.0f * std::pow (18000.0f / 350.0f, toneNormalised);
    const float safeCutoff = std::min (cutoff, static_cast<float> (sampleRate * 0.45));
    const float toneCoefficient = 1.0f - std::exp (-2.0f * pi * safeCutoff / static_cast<float> (sampleRate));

    const float dryLeft = finiteOrZero (inputLeft);
    const float dryRight = stereo ? finiteOrZero (inputRight) : dryLeft;

    // A fully bypassed switch is bit-transparent, including for samples outside
    // the plugin's normal operating range. The transition is ramped before here.
    if (enabled <= 0.0f)
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
        return;
    }

    const float wetLeft = processChannel (dryLeft, toneStateLeft, toneCoefficient, drive, bias, body);
    const float wetRight = stereo ? processChannel (dryRight, toneStateRight, toneCoefficient, drive, bias, body)
                                  : wetLeft;

    // Preserve a true dry path at MIX=0 instead of passing it through the safety
    // saturator. The limiter only catches peaks on the processed blend.
    const float mixedLeft = mix <= 0.0f ? dryLeft : safeOutput (dryLeft + (wetLeft - dryLeft) * mix);
    const float mixedRight = mix <= 0.0f ? dryRight : safeOutput (dryRight + (wetRight - dryRight) * mix);
    outputLeft = dryLeft + (mixedLeft - dryLeft) * enabled;
    outputRight = stereo ? dryRight + (mixedRight - dryRight) * enabled : 0.0f;
}