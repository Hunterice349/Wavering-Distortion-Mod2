#include "SpectralDelayDSP.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
int failures = 0;
void require (bool condition, const char* message)
{
    if (! condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
void process (SpectralDelayDSP& dsp, float inL, float inR, bool stereo, float& outL, float& outR)
{
    dsp.processFrame (inL, inR, stereo, outL, outR);
    require (std::isfinite (outL) && std::isfinite (outR), "outputs remain finite");
}

void testChannelsBypassAndRates()
{
    SpectralDelayDSP::Parameters p;
    p.enabled = false;
    p.mix = 10.0f;
    SpectralDelayDSP bypass;
    bypass.prepare (48000.0, p);
    float l = 0.0f, r = 0.0f;
    process (bypass, 0.25f, -0.5f, true, l, r);
    require (l == 0.25f && r == -0.5f, "disabled stereo is bit-transparent");
    process (bypass, 0.25f, 0.0f, false, l, r);
    require (l == 0.25f && r == 0.0f, "mono clears right output while bypassed");

    p.enabled = true;
    p.mix = 0.0f;
    SpectralDelayDSP dry;
    dry.prepare (48000.0, p);
    process (dry, 0.25f, -0.5f, true, l, r);
    require (l == 0.25f && r == -0.5f, "Mix=0 preserves dry signal");

    p.mix = 10.0f;
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        SpectralDelayDSP dsp;
        dsp.prepare (rate, p);
        process (dsp, 0.2f, -0.1f, true, l, r);
        dsp.prepare (rate == 44100.0 ? 96000.0 : 44100.0, p);
        process (dsp, 0.2f, -0.1f, false, l, r);
        require (std::isfinite (l) && r == 0.0f, "prepare at changed rate and mono remain valid");
    }
}

void testNonFiniteAndReset()
{
    SpectralDelayDSP::Parameters p;
    p.enabled = true;
    p.mix = 10.0f;
    p.feedback = 8.0f;
    SpectralDelayDSP dsp, fresh;
    dsp.prepare (48000.0, p);
    fresh.prepare (48000.0, p);
    float l = 0.0f, r = 0.0f;
    const std::array<float, 3> bad { std::numeric_limits<float>::quiet_NaN(),
                                     std::numeric_limits<float>::infinity(),
                                     -std::numeric_limits<float>::infinity() };
    for (float x : bad) process (dsp, x, x, true, l, r);
    dsp.reset();
    for (int i = 0; i < 256; ++i)
    {
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
        process (dsp, 0.0f, 0.0f, true, a, b);
        process (fresh, 0.0f, 0.0f, true, c, d);
        require (std::abs (a - c) < 1.0e-6f && std::abs (b - d) < 1.0e-6f,
                 "reset clears delay lines and crossover state");
    }
}

void testDelayedImpulseAndFeedback()
{
    SpectralDelayDSP::Parameters p;
    p.time = 0.0f; // 10 ms base delay, all bands neutral
    p.low = p.mid = p.high = 5.0f;
    p.feedback = 10.0f;
    p.width = 5.0f;
    p.mix = 10.0f;
    p.enabled = true;
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        SpectralDelayDSP dsp;
        dsp.prepare (rate, p);
        float l = 0.0f, r = 0.0f;
        double delayedPeak = 0.0, earlyPeak = 0.0, latePeak = 0.0;
        const int length = static_cast<int> (rate * 0.8);
        for (int i = 0; i < length; ++i)
        {
            process (dsp, i == 0 ? 1.0f : 0.0f, 0.0f, false, l, r);
            const double magnitude = std::abs (l);
            if (i < static_cast<int> (rate * 0.008)) earlyPeak = std::max (earlyPeak, magnitude);
            if (i >= static_cast<int> (rate * 0.008) && i <= static_cast<int> (rate * 0.03))
                delayedPeak = std::max (delayedPeak, magnitude);
            if (i > static_cast<int> (rate * 0.6)) latePeak = std::max (latePeak, magnitude);
        }
        require (delayedPeak > 1.0e-4, "impulse produces an audible delayed response near configured delay");
        require (earlyPeak > 0.1, "dry impulse is present before delayed response");
        require (latePeak < 0.1, "feedback response decays and remains bounded");
    }
}

void testThreeBands()
{
    const std::array<float, 3> frequencies { 100.0f, 1000.0f, 6000.0f };
    for (const float frequency : frequencies)
    {
        SpectralDelayDSP::Parameters p;
        p.time = 0.0f;
        p.low = p.mid = p.high = 5.0f;
        p.feedback = 0.0f;
        p.mix = 10.0f;
        p.enabled = true;
        SpectralDelayDSP dsp;
        dsp.prepare (48000.0, p);
        double delayedEnergy = 0.0;
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 12000; ++i)
        {
            const float input = 0.5f * std::sin (6.28318530718f * frequency * i / 48000.0f);
            process (dsp, input, input, true, l, r);
            if (i > 1000)
                delayedEnergy += static_cast<double> (l) * l;
        }
        require (delayedEnergy > 0.01, "low/mid/high band tone produces delayed output energy");
    }
}
}

int main()
{
    testChannelsBypassAndRates();
    testNonFiniteAndReset();
    testDelayedImpulseAndFeedback();
    testThreeBands();
    if (failures != 0) std::cerr << "Spectral Delay DSP: " << failures << " assertion(s) failed.\n";
    else std::cout << "Spectral Delay DSP checks passed.\n";
    return failures == 0 ? 0 : 1;
}
