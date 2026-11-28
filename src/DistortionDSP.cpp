
#include "DistortionDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;

constexpr float maximumWet = 1.0f;
constexpr float maximumInput = 16.0f;
constexpr float minimumSampleRate = 8000.0f;
constexpr float maximumSampleRate = 384000.0f;
constexpr double smoothingTimeSeconds = 0.015;
}

void DistortionDSP::Smoother::reset (
    int rampSamples,
    float initial) noexcept
{
    rampLength = std::max (1, rampSamples);

    current = initial;
    target = initial;

    increment = 0.0f;
    remaining = 0;
}

void DistortionDSP::Smoother::setTarget (float value) noexcept
{
    if (! std::isfinite (value) || value == target)
        return;

    target = value;
    remaining = rampLength;

    increment = (target - current)
              / static_cast<float> (remaining);
}

float DistortionDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;

        --remaining;

        if (remaining == 0)
            current = target;
    }

    return current;
}

float DistortionDSP::finiteOrZero (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

float DistortionDSP::clampParameter (
    float value,
    float low,
    float high) noexcept
{
    return std::clamp (
        finiteOrZero (value),
        low,
        high);
}

int DistortionDSP::validModeIndex (Mode mode) noexcept
{
    const int value = static_cast<int> (mode);

    return value >= 0 && value <= 4 ? value : 0;
}

void DistortionDSP::prepare (
    double newSampleRate,
    const Parameters& initial) noexcept
{
    if (! std::isfinite (newSampleRate)
        || newSampleRate < minimumSampleRate)
    {
        sampleRate = 44100.0;
    }
    else
    {
        sampleRate = std::clamp (
            newSampleRate,
            static_cast<double> (minimumSampleRate),
            static_cast<double> (maximumSampleRate));
    }

    const int rampSamples = std::max (
        1,
        static_cast<int> (
            sampleRate * smoothingTimeSeconds));

    driveSmoother.reset (
        rampSamples,
        clampParameter (initial.drive, 0.0f, 10.0f));

    toneSmoother.reset (
        rampSamples,
        clampParameter (initial.tone, 0.0f, 10.0f));

    biasSmoother.reset (
        rampSamples,
        clampParameter (initial.bias, 0.0f, 10.0f));

    bodySmoother.reset (
        rampSamples,
        clampParameter (initial.body, 0.0f, 10.0f));

    mixSmoother.reset (
        rampSamples,
        clampParameter (initial.mix, 0.0f, 10.0f));

    modeSmoother.reset (
        rampSamples,
        static_cast<float> (validModeIndex (initial.mode)));

    enableSmoother.reset (
        rampSamples,
        initial.enabled ? 1.0f : 0.0f);

    reset();
}

void DistortionDSP::reset() noexcept
{
    toneStateLeft = 0.0f;
    toneStateRight = 0.0f;
}

void DistortionDSP::setParameters (
    const Parameters& parameters) noexcept
{
    driveSmoother.setTarget (
        clampParameter (parameters.drive, 0.0f, 10.0f));

    toneSmoother.setTarget (
        clampParameter (parameters.tone, 0.0f, 10.0f));

    biasSmoother.setTarget (
        clampParameter (parameters.bias, 0.0f, 10.0f));

    bodySmoother.setTarget (
        clampParameter (parameters.body, 0.0f, 10.0f));

    mixSmoother.setTarget (
        clampParameter (parameters.mix, 0.0f, 10.0f));

    modeSmoother.setTarget (
        static_cast<float> (validModeIndex (parameters.mode)));

    enableSmoother.setTarget (
        parameters.enabled ? 1.0f : 0.0f);
}

float DistortionDSP::processWaveshaper (
    float input,
    Mode mode) noexcept
{
    switch (mode)
    {
        case Mode::drive:
        {
            return std::tanh (input);
        }

        case Mode::overdrive:
        {
            const float asymmetric =
                input >= 0.0f
                    ? input * 1.08f
                    : input * 0.92f;

            return std::tanh (asymmetric * 1.45f)
                 / std::tanh (1.45f);
        }

        case Mode::distortion:
        {
            return (2.0f / pi)
                 * std::atan (input * 3.2f);
        }

        case Mode::fuzz:
        {
            const float boosted = input * 4.5f;

            const float shaped = std::tanh (boosted);

            const float folded = std::sin (
                std::clamp (
                    boosted,
                    -pi * 0.5f,
                    pi * 0.5f));

            return std::clamp (
                0.78f * shaped + 0.22f * folded,
                -1.0f,
                1.0f);
        }

        case Mode::hardClip:
        {
            return std::clamp (
                input * 2.2f,
                -1.0f,
                1.0f);
        }
    }

    return std::tanh (input);
}

float DistortionDSP::processChannel (
    float input,
    float& toneState,
    float toneCoefficient,
    float drive,
    float bias,
    float body,
    float modePosition) noexcept
{
    // Protect the DSP from extreme finite input values.
    const float boundedInput = std::clamp (
        finiteOrZero (input),
        -maximumInput,
        maximumInput);

    const float driveGain = 1.0f + drive * 2.0f;

    const float biasOffset = (bias - 5.0f) * 0.08f;

    const float driven = finiteOrZero (
        boundedInput * driveGain + biasOffset);

    // Smoothly interpolate adjacent distortion modes.
    const float boundedMode = std::clamp (
        modePosition,
        0.0f,
        4.0f);

    const int lowerIndex = static_cast<int> (boundedMode);

    const int upperIndex = std::min (
        lowerIndex + 1,
        4);

    const float modeFraction =
        boundedMode - static_cast<float> (lowerIndex);

    const float lower = processWaveshaper (
        driven,
        static_cast<Mode> (lowerIndex));

    const float upper = processWaveshaper (
        driven,
        static_cast<Mode> (upperIndex));

    const float saturated =
        lower + (upper - lower) * modeFraction;

    // Tone: higher values produce a higher low-pass cutoff.
    const float safeTone = std::clamp (
        toneCoefficient,
        0.0f,
        1.0f);

    const float cutoff =
        500.0f * std::pow (36.0f, safeTone);

    const float safeCutoff = std::min (
        cutoff,
        static_cast<float> (sampleRate * 0.45));

    const float coefficient = 1.0f - std::exp (
        -2.0f * pi * safeCutoff
        / static_cast<float> (sampleRate));

    toneState += coefficient * (saturated - toneState);

    toneState = finiteOrZero (toneState);

    const float bodyGain =
        1.0f + (body - 5.0f) * 0.025f;

    return finiteOrZero (toneState * bodyGain);
}

void DistortionDSP::processFrame (
    float inputLeft,
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

    const float modePosition = modeSmoother.next();

    const float enabled = enableSmoother.next();

    const float dryLeft = finiteOrZero (inputLeft);

    const float dryRight = stereo
        ? finiteOrZero (inputRight)
        : dryLeft;

    // Exact bypass: preserve finite dry signal and headroom.
    if (mix <= 0.0f || enabled <= 0.0f)
    {
        outputLeft = dryLeft;

        outputRight = stereo
            ? dryRight
            : 0.0f;

        return;
    }

    const float wetLeft = std::clamp (
        processChannel (
            dryLeft,
            toneStateLeft,
            tone * 0.1f,
            drive,
            bias,
            body,
            modePosition),
        -maximumWet,
        maximumWet);

    const float wetRight = stereo
        ? std::clamp (
            processChannel (
                dryRight,
                toneStateRight,
                tone * 0.1f,
                drive,
                bias,
                body,
                modePosition),
            -maximumWet,
            maximumWet)
        : wetLeft;

    // Classical linear Dry/Wet crossfade.
    const float wetMix = std::clamp (
        mix * enabled,
        0.0f,
        1.0f);

    outputLeft = finiteOrZero (
        dryLeft + (wetLeft - dryLeft) * wetMix);

    outputRight = stereo
        ? finiteOrZero (
            dryRight + (wetRight - dryRight) * wetMix)
        : 0.0f;
}
