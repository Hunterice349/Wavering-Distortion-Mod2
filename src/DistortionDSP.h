#pragma once

#include <algorithm>
#include <cmath>

// Allocation-free, channel-linked parameter smoothing and distortion processing.
class DistortionDSP
{
public:
    struct Parameters
    {
        float drive = 0.0f;
        float tone = 5.0f;
        float bias = 5.0f;
        float body = 5.0f;
        float mix = 10.0f;
        bool enabled = true;
    };

    void prepare (double sampleRate, const Parameters& initial) noexcept;
    void setParameters (const Parameters& parameters) noexcept;
    void reset() noexcept;

    void processFrame (float inputLeft, float inputRight, bool stereo,
                       float& outputLeft, float& outputRight) noexcept;

private:
    class Smoother
    {
    public:
        void reset (int rampSamples, float value) noexcept;
        void setTarget (float value) noexcept;
        float next() noexcept;

    private:
        float current = 0.0f;
        float target = 0.0f;
        float increment = 0.0f;
        int remaining = 0;
        int rampLength = 1;
    };

    float processChannel (float input, float& toneState, float toneCoefficient,
                          float drive, float bias, float body) const noexcept;
    static float finiteOrZero (float value) noexcept;
    static float safeOutput (float value) noexcept;

    double sampleRate = 44100.0;
    int smoothingSamples = 441;
    Smoother driveSmoother, toneSmoother, biasSmoother, bodySmoother, mixSmoother, enableSmoother;
    float toneStateLeft = 0.0f, toneStateRight = 0.0f;
};
