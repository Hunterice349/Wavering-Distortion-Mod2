#include "RingModulatorDSP.h"

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
void process (RingModulatorDSP& dsp, float inL, float inR, bool stereo, float& outL, float& outR)
{
    dsp.processFrame (inL, inR, stereo, outL, outR);
    require (std::isfinite (outL) && std::isfinite (outR), "outputs remain finite");
}

void testChannelsAndBypass()
{
    RingModulatorDSP::Parameters p;
    p.enabled = false;
    RingModulatorDSP bypass;
    bypass.prepare (48000.0, p);
    float l = 0.0f, r = 0.0f;
    process (bypass, 0.25f, -0.5f, true, l, r);
    require (l == 0.25f && r == -0.5f, "disabled stereo is bit-transparent");
    process (bypass, 0.25f, 1.0f, false, l, r);
    require (l == 0.25f && r == 0.0f, "mono clears right output while bypassed");
    p.enabled = true;
    p.mix = 0.0f;
    p.rate = 5.0f;
    p.depth = 10.0f;
    RingModulatorDSP dry;
    dry.prepare (48000.0, p);
    process (dry, 0.4f, -0.2f, true, l, r);
    require (l == 0.4f && r == -0.2f, "Mix=0 preserves exact stereo dry path");
}

void testWaveformsAndRates()
{
    const std::array<double, 3> rates { 44100.0, 48000.0, 96000.0 };
    std::array<double, 4> signatures {};
    for (int waveform = 0; waveform < 4; ++waveform)
    {
        RingModulatorDSP::Parameters p;
        p.rate = 7.0f;
        p.depth = 10.0f;
        p.waveform = waveform;
        p.fine = 5.0f;
        p.mix = 10.0f;
        p.enabled = true;
        RingModulatorDSP dsp;
        for (const auto rate : rates)
        {
            dsp.prepare (rate, p);
            float l = 0.0f, r = 0.0f;
            double sumSquares = 0.0;
            for (int i = 0; i < 4096; ++i)
            {
                const float input = 0.5f * std::sin (6.28318530718f * 997.0f * static_cast<float> (i / rate));
                process (dsp, input, -input, (i & 1) == 0, l, r);
                sumSquares += static_cast<double> (l) * l;
            }
            require (sumSquares > 0.01, "each waveform produces nonzero output energy");
        }
        // One deterministic pass at 48 kHz provides a per-mode signature.
        dsp.prepare (48000.0, p);
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 2048; ++i)
        {
            process (dsp, 0.5f, 0.0f, false, l, r);
            signatures[static_cast<std::size_t> (waveform)] += static_cast<double> (l) * (i + 1);
        }
    }
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            require (std::abs (signatures[static_cast<std::size_t> (i)]
                               - signatures[static_cast<std::size_t> (j)]) > 1.0e-4,
                     "waveform modes have distinct output signatures");
}

void testNonFiniteResetAndReprepare()
{
    RingModulatorDSP::Parameters p;
    p.enabled = true;
    p.mix = 10.0f;
    p.depth = 10.0f;
    RingModulatorDSP dsp;
    dsp.prepare (44100.0, p);
    float l = 0.0f, r = 0.0f;
    const std::array<float, 3> bad { std::numeric_limits<float>::quiet_NaN(),
                                     std::numeric_limits<float>::infinity(),
                                     -std::numeric_limits<float>::infinity() };
    for (float x : bad) process (dsp, x, x, true, l, r);

    dsp.reset();
    RingModulatorDSP fresh;
    fresh.prepare (44100.0, p);
    for (int i = 0; i < 128; ++i)
    {
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
        process (dsp, 0.5f, -0.25f, true, a, b);
        process (fresh, 0.5f, -0.25f, true, c, d);
        require (std::abs (a - c) < 1.0e-6f && std::abs (b - d) < 1.0e-6f,
                 "reset restores carrier phase");
    }
    for (double rate : { 96000.0, 48000.0, 44100.0 })
    {
        dsp.prepare (rate, p);
        process (dsp, 0.3f, -0.3f, true, l, r);
        require (std::isfinite (l) && std::isfinite (r), "re-prepare at each sample rate remains finite");
    }
}
}

int main()
{
    testChannelsAndBypass();
    testWaveformsAndRates();
    testNonFiniteResetAndReprepare();
    if (failures != 0) std::cerr << "Ring Modulator DSP: " << failures << " assertion(s) failed.\n";
    else std::cout << "Ring Modulator DSP checks passed.\n";
    return failures == 0 ? 0 : 1;
}
