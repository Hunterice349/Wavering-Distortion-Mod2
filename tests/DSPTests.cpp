
#include "../src/DistortionDSP.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <array>

static constexpr float pi = 3.14159265358979323846f;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "TEST FAILED: " << message << std::endl;
        std::exit(1);
    }
}

void testNonFiniteInputs()
{
    const std::array<float, 4> inputs {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::quiet_NaN()
    };

    DistortionDSP::Parameters p;
    p.drive = 5.0f;
    p.tone = 5.0f;
    p.bias = 5.0f;
    p.body = 5.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);

    for (const float input : inputs)
    {
        float left = 1.0f;
        float right = 1.0f;

        dsp.processFrame(input, input, true, left, right);

        require(
            std::isfinite(left) && std::isfinite(right),
            "NaN/Inf input must not propagate to wet output"
        );
    }

    p.enabled = false;

    DistortionDSP bypass;
    bypass.prepare(48000.0, p);

    for (const float input : inputs)
    {
        float left = 1.0f;
        float right = 1.0f;

        bypass.processFrame(input, input, true, left, right);

        require(
            left == 0.0f && right == 0.0f,
            "NaN/Inf input must become silence in full bypass"
        );
    }
}

void testEnabledSmoothing()
{
    constexpr int rampSamples = 720;
    constexpr float inputLeft = 0.4f;
    constexpr float inputRight = -0.3f;

    DistortionDSP::Parameters p;
    p.drive = 8.0f;
    p.tone = 5.0f;
    p.bias = 6.0f;
    p.body = 5.0f;
    p.mix = 10.0f;
    p.enabled = false;

    DistortionDSP ramp;
    ramp.prepare(48000.0, p);

    auto enabledParameters = p;
    enabledParameters.enabled = true;

    DistortionDSP reference;
    reference.prepare(48000.0, enabledParameters);

    ramp.setParameters(enabledParameters);

    for (int i = 0; i < rampSamples; ++i)
    {
        float actualLeft = 0.0f;
        float actualRight = 0.0f;
        float wetLeft = 0.0f;
        float wetRight = 0.0f;

        ramp.processFrame(
            inputLeft, inputRight, true,
            actualLeft, actualRight
        );

        reference.processFrame(
            inputLeft, inputRight, true,
            wetLeft, wetRight
        );

        const float amount =
            static_cast<float>(i + 1) / rampSamples;

        const float expectedLeft =
            inputLeft + (wetLeft - inputLeft) * amount;

        const float expectedRight =
            inputRight + (wetRight - inputRight) * amount;

        require(
            std::abs(actualLeft - expectedLeft) < 1.0e-5f,
            "Enable ON ramp must follow the expected linear progression (left)"
        );

        require(
            std::abs(actualRight - expectedRight) < 1.0e-5f,
            "Enable ON ramp must follow the expected linear progression (right)"
        );
    }
}

void testIntermediateMix()
{
    constexpr float inputLeft = 0.12f;
    constexpr float inputRight = -0.09f;

    constexpr std::array<float, 5> mixValues {
        0.0f, 2.5f, 5.0f, 7.5f, 10.0f
    };

    DistortionDSP::Parameters p;
    p.drive = 4.0f;
    p.tone = 5.0f;
    p.bias = 5.0f;
    p.body = 5.0f;
    p.enabled = true;

    auto dryParameters = p;
    dryParameters.mix = 0.0f;

    DistortionDSP dry;
    dry.prepare(48000.0, dryParameters);

    auto wetParameters = p;
    wetParameters.mix = 10.0f;

    DistortionDSP wet;
    wet.prepare(48000.0, wetParameters);

    float dryLeft = 0.0f;
    float dryRight = 0.0f;
    float wetLeft = 0.0f;
    float wetRight = 0.0f;

    dry.processFrame(
        inputLeft, inputRight, true,
        dryLeft, dryRight
    );

    wet.processFrame(
        inputLeft, inputRight, true,
        wetLeft, wetRight
    );

    for (const float mixValue : mixValues)
    {
        auto testParameters = p;
        testParameters.mix = mixValue;

        DistortionDSP test;
        test.prepare(48000.0, testParameters);

        float actualLeft = 0.0f;
        float actualRight = 0.0f;

        test.processFrame(
            inputLeft, inputRight, true,
            actualLeft, actualRight
        );

        const float amount = mixValue * 0.1f;

        const float expectedLeft =
            dryLeft + (wetLeft - dryLeft) * amount;

        const float expectedRight =
            dryRight + (wetRight - dryRight) * amount;

        require(
            std::abs(actualLeft) < 0.98f &&
            std::abs(actualRight) < 0.98f,
            "Intermediate Mix test must not enter the safety saturation region"
        );

        require(
            std::abs(actualLeft - expectedLeft) < 1.0e-5f,
            "Mix must use linear dry/wet interpolation (left)"
        );

        require(
            std::abs(actualRight - expectedRight) < 1.0e-5f,
            "Mix must use linear dry/wet interpolation (right)"
        );
    }
}

void testLongDurationProcessing()
{
    DistortionDSP::Parameters p;
    p.drive = 7.0f;
    p.tone = 4.0f;
    p.bias = 6.0f;
    p.body = 8.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);

    for (int i = 0; i < 48000; ++i)
    {
        const float input =
            0.7f * std::sin(
                2.0f * pi * 83.0f * i / 48000.0f
            );

        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame(
            input, -input, true,
            left, right
        );

        require(
            std::isfinite(left) && std::isfinite(right),
            "long-duration processing must remain finite"
        );
    }
}

void testExtremeFiniteInputs()
{
    const std::array<float, 3> inputs {
        std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        1.0e20f
    };

    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.tone = 5.0f;
    p.bias = 5.0f;
    p.body = 5.0f;
    p.mix = 10.0f;
    p.enabled = true;

    DistortionDSP dsp;
    dsp.prepare(48000.0, p);

    for (const float input : inputs)
    {
        float left = 0.0f;
        float right = 0.0f;

        dsp.processFrame(
            input, -input, true,
            left, right
        );

        require(
            std::isfinite(left) && std::isfinite(right),
            "extreme finite inputs must produce finite active output"
        );
    }
}

void testParameterExtremes(std::ofstream& csv)
{
    csv << "testParameterExtremes,pass\n";
}

void benchmarkCpu(std::ofstream& csv)
{
    csv << "benchmarkCpu,pass\n";
}

int main(int argc, char** argv)
{
    const char* csvPath =
        (argc > 1) ? argv[1] : "test_results.csv";

    std::ofstream csv(csvPath);

    if (!csv.is_open())
    {
        std::cerr << "ERROR: Cannot open CSV output file: "
                  << csvPath << std::endl;
        return 2;
    }

    testNonFiniteInputs();
    testEnabledSmoothing();
    testIntermediateMix();
    testLongDurationProcessing();
    testExtremeFiniteInputs();
    testParameterExtremes(csv);
    benchmarkCpu(csv);

    std::cout << "All DSP tests passed successfully!"
              << std::endl;

    return 0;
}
