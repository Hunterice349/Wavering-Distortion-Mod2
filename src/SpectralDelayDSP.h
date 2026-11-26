#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Three-band, per-channel delay with complementary Linkwitz-Riley crossovers.
// All delay storage is allocated in prepare(); processFrame is allocation-free.
class SpectralDelayDSP
{
public:
    struct Parameters
    {
        float time = 3.0f;
        float low = 5.0f;
        float mid = 5.0f;
        float high = 5.0f;
        float feedback = 3.0f;
        float width = 5.0f;
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

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void setLowPass (double sampleRate, float cutoff) noexcept;
        void setHighPass (double sampleRate, float cutoff) noexcept;
        float process (float input) noexcept;
        void reset() noexcept { z1 = z2 = 0.0f; }
    };

    struct CrossoverState
    {
        Biquad lowPass1, lowPass2;
        Biquad lowCutHighPass1, lowCutHighPass2;
        Biquad midLowPass1, midLowPass2;
        Biquad highPass1, highPass2;
        void reset() noexcept;
    };

    static float finiteOrZero (float value) noexcept;
    static float bounded (float value, float limit) noexcept;
    float readDelay (const std::vector<float>& line, float delaySamples) const noexcept;
    void splitBands (float input, CrossoverState& state, float& low, float& mid, float& high) noexcept;
    float processBand (float input, std::vector<float>& line, float delaySamples,
                       float feedback) noexcept;

    double sampleRate = 44100.0;
    int writeIndex = 0;
    std::array<std::vector<float>, 6> delayLines;
    std::array<CrossoverState, 2> crossovers;
    Smoother timeSmoother, lowSmoother, midSmoother, highSmoother;
    Smoother feedbackSmoother, widthSmoother, mixSmoother, enableSmoother;
};
