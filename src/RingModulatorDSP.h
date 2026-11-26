#pragma once

#include <algorithm>
#include <cmath>

// Allocation-free, channel-linked ring modulation with a shared stereo carrier phase.
class RingModulatorDSP
{
public:
    enum class Waveform : int { sine = 0, triangle, square, saw };

    struct Parameters
    {
        float rate = 5.0f;
        float depth = 5.0f;
        int waveform = 0;
        float fine = 5.0f;
        float mix = 5.0f;
        bool enabled = false;
    };

    void prepare (double sampleRate, const Parameters& initial) noexcept;
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
    static float polyBlep (float phase, float phaseIncrement) noexcept;
    static float oscillator (int waveform, float phase, float phaseIncrement) noexcept;
    static float safeOutput (float value) noexcept;

    double sampleRate = 44100.0;
    float phase = 0.0f;
    Smoother rateSmoother, depthSmoother, waveformSmoother, fineSmoother;
    Smoother mixSmoother, enableSmoother;
};
