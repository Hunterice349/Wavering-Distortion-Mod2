#include "WaveguideDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float maximumFeedback = 0.94f;
constexpr float maximumDelaySeconds = 0.125f;
}

void WaveguideDSP::Smoother::reset (int rampSamples, float initial) noexcept
{
    rampLength = std::max (1, rampSamples);
    current = target = initial;
    increment = 0.0f;
    remaining = 0;
}

void WaveguideDSP::Smoother::setTarget (float value) noexcept
{
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
    return current;
}

float WaveguideDSP::finiteOrZero (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

void WaveguideDSP::prepare (double newSampleRate, const Parameters& initial)
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1000.0
                   ? std::clamp (newSampleRate, 8000.0, 384000.0) : 44100.0;
    const auto capacity = static_cast<size_t> (std::ceil (sampleRate * maximumDelaySeconds)) + 4u;
    delayLeft.assign (capacity, 0.0f);
    delayRight.assign (capacity, 0.0f);
    writeIndex = 0;

    const int rampSamples = std::max (1, static_cast<int> (std::lround (sampleRate * 0.015)));
    rateSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.rate), 0.0f, 10.0f));
    resonanceSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.resonance), 0.0f, 10.0f));
    feedbackSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.feedback), 0.0f, 10.0f));
    dampingSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.damping), 0.0f, 10.0f));
    characterSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.character), 0.0f, 10.0f));
    mixSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.reset (rampSamples, initial.enabled ? 1.0f : 0.0f);
    reset();
}

void WaveguideDSP::reset() noexcept
{
    std::fill (delayLeft.begin(), delayLeft.end(), 0.0f);
    std::fill (delayRight.begin(), delayRight.end(), 0.0f);
    writeIndex = 0;
    dampingLeft = dampingRight = 0.0f;
}

void WaveguideDSP::setParameters (const Parameters& parameters) noexcept
{
    rateSmoother.setTarget (std::clamp (finiteOrZero (parameters.rate), 0.0f, 10.0f));
    resonanceSmoother.setTarget (std::clamp (finiteOrZero (parameters.resonance), 0.0f, 10.0f));
    feedbackSmoother.setTarget (std::clamp (finiteOrZero (parameters.feedback), 0.0f, 10.0f));
    dampingSmoother.setTarget (std::clamp (finiteOrZero (parameters.damping), 0.0f, 10.0f));
    characterSmoother.setTarget (std::clamp (finiteOrZero (parameters.character), 0.0f, 10.0f));
    mixSmoother.setTarget (std::clamp (finiteOrZero (parameters.mix), 0.0f, 10.0f) * 0.1f);
    enableSmoother.setTarget (parameters.enabled ? 1.0f : 0.0f);
}

float WaveguideDSP::readDelay (const std::vector<float>& buffer, float delaySamples) const noexcept
{
    if (buffer.empty())
        return 0.0f;

    const float maxDelay = static_cast<float> (buffer.size() - 2u);
    const float boundedDelay = std::clamp (delaySamples, 1.0f, maxDelay);
    float readPosition = static_cast<float> (writeIndex) - boundedDelay;
    if (readPosition < 0.0f)
        readPosition += static_cast<float> (buffer.size());

    const int index0 = static_cast<int> (readPosition);
    const int index1 = (index0 + 1) % static_cast<int> (buffer.size());
    const float fraction = readPosition - static_cast<float> (index0);
    return buffer[static_cast<size_t> (index0)]
           + (buffer[static_cast<size_t> (index1)] - buffer[static_cast<size_t> (index0)]) * fraction;
}

float WaveguideDSP::processChannel (float input, std::vector<float>& buffer, float& dampingState,
                                    float delaySamples, float feedbackGain, float dampingCoefficient,
                                    float character) noexcept
{
    if (buffer.empty())
        return input;

    const float delayed = readDelay (buffer, delaySamples);
    dampingState += dampingCoefficient * (delayed - dampingState);
    if (! std::isfinite (dampingState))
        dampingState = 0.0f;

    // Character blends linear feedback with normalized soft saturation. It
    // changes loop harmonics without increasing small-signal loop gain.
    const float feedbackInput = std::clamp (dampingState, -4.0f, 4.0f);
    const float saturatedFeedback = std::tanh (feedbackInput * 2.0f) * 0.5f;
    const float loopSignal = dampingState + (saturatedFeedback - dampingState) * character;
    float writeValue = finiteOrZero (input) + loopSignal * feedbackGain;
    writeValue = std::clamp (finiteOrZero (writeValue), -8.0f, 8.0f);
    buffer[static_cast<size_t> (writeIndex)] = writeValue;
    return finiteOrZero (delayed);
}

void WaveguideDSP::processFrame (float inputLeft, float inputRight, bool stereo,
                                 float& outputLeft, float& outputRight) noexcept
{
    if (delayLeft.empty() || delayRight.empty())
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
        return;
    }

    const float rate = rateSmoother.next() * 0.1f;
    const float resonance = resonanceSmoother.next() * 0.1f;
    const float feedback = feedbackSmoother.next() * 0.1f;
    const float damping = dampingSmoother.next() * 0.1f;
    const float character = characterSmoother.next() * 0.1f;
    const float mix = mixSmoother.next();
    const float enabled = enableSmoother.next();

    // RATE maps exponentially to about 2..120 ms; its smoothing keeps delay
    // motion continuous while the fixed-size delay allocation remains bounded.
    const float delaySeconds = 0.002f * std::pow (60.0f, rate);
    const float delaySamples = std::clamp (delaySeconds * static_cast<float> (sampleRate), 1.0f,
                                            static_cast<float> (delayLeft.size() - 2u));
    const float feedbackGain = maximumFeedback * feedback * (0.12f + 0.88f * resonance);
    const float dampingCutoff = 550.0f * std::pow (18000.0f / 550.0f, damping);
    const float dampingCoefficient = 1.0f - std::exp (-2.0f * pi * std::min (dampingCutoff,
                                                  static_cast<float> (sampleRate * 0.45))
                                                  / static_cast<float> (sampleRate));

    const float dryLeft = finiteOrZero (inputLeft);
    const float dryRight = stereo ? finiteOrZero (inputRight) : dryLeft;
    const float wetLeft = processChannel (dryLeft, delayLeft, dampingLeft, delaySamples,
                                          feedbackGain, dampingCoefficient, character);
    const float wetRight = stereo ? processChannel (dryRight, delayRight, dampingRight, delaySamples,
                                                   feedbackGain, dampingCoefficient, character)
                                  : wetLeft;

    // On a steady OFF state this is a bit-transparent pass-through. During the
    // transition the enable envelope crossfades smoothly to/from the module.
    if (enabled <= 0.0f)
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
    }
    else
    {
        const float wetMixLeft = dryLeft + (wetLeft - dryLeft) * mix;
        const float wetMixRight = dryRight + (wetRight - dryRight) * mix;
        outputLeft = dryLeft + (wetMixLeft - dryLeft) * enabled;
        outputRight = stereo ? dryRight + (wetMixRight - dryRight) * enabled : 0.0f;
    }

    if (++writeIndex >= static_cast<int> (delayLeft.size()))
        writeIndex = 0;
}
