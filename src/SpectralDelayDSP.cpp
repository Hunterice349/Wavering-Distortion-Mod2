#include "SpectralDelayDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float maximumFeedback = 0.92f;
constexpr float maximumDelaySeconds = 2.3f;
constexpr float maximumSignal = 32.0f;
constexpr float crossoverLowHz = 250.0f;
constexpr float crossoverHighHz = 2500.0f;
constexpr float inverseSqrtTwo = 0.7071067811865475f;
}

void SpectralDelayDSP::Smoother::reset (int rampSamples, float initial) noexcept
{
    rampLength = std::max (1, rampSamples);
    current = target = initial;
    increment = 0.0f;
    remaining = 0;
}

void SpectralDelayDSP::Smoother::setTarget (float value) noexcept
{
    if (value == target)
        return;

    target = value;
    remaining = rampLength;
    increment = (target - current) / static_cast<float> (remaining);
}

float SpectralDelayDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;
        if (--remaining == 0)
            current = target;
    }
    return current;
}

void SpectralDelayDSP::Biquad::setLowPass (double rate, float cutoff) noexcept
{
    const float safeRate = static_cast<float> (std::max (rate, 1.0));
    const float safeCutoff = std::clamp (cutoff, 10.0f, safeRate * 0.45f);
    const float omega = 2.0f * pi * safeCutoff / safeRate;
    const float cosine = std::cos (omega);
    const float alpha = std::sin (omega) * inverseSqrtTwo;
    const float a0 = 1.0f + alpha;
    b0 = (1.0f - cosine) * 0.5f / a0;
    b1 = (1.0f - cosine) / a0;
    b2 = b0;
    a1 = -2.0f * cosine / a0;
    a2 = (1.0f - alpha) / a0;
}

void SpectralDelayDSP::Biquad::setHighPass (double rate, float cutoff) noexcept
{
    const float safeRate = static_cast<float> (std::max (rate, 1.0));
    const float safeCutoff = std::clamp (cutoff, 10.0f, safeRate * 0.45f);
    const float omega = 2.0f * pi * safeCutoff / safeRate;
    const float cosine = std::cos (omega);
    const float alpha = std::sin (omega) * inverseSqrtTwo;
    const float a0 = 1.0f + alpha;
    b0 = (1.0f + cosine) * 0.5f / a0;
    b1 = -(1.0f + cosine) / a0;
    b2 = b0;
    a1 = -2.0f * cosine / a0;
    a2 = (1.0f - alpha) / a0;
}

float SpectralDelayDSP::Biquad::process (float input) noexcept
{
    const float output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    if (! std::isfinite (z1) || ! std::isfinite (z2) || ! std::isfinite (output))
    {
        reset();
        return 0.0f;
    }
    return output;
}

void SpectralDelayDSP::CrossoverState::reset() noexcept
{
    lowPass1.reset(); lowPass2.reset();
    lowCutHighPass1.reset(); lowCutHighPass2.reset();
    midLowPass1.reset(); midLowPass2.reset();
    highPass1.reset(); highPass2.reset();
}

float SpectralDelayDSP::finiteOrZero (float value) noexcept
{
    return std::isfinite (value) ? value : 0.0f;
}

float SpectralDelayDSP::bounded (float value, float limit) noexcept
{
    return std::clamp (finiteOrZero (value), -limit, limit);
}

