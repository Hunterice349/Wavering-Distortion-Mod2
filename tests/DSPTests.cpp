
#include "../src/DistortionDSP.h"
#include "../src/WaveguideDSP.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
constexpr float pi = 3.14159265358979323846f;

void require (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "TEST FAILED: " << message << '\n';
        std::exit (1);
    }
}

float renderSteadyState (
    DistortionDSP::Mode mode,
    float input = 0.2f)
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
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    for (int i = 0; i < 12000; ++i)
        dsp.processFrame (input, input, true, left, right);

    return left;
}

float renderToneRms (float tone)
{
    DistortionDSP::Parameters p;
    p.drive = 0.0f;
    p.tone = tone;
    p.bias = 5.0f;
    p.body = 5.0f;
    p.mix = 10.0f;
    p.mode = DistortionDSP::Mode::drive;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare (48000.0, p);

    double sum = 0.0;
    int count = 0;

    float left = 0.0f;
    float right = 0.0f;

    for (int i = 0; i < 48000; ++i)
    {
        const float input =
            0.1f * std::sin (
                2.0f * pi * 6000.0f *
                static_cast<float> (i) / 48000.0f);

        dsp.processFrame (input, input, true, left, right);

        if (i >= 12000)
        {
            sum += static_cast<double> (left) * left;
            ++count;
        }
    }

    return count > 0
        ? static_cast<float> (std::sqrt (sum / count))
        : 0.0f;
}

void testExactBypassPreservesHeadroom()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.mix = 10.0f;
    p.enabled = false;

    DistortionDSP dsp;
    dsp.prepare (48000.0, p);

    const std::array<float, 5> values {
        1.25f, -1.75f, 4.0f, -3.0f, 0.375f
    };

    for (float value : values)
    {
        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame (value, -value, true, left, right);

        require (
            left == value && right == -value,
            "Disabled bypass must preserve finite input exactly");
    }
}

void testExactZeroMix()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.mix = 0.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare (48000.0, p);

    for (int i = 0; i < 100; ++i)
    {
        const float input =
            (i % 2 == 0) ? 2.25f : -2.75f;

        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame (input, -input, true, left, right);

        require (
            left == input && right == -input,
            "Zero Mix must preserve the exact dry signal");
    }
}

void testLegacyDriveCurve()
{
    const float actual =
        renderSteadyState (DistortionDSP::Mode::drive);

    const float expected = std::tanh (1.0f);

    require (
        std::abs (actual - expected) < 0.002f,
        "Drive mode must preserve the legacy tanh curve");
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
        results[i] = renderSteadyState (modes[i], 0.17f);

    for (size_t i = 0; i < results.size(); ++i)
    {
        for (size_t j = i + 1; j < results.size(); ++j)
        {
            require (
                std::abs (results[i] - results[j]) > 0.0005f,
                "Distortion modes must produce distinct responses");
        }
    }
}

void testToneDirection()
{
    const float darkTone = renderToneRms (0.0f);
    const float brightTone = renderToneRms (10.0f);

    require (
        brightTone > darkTone * 2.0f,
        "Higher Tone setting must produce a brighter signal");
}

void testDistortionExtremeInputs()
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
        p.mode = static_cast<DistortionDSP::Mode> (mode);
        p.enabled = true;

        DistortionDSP dsp;
        dsp.prepare (48000.0, p);

        for (float input : inputs)
        {
            float left = 0.0f;
            float right = 0.0f;

            dsp.processFrame (input, input, true, left, right);

            require (
                std::isfinite (left) && std::isfinite (right),
                "Distortion must never produce NaN or infinity");
        }
    }
}

void testDistortionMonoStereo()
{
    DistortionDSP::Parameters p;
    p.drive = 3.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare (44100.0, p);

    float left = 0.0f;
    float right = 123.0f;

    dsp.processFrame (0.25f, -0.5f, false, left, right);

    require (
        std::isfinite (left) && right == 0.0f,
        "Mono processing must clear the unused right output");

    dsp.reset();

    dsp.processFrame (0.25f, -0.5f, true, left, right);

    require (
        std::isfinite (left) && std::isfinite (right),
        "Stereo outputs must remain finite");
}

void testModeTransitionRemainsFinite()
{
    DistortionDSP::Parameters p;
    p.drive = 6.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    for (int mode = 0; mode < 5; ++mode)
    {
        p.mode = static_cast<DistortionDSP::Mode> (mode);
        dsp.setParameters (p);

        for (int i = 0; i < 1500; ++i)
        {
            dsp.processFrame (0.35f, -0.35f, true, left, right);

            require (
                std::isfinite (left) && std::isfinite (right),
                "Mode transitions must remain finite");
        }
    }
}

