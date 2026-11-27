
#include "../src/DistortionDSP.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>

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
}

int main(int argc, char* argv[])
{
    testNonFiniteInputs();

    std::cout << "DSP tests completed." << std::endl;

    return 0;
}
