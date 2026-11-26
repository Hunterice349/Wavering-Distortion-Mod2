
#include "WaveguideDSP.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
int failures = 0;

void require (bool condition, const char* message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void process (WaveguideDSP& dsp, float inL, float inR, bool stereo, float& outL, float& outR)
{
    dsp.processFrame (inL, inR, stereo, outL, outR);
    require (std::isfinite (outL) && std::isfinite (outR), "outputs remain finite");
}

void testChannelsAndBypass()
{
    WaveguideDSP::Parameters p;
    p.enabled = false;
    p.mix = 10.0f;

    WaveguideDSP bypass;
    bypass.prepare (48000.0, p);

    float l = 0.0f, r = 0.0f;

    process (bypass, 0.25f, -0.5f, true, l, r);
    require (l == 0.25f && r == -0.5f, "disabled stereo is bit-transparent");

    process (bypass, 0.25f, 9.0f, false, l, r);
    require (l == 0.25f && r == 0.0f, "mono clears right output while bypassed");

    p.enabled = true;
    p.mix = 0.0f;

    WaveguideDSP dry;
    dry.prepare (48000.0, p);

    process (dry, 0.25f, -0.5f, true, l, r);
    require (l == 0.25f && r == -0.5f, "Mix=0 preserves stereo dry path");

    process (dry, 0.25f, 0.0f, false, l, r);
    require (l == 0.25f && r == 0.0f, "Mix=0 mono preserves dry path and clears right");
}

void testNonFiniteAndRates()
{
    const std::array<double, 3> rates { 44100.0, 48000.0, 96000.0 };

    const std::array<float, 3> bad {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };

    WaveguideDSP::Parameters p;
    p.enabled = true;
    p.mix = 10.0f;

    for (const auto rate : rates)
    {
        WaveguideDSP dsp;
        dsp.prepare (rate, p);

        float l = 0.0f, r = 0.0f;

        for (const auto input : bad)
            process (dsp, input, input, true, l, r);

        // Re-preparing replaces delay storage and resets state at a changed rate.
        dsp.prepare (rate == 44100.0 ? 96000.0 : 44100.0, p);

        process (dsp, 0.2f, -0.3f, true, l, r);

        require (std::isfinite (l) && std::isfinite (r),
                 "re-prepare at changed rate remains finite");
    }
}

void testResetAndFeedbackDecay()
{
    WaveguideDSP::Parameters p;
    p.enabled = true;
    p.mix = 10.0f;
    p.rate = 0.0f;
    p.resonance = 10.0f;
    p.feedback = 10.0f;
    p.damping = 5.0f;

    WaveguideDSP afterReset, fresh;

    afterReset.prepare (48000.0, p);
    fresh.prepare (48000.0, p);

    float l = 0.0f, r = 0.0f;

    process (afterReset, 1.0f, 1.0f, true, l, r);

    for (int i = 0; i < 2000; ++i)
        process (afterReset, 0.0f, 0.0f, true, l, r);

    afterReset.reset();

    for (int i = 0; i < 128; ++i)
    {
        float resetL = 0.0f, resetR = 0.0f;
        float freshL = 0.0f, freshR = 0.0f;

        process (afterReset, 0.0f, 0.0f, true, resetL, resetR);
        process (fresh, 0.0f, 0.0f, true, freshL, freshR);

        require (std::abs (resetL - freshL) < 1.0e-6f
                 && std::abs (resetR - freshR) < 1.0e-6f,
                 "reset clears delay and damping state");
    }

    WaveguideDSP::Parameters resonatorParameters = p;
    resonatorParameters.enabled = true;
    resonatorParameters.rate = 5.0f;

    WaveguideDSP decay;
    decay.prepare (48000.0, resonatorParameters);

    double earlyPeak = 0.0;
    double latePeak = 0.0;

    for (int i = 0; i < 48000; ++i)
    {
        process (decay, i == 0 ? 1.0f : 0.0f, 0.0f, false, l, r);

        const double magnitude = std::abs (l);

        if (i < 2400)
            earlyPeak = std::max (earlyPeak, magnitude);

        if (i >= 38400)
            latePeak = std::max (latePeak, magnitude);
    }

    require (earlyPeak > 0.01,
             "impulse produces a measurable resonator response");

    require (latePeak < earlyPeak,
             "feedback response decays over time");
}
}

int main()
{
    testChannelsAndBypass();
    testNonFiniteAndRates();
    testResetAndFeedbackDecay();

    if (failures != 0)
        std::cerr << "Waveguide DSP: " << failures << " assertion(s) failed.\n";
    else
        std::cout << "Waveguide DSP checks passed.\n";

    return failures == 0 ? 0 : 1;
}