void testWaveguideExactBypass()
{
    WaveguideDSP::Parameters p;
    p.mix = 10.0f;
    p.enabled = false;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    const std::array<float, 5> values {
        1.25f, -1.75f, 4.0f, -3.0f, 0.375f
    };

    for (float value : values)
    {
        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame (value, -value, true, left, right);

        require (
            left == value && right == -value,
            "Disabled Waveguide must preserve dry signal exactly");
    }

    p.enabled = true;
    p.mix = 0.0f;
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    dsp.processFrame (2.5f, -2.5f, true, left, right);

    require (
        left == 2.5f && right == -2.5f,
        "Zero Waveguide Mix must preserve dry signal exactly");
}

void testWaveguideImpulseDelay()
{
    WaveguideDSP::Parameters p;
    p.rate = 0.0f;
    p.resonance = 0.0f;
    p.feedback = 0.0f;
    p.damping = 5.0f;
    p.character = 0.0f;
    p.mix = 10.0f;
    p.enabled = true;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    for (int i = 0; i <= 110; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;

        dsp.processFrame (input, input, true, left, right);

        if (i < 96)
        {
            require (
                std::abs (left) < 0.0001f,
                "Waveguide must not output impulse before its delay");
        }

        if (i == 96)
        {
            require (
                std::abs (left - 1.0f) < 0.0001f,
                "Waveguide must output impulse at 2 ms / 96 samples");
        }
    }
}

void testWaveguideStereoIsolation()
{
    WaveguideDSP::Parameters p;
    p.rate = 0.0f;
    p.feedback = 0.0f;
    p.mix = 10.0f;
    p.enabled = true;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    for (int i = 0; i <= 96; ++i)
    {
        const float inputLeft = (i == 0) ? 1.0f : 0.0f;

        dsp.processFrame (
            inputLeft, 0.0f, true, left, right);

        require (
            std::abs (right) < 0.0001f,
            "Left input must not leak into the right channel");
    }

    require (
        std::abs (left - 1.0f) < 0.0001f,
        "Left channel must retain its delayed impulse");
}

void testWaveguideReset()
{
    WaveguideDSP::Parameters p;
    p.rate = 0.0f;
    p.feedback = 0.0f;
    p.mix = 10.0f;
    p.enabled = true;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    float left = 0.0f;
    float right = 0.0f;

    dsp.processFrame (1.0f, 1.0f, true, left, right);

    for (int i = 0; i < 20; ++i)
        dsp.processFrame (0.0f, 0.0f, true, left, right);

    dsp.reset();

    for (int i = 0; i < 110; ++i)
    {
        dsp.processFrame (0.0f, 0.0f, true, left, right);

        require (
            std::abs (left) < 0.0001f
                && std::abs (right) < 0.0001f,
            "Reset must clear Waveguide delay memory");
    }
}

void testWaveguideExtremeInputs()
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

    WaveguideDSP::Parameters p;
    p.feedback = 10.0f;
    p.resonance = 10.0f;
    p.mix = 10.0f;
    p.enabled = true;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    for (float input : inputs)
    {
        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame (input, input, true, left, right);

        require (
            std::isfinite (left) && std::isfinite (right),
            "Waveguide must never produce NaN or infinity");
    }
}

void testWaveguideParameterChanges()
{
    WaveguideDSP::Parameters p;
    p.mix = 10.0f;
    p.enabled = true;

    WaveguideDSP dsp;
    dsp.prepare (48000.0, p);

    p.rate = 10.0f;
    p.resonance = 10.0f;
    p.feedback = 10.0f;
    p.damping = 0.0f;
    p.character = 10.0f;

    dsp.setParameters (p);

    float left = 0.0f;
    float right = 0.0f;

    for (int i = 0; i < 12000; ++i)
    {
        const float input =
            0.2f * std::sin (
                2.0f * pi * 440.0f *
                static_cast<float> (i) / 48000.0f);

        dsp.processFrame (input, input, true, left, right);

        require (
            std::isfinite (left) && std::isfinite (right),
            "Waveguide parameter changes must remain finite");
    }
}
}

int main()
{
    testExactBypassPreservesHeadroom();
    testExactZeroMix();
    testLegacyDriveCurve();
    testModesAreDistinct();
    testToneDirection();
    testDistortionExtremeInputs();
    testDistortionMonoStereo();
    testModeTransitionRemainsFinite();

    testWaveguideExactBypass();
    testWaveguideImpulseDelay();
    testWaveguideStereoIsolation();
    testWaveguideReset();
    testWaveguideExtremeInputs();
    testWaveguideParameterChanges();

    std::cout << "All DSP tests passed.\n";
    return 0;
}
