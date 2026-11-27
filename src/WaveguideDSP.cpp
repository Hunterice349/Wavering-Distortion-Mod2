
#include "WaveguideDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float maximumFeedback = 0.94f;
constexpr float maximumDelaySeconds = 0.125f;
constexpr float maximumSignal = 8.0f;

float safeParameter (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}
}

//==============================================================================
// Smoother
//==============================================================================

void WaveguideDSP::Smoother::reset (int rampSamples, float initial) noexcept
{
    rampLength = std::max (1, rampSamples);
    current = safeParameter (initial);
    target = current;
    increment = 0.0f;
    remaining = 0;
}

void WaveguideDSP::Smoother::setTarget (float value) noexcept
{
    value = safeParameter (value);

    if (value == target)
        return;

    target = value;
    remaining = rampLength;
    increment = (target - current) / static_cast<float> (remaining);
}

float WaveguideDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;

        if (--remaining == 0)
            current = target;
    }

    return safeParameter (current);
}

//==============================================================================
// Utilities
//==============================================================================

float WaveguideDSP::finiteOrZero (float value) noexcept
{
    return safeParameter (value);
}

float WaveguideDSP::readDelay (
    const std::vector<float>& buffer,
    float delaySamples) const noexcept
{
    // At least three samples are required for safe interpolation.
    if (buffer.size() < 3)
        return 0.0f;

    if (! std::isfinite (delaySamples))
        delaySamples = 1.0f;

    const float maxDelay =
        static_cast<float> (buffer.size() - 2u);

    const float boundedDelay =
        std::clamp (delaySamples, 1.0f, maxDelay);

    float readPosition =
        static_cast<float> (writeIndex) - boundedDelay;

    const float bufferSize =
        static_cast<float> (buffer.size());

    while (readPosition < 0.0f)
        readPosition += bufferSize;

    while (readPosition >= bufferSize)
        readPosition -= bufferSize;

    const int index0 =
        static_cast<int> (readPosition);

    const int index1 =
        (index0 + 1) % static_cast<int> (buffer.size());

    const float fraction =
        readPosition - static_cast<float> (index0);

    const float sample0 =
        buffer[static_cast<size_t> (index0)];

    const float sample1 =
        buffer[static_cast<size_t> (index1)];

    const float result =
        sample0 + (sample1 - sample0) * fraction;

    return finiteOrZero (result);
}

//==============================================================================
// Channel Processing
//==============================================================================

float WaveguideDSP::processChannel (
    float input,
    std::vector<float>& buffer,
    float& dampingState,
    float delaySamples,
    float feedbackGain,
    float dampingCoefficient,
    float character) noexcept
{
    if (buffer.size() < 3)
        return finiteOrZero (input);

    input = finiteOrZero (input);
    dampingState = finiteOrZero (dampingState);

    const float delayed =
        readDelay (buffer, delaySamples);

    dampingState += dampingCoefficient *
                    (delayed - dampingState);

    dampingState = finiteOrZero (dampingState);

    // Character blends linear feedback with normalized saturation.
    const float feedbackInput =
        std::clamp (dampingState, -4.0f, 4.0f);

    const float saturatedFeedback =
        std::tanh (feedbackInput * 2.0f) * 0.5f;

    const float loopSignal =
        dampingState +
        (saturatedFeedback - dampingState) * character;

    float writeValue =
        input + loopSignal * feedbackGain;

    writeValue = std::clamp (
        finiteOrZero (writeValue),
        -maximumSignal,
        maximumSignal);

    buffer[static_cast<size_t> (writeIndex)] =
        writeValue;

    return finiteOrZero (delayed);
}

//==============================================================================
// Prepare
//==============================================================================

void WaveguideDSP::prepare (
    double newSampleRate,
    const Parameters& initial)
{
    sampleRate =
        std::isfinite (newSampleRate) && newSampleRate > 1000.0
        ? std::clamp (newSampleRate, 8000.0, 384000.0)
        : 44100.0;

    const auto capacity =
        static_cast<size_t> (
            std::ceil (sampleRate * maximumDelaySeconds)) + 4u;

    delayLeft.assign (capacity, 0.0f);
    delayRight.assign (capacity, 0.0f);

    writeIndex = 0;

    const int rampSamples =
        std::max (
            1,
            static_cast<int> (
                std::lround (sampleRate * 0.015)));

    rateSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.rate), 0.0f, 10.0f));

    resonanceSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.resonance), 0.0f, 10.0f));

    feedbackSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.feedback), 0.0f, 10.0f));

    dampingSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.damping), 0.0f, 10.0f));

    characterSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.character), 0.0f, 10.0f));

    mixSmoother.reset (
        rampSamples,
        std::clamp (finiteOrZero (initial.mix), 0.0f, 10.0f) * 0.1f);

    enableSmoother.reset (
        rampSamples,
        initial.enabled ? 1.0f : 0.0f);

    reset();
}

