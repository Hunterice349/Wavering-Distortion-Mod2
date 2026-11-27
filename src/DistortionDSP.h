#pragma once

#include <algorithm>
#include <cmath>

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

    void prepare(double sampleRate, const Parameters& initial) noexcept;
    void reset() noexcept;
    void setParameters(const Parameters& parameters) noexcept;

    void processFrame(float inputLeft, float inputRight, bool stereo,
                      float& outputLeft, float& outputRight) noexcept;

private:
    class Smoother
    {
    public:
        void reset(int rampSamples, float initial) noexcept;
        void setTarget(float value) noexcept;
        float next() noexcept;

    private:
        float current = 0.0f;
        float target = 0.0f;
        float increment = 0.0f;
        int remaining = 0;
        int rampLength = 1;
    };

    static float finiteOrZero(float value) noexcept;
    static float safeOutput(float value) noexcept;

    float processChannel(float input, float& toneState,
                         float toneCoefficient, float drive,
                         float bias, float body) noexcept;

    double sampleRate = 44100.0;

    float toneStateLeft = 0.0f;
    float toneStateRight = 0.0f;

    Smoother driveSmoother;
    Smoother toneSmoother;
    Smoother biasSmoother;
    Smoother bodySmoother;
    Smoother mixSmoother;
    Smoother enableSmoother;
};