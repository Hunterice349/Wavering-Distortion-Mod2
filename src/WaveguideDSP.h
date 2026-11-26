#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Independent, allocation-free delay-line resonator. Storage is allocated only
// by prepare(); processFrame() is suitable for the realtime audio thread.
class WaveguideDSP
{
public:
    struct Parameters
    {
        float rate = 5.0f;
        float resonance = 5.0f;
        float feedback = 4.0f;
        float damping = 5.0f;
        float character = 0.0f;
        float mix = 0.0f;
        bool enabled = false;
    };

    void prepare (double sampleRate, const Parameters& initial);
    void reset() noexcept;
    void setParameters (const Parameters& parameters) noexcept;
    void processFrame (float inputLeft, float inputRight, bool stereo,
                       float& outputLeft, float& outputRight) noexcept;

private:
    class Smoother
    {
    public:
        void reset (int rampSamples, float initial) noexcept;
        void setTarget (float value) noexcept;
        float next() noexcept;

    private:
        float current = 0.0f, target = 0.0f, increment = 0.0f;
        int remaining = 0, rampLength = 1;
    };

    static float finiteOrZero (float value) noexcept;
    float readDelay (const std::vector<float>& buffer, float delaySamples) const noexcept;
    float processChannel (float input, std::vector<float>& buffer, float& dampingState,
                          float delaySamples, float feedbackGain, float dampingCoefficient,
                          float character) noexcept;

    double sampleRate = 44100.0;
    int writeIndex = 0;
    std::vector<float> delayLeft, delayRight;
    float dampingLeft = 0.0f, dampingRight = 0.0f;
    Smoother rateSmoother, resonanceSmoother, feedbackSmoother;
    Smoother dampingSmoother, characterSmoother, mixSmoother, enableSmoother;
};