//==============================================================================
// Reset
//==============================================================================

void WaveguideDSP::reset() noexcept
{
    std::fill (
        delayLeft.begin(),
        delayLeft.end(),
        0.0f);

    std::fill (
        delayRight.begin(),
        delayRight.end(),
        0.0f);

    writeIndex = 0;

    dampingLeft = 0.0f;
    dampingRight = 0.0f;
}

//==============================================================================
// Parameter Updates
//==============================================================================

void WaveguideDSP::setParameters (
    const Parameters& parameters) noexcept
{
    rateSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.rate), 0.0f, 10.0f));

    resonanceSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.resonance), 0.0f, 10.0f));

    feedbackSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.feedback), 0.0f, 10.0f));

    dampingSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.damping), 0.0f, 10.0f));

    characterSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.character), 0.0f, 10.0f));

    mixSmoother.setTarget (
        std::clamp (finiteOrZero (parameters.mix), 0.0f, 10.0f) * 0.1f);

    enableSmoother.setTarget (
        parameters.enabled ? 1.0f : 0.0f);
}

//==============================================================================
// Audio Processing
//==============================================================================

void WaveguideDSP::processFrame (
    float inputLeft,
    float inputRight,
    bool stereo,
    float& outputLeft,
    float& outputRight) noexcept
{
    const float safeLeft = finiteOrZero (inputLeft);
    const float safeRight = stereo
                          ? finiteOrZero (inputRight)
                          : safeLeft;

    // Safe fallback if storage is unavailable or invalid.
    if (delayLeft.size() < 3 || delayRight.size() < 3)
    {
        outputLeft = safeLeft;
        outputRight = stereo ? safeRight : 0.0f;
        return;
    }

    const float rate =
        rateSmoother.next() * 0.1f;

    const float resonance =
        resonanceSmoother.next() * 0.1f;

    const float feedback =
        feedbackSmoother.next() * 0.1f;

    const float damping =
        dampingSmoother.next() * 0.1f;

    const float character =
        characterSmoother.next() * 0.1f;

    const float mix =
        std::clamp (mixSmoother.next(), 0.0f, 1.0f);

    const float enabled =
        std::clamp (enableSmoother.next(), 0.0f, 1.0f);

    // RATE maps exponentially to approximately 2..120 ms.
    const float delaySeconds =
        0.002f * std::pow (60.0f, rate);

    const float maximumSafeDelay =
        static_cast<float> (delayLeft.size() - 2u);

    const float delaySamples =
        std::clamp (
            delaySeconds * static_cast<float> (sampleRate),
            1.0f,
            maximumSafeDelay);

    const float feedbackGain =
        maximumFeedback * feedback *
        (0.12f + 0.88f * resonance);

    const float dampingCutoff =
        550.0f * std::pow (
            18000.0f / 550.0f,
            damping);

    const float safeCutoff =
        std::min (
            dampingCutoff,
            static_cast<float> (sampleRate * 0.45));

    const float dampingCoefficient =
        1.0f - std::exp (
            -2.0f * pi * safeCutoff /
            static_cast<float> (sampleRate));

    // Process left channel.
    const float wetLeft =
        processChannel (
            safeLeft,
            delayLeft,
            dampingLeft,
            delaySamples,
            feedbackGain,
            dampingCoefficient,
            character);

    // Process right channel only when stereo is active.
    const float wetRight =
        stereo
        ? processChannel (
            safeRight,
            delayRight,
            dampingRight,
            delaySamples,
            feedbackGain,
            dampingCoefficient,
            character)
        : wetLeft;

    // Crossfade between dry and processed signals.
    const float mixedLeft =
        safeLeft + (wetLeft - safeLeft) * mix;

    const float mixedRight =
        safeRight + (wetRight - safeRight) * mix;

    outputLeft =
        finiteOrZero (
            safeLeft + (mixedLeft - safeLeft) * enabled);

    outputRight =
        stereo
        ? finiteOrZero (
            safeRight + (mixedRight - safeRight) * enabled)
        : 0.0f;

    // Advance circular delay-line position.
    ++writeIndex;

    if (writeIndex >= static_cast<int> (delayLeft.size()))
        writeIndex = 0;
}