void SpectralDelayDSP::prepare (double newSampleRate, const Parameters& initial)
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1000.0
                   ? std::clamp (newSampleRate, 8000.0, 384000.0) : 44100.0;

    const auto capacity = static_cast<size_t> (std::ceil (sampleRate * maximumDelaySeconds)) + 4u;
    for (auto& line : delayLines)
        line.assign (capacity, 0.0f);

    const float nyquistSafe = static_cast<float> (sampleRate * 0.45);
    const float lowCutoff = std::min (crossoverLowHz, nyquistSafe);
    const float highCutoff = std::min (crossoverHighHz, nyquistSafe);
    for (auto& state : crossovers)
    {
        state.lowPass1.setLowPass (sampleRate, lowCutoff);
        state.lowPass2.setLowPass (sampleRate, lowCutoff);
        state.lowCutHighPass1.setHighPass (sampleRate, lowCutoff);
        state.lowCutHighPass2.setHighPass (sampleRate, lowCutoff);
        state.midLowPass1.setLowPass (sampleRate, highCutoff);
        state.midLowPass2.setLowPass (sampleRate, highCutoff);
        state.highPass1.setHighPass (sampleRate, highCutoff);
        state.highPass2.setHighPass (sampleRate, highCutoff);
    }

    const int rampSamples = std::max (1, static_cast<int> (std::lround (sampleRate * 0.02)));
    timeSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.time), 0.0f, 10.0f));
    lowSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.low), 0.0f, 10.0f));
    midSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.mid), 0.0f, 10.0f));
    highSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.high), 0.0f, 10.0f));
    feedbackSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.feedback), 0.0f, 10.0f));
    widthSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.width), 0.0f, 10.0f));
    mixSmoother.reset (rampSamples, std::clamp (finiteOrZero (initial.mix), 0.0f, 10.0f));
    enableSmoother.reset (rampSamples, initial.enabled ? 1.0f : 0.0f);
    reset();
}

void SpectralDelayDSP::reset() noexcept
{
    for (auto& line : delayLines)
        std::fill (line.begin(), line.end(), 0.0f);
    for (auto& state : crossovers)
        state.reset();
    writeIndex = 0;
}

void SpectralDelayDSP::setParameters (const Parameters& parameters) noexcept
{
    timeSmoother.setTarget (std::clamp (finiteOrZero (parameters.time), 0.0f, 10.0f));
    lowSmoother.setTarget (std::clamp (finiteOrZero (parameters.low), 0.0f, 10.0f));
    midSmoother.setTarget (std::clamp (finiteOrZero (parameters.mid), 0.0f, 10.0f));
    highSmoother.setTarget (std::clamp (finiteOrZero (parameters.high), 0.0f, 10.0f));
    feedbackSmoother.setTarget (std::clamp (finiteOrZero (parameters.feedback), 0.0f, 10.0f));
    widthSmoother.setTarget (std::clamp (finiteOrZero (parameters.width), 0.0f, 10.0f));
    mixSmoother.setTarget (std::clamp (finiteOrZero (parameters.mix), 0.0f, 10.0f));
    enableSmoother.setTarget (parameters.enabled ? 1.0f : 0.0f);
}

float SpectralDelayDSP::readDelay (const std::vector<float>& line, float delaySamples) const noexcept
{
    if (line.size() < 4u)
        return 0.0f;

    const float maximum = static_cast<float> (line.size() - 2u);
    const float delay = std::clamp (delaySamples, 1.0f, maximum);
    float position = static_cast<float> (writeIndex) - delay;
    if (position < 0.0f)
        position += static_cast<float> (line.size());

    const int index0 = static_cast<int> (position);
    const int index1 = (index0 + 1) % static_cast<int> (line.size());
    const float fraction = position - static_cast<float> (index0);
    const auto a = line[static_cast<size_t> (index0)];
    const auto b = line[static_cast<size_t> (index1)];
    return finiteOrZero (a + (b - a) * fraction);
}

void SpectralDelayDSP::splitBands (float input, CrossoverState& state,
                                   float& low, float& mid, float& high) noexcept
{
    // Each two-pole Butterworth branch is cascaded to produce LR4 sections.
    // The second crossover acts on the low-crossover high branch, so the
    // three bands sum coherently when their delay times are equal.
    low = state.lowPass2.process (state.lowPass1.process (input));
    const float aboveLow = state.lowCutHighPass2.process (state.lowCutHighPass1.process (input));
    mid = state.midLowPass2.process (state.midLowPass1.process (aboveLow));
    high = state.highPass2.process (state.highPass1.process (aboveLow));
}

float SpectralDelayDSP::processBand (float input, std::vector<float>& line,
                                     float delaySamples, float feedback) noexcept
{
    const float delayed = readDelay (line, delaySamples);
    const float writeValue = bounded (finiteOrZero (input) + delayed * feedback, maximumSignal);
    if (! line.empty())
        line[static_cast<size_t> (writeIndex)] = writeValue;
    return delayed;
}

