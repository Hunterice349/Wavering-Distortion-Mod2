#include "../src/DistortionDSP.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "TEST FAILED: " << message << '\n';
        std::exit(1);
    }
}

float renderSteadyState(DistortionDSP::Mode mode, float input = 0.2f)
{
    DistortionDSP::Parameters p;
    p.drive = 2.0f;
    p.tone = 10.0f;
    p.bias = 5.0f;
    p.body = 5.0f;
    p.mix = 10.0f;
    p.mode = mode;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);
    float left = 0.0f, right = 0.0f;
    for (int i = 0; i < 12000; ++i)
        dsp.processFrame(input, input, true, left, right);
    return left;
}

void testExactBypassPreservesHeadroom()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.mix = 10.0f;
    p.enabled = false;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);
    const std::array<float, 5> values { 1.25f, -1.75f, 4.0f, -3.0f, 0.375f };
    for (float value : values)
    {
        float left = 0.0f, right = 0.0f;
        dsp.processFrame(value, -value, true, left, right);
        require(left == value && right == -value,
                "disabled bypass must preserve finite input exactly, including values outside +/-1");
    }
}

void testExactZeroMix()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.mix = 0.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);
    for (int i = 0; i < 100; ++i)
    {
        const float input = (i % 2 == 0) ? 2.25f : -2.75f;
        float left = 0.0f, right = 0.0f;
        dsp.processFrame(input, -input, true, left, right);
        require(left == input && right == -input,
                "Mix at zero must be an exact dry path without clipping");
    }
}

void testLegacyDriveCurve()
{
    const float actual = renderSteadyState(DistortionDSP::Mode::drive);
    const float expected = std::tanh(1.0f);
    require(std::abs(actual - expected) < 0.002f,
            "Drive mode must preserve the legacy tanh transfer curve");
}

void testModesAreDistinct()
{
    const std::array<DistortionDSP::Mode, 5> modes {
        DistortionDSP::Mode::drive,
        DistortionDSP::Mode::overdrive,
        DistortionDSP::Mode::distortion,
        DistortionDSP::Mode::fuzz,
        DistortionDSP::Mode::hardClip
    };

    std::array<float, 5> results {};
    for (size_t i = 0; i < modes.size(); ++i)
        results[i] = renderSteadyState(modes[i], 0.17f);

    for (size_t i = 0; i < results.size(); ++i)
        for (size_t j = i + 1; j < results.size(); ++j)
            require(std::abs(results[i] - results[j]) > 0.0005f,
                    "Each distortion mode must produce a distinct response for the test signal");
}

void testNonFiniteInputsAndExtremeValues()
{
    const std::array<float, 7> inputs {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        1.0e20f,
        -1.0e20f
    };

    for (int mode = 0; mode < 5; ++mode)
    {
        DistortionDSP::Parameters p;
        p.drive = 10.0f;
        p.tone = 5.0f;
        p.bias = 10.0f;
        p.body = 10.0f;
        p.mix = 10.0f;
        p.mode = static_cast<DistortionDSP::Mode>(mode);
        p.enabled = true;

        DistortionDSP dsp;
        dsp.prepare(48000.0, p);
        for (float input : inputs)
        {
            float left = 0.0f, right = 0.0f;
            dsp.processFrame(input, input, true, left, right);
            require(std::isfinite(left) && std::isfinite(right),
                    "Non-finite and extreme input must never create non-finite output");
        }
    }
}

void testMonoAndStereoOutputContract()
{
    DistortionDSP::Parameters p;
    p.drive = 3.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(44100.0, p);
    float left = 0.0f, right = 123.0f;
    dsp.processFrame(0.25f, -0.5f, false, left, right);
    require(std::isfinite(left) && right == 0.0f,
            "Mono processing must produce a finite left output and clear right output");

    dsp.reset();
    dsp.processFrame(0.25f, -0.5f, true, left, right);
    require(std::isfinite(left) && std::isfinite(right),
            "Stereo processing must produce finite output on both channels");
}

void testModeTransitionRemainsFinite()
{
    DistortionDSP::Parameters p;
    p.drive = 6.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);
    float left = 0.0f, right = 0.0f;
    for (int mode = 0; mode < 5; ++mode)
    {
        p.mode = static_cast<DistortionDSP::Mode>(mode);
        dsp.setParameters(p);
        for (int i = 0; i < 1500; ++i)
        {
            dsp.processFrame(0.35f, -0.35f, true, left, right);
            require(std::isfinite(left) && std::isfinite(right),
                    "Smoothed mode transitions must remain finite");
        }
    }
}
}

int main()
{
    testExactBypassPreservesHeadroom();
    testExactZeroMix();
    testLegacyDriveCurve();
    testModesAreDistinct();
    testNonFiniteInputsAndExtremeValues();
    testMonoAndStereoOutputContract();
    testModeTransitionRemainsFinite();
    std::cout << "All DistortionDSP tests passed.\n";
    return 0;
}