void SpectralDelayDSP::processFrame (float inputLeft, float inputRight, bool stereo,
                                     float& outputLeft, float& outputRight) noexcept
{
    const float time = timeSmoother.next();
    const float lowTime = lowSmoother.next();
    const float midTime = midSmoother.next();
    const float highTime = highSmoother.next();
    const float feedbackControl = feedbackSmoother.next() * 0.1f;
    const float widthControl = widthSmoother.next() * 0.1f;
    const float mix = mixSmoother.next() * 0.1f;
    const float enabled = enableSmoother.next();

    if (delayLines[0].empty())
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
        return;
    }

    const float dryLeft = bounded (inputLeft, maximumSignal);
    const float dryRight = stereo ? bounded (inputRight, maximumSignal) : dryLeft;
    float lowL, midL, highL, lowR = 0.0f, midR = 0.0f, highR = 0.0f;
    splitBands (dryLeft, crossovers[0], lowL, midL, highL);
    if (stereo)
        splitBands (dryRight, crossovers[1], lowR, midR, highR);

    // TIME is exponential 10 ms..1.5 s. Band controls scale this base from
    // 0.5x..1.5x with 5 neutral. The allocation accommodates the 2.25 s maximum.
    const float baseSeconds = 0.01f * std::pow (150.0f, time * 0.1f);
    const auto delaySamplesFor = [this, baseSeconds] (float band) noexcept
    {
        const float factor = 0.5f + 0.1f * band;
        const float seconds = std::min (baseSeconds * factor, maximumDelaySeconds - 0.01f);
        return std::clamp (seconds * static_cast<float> (sampleRate), 1.0f,
                           static_cast<float> (delayLines[0].size() - 2u));
    };

    const float feedback = maximumFeedback * feedbackControl;
    const float lowDelay = delaySamplesFor (lowTime);
    const float midDelay = delaySamplesFor (midTime);
    const float highDelay = delaySamplesFor (highTime);

    // Do not feed fresh input into the delay network while bypassed or with a
    // zero mix. Existing echoes continue to decay under bounded feedback.
    const float inputGate = (mix > 0.0f ? enabled : 0.0f);
    const float wetL = processBand (lowL * inputGate, delayLines[0], lowDelay, feedback)
                       + processBand (midL * inputGate, delayLines[1], midDelay, feedback)
                       + processBand (highL * inputGate, delayLines[2], highDelay, feedback);
    float wetR = wetL;
    if (stereo)
        wetR = processBand (lowR * inputGate, delayLines[3], lowDelay, feedback)
               + processBand (midR * inputGate, delayLines[4], midDelay, feedback)
               + processBand (highR * inputGate, delayLines[5], highDelay, feedback);

    if (enabled <= 0.0f || mix <= 0.0f)
    {
        outputLeft = inputLeft;
        outputRight = stereo ? inputRight : 0.0f;
    }
    else if (stereo)
    {
        // 0 is mono, 5 is neutral, and 10 widens by at most 50 percent.
        // The moderate cap limits peak growth and avoids extreme channel inversion.
        const float width = std::min (0.5f + widthControl, 1.5f);
        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * width;
        const float widenedLeft = mid + side;
        const float widenedRight = mid - side;
        const float wetBlendLeft = dryLeft + (bounded (widenedLeft, maximumSignal) - dryLeft) * mix;
        const float wetBlendRight = dryRight + (bounded (widenedRight, maximumSignal) - dryRight) * mix;
        outputLeft = bounded (dryLeft + (wetBlendLeft - dryLeft) * enabled, maximumSignal);
        outputRight = bounded (dryRight + (wetBlendRight - dryRight) * enabled, maximumSignal);
    }
    else
    {
        const float wetBlend = dryLeft + (bounded (wetL, maximumSignal) - dryLeft) * mix;
        outputLeft = bounded (dryLeft + (wetBlend - dryLeft) * enabled, maximumSignal);
        outputRight = 0.0f;
    }

    if (++writeIndex >= static_cast<int> (delayLines[0].size()))
        writeIndex = 0;
}
