#include "DistortionDSP.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr std::array<double, 6> sampleRates { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };
constexpr std::array<int, 6> frameBatchSizes { 32, 64, 128, 256, 512, 1024 };
constexpr double pi = 3.14159265358979323846;
int failures = 0;

struct Metrics
{
    double sum = 0.0;
    double sumSquares = 0.0;
    float peak = 0.0f;
    std::size_t count = 0;
    std::size_t nonFinite = 0;
    std::size_t nearFullScale = 0;

    void add (float sample)
    {
        ++count;
        if (! std::isfinite (sample))
        {
            ++nonFinite;
            return;
        }
        sum += sample;
        sumSquares += static_cast<double> (sample) * sample;
        peak = std::max (peak, std::abs (sample));
        if (std::abs (sample) >= 0.999f)
            ++nearFullScale;
    }

    double dc() const
    {
        const auto validCount = count - nonFinite;
        return validCount == 0 ? 0.0 : sum / static_cast<double> (validCount);
    }

    double rms() const
    {
        const auto validCount = count - nonFinite;
        return validCount == 0 ? 0.0 : std::sqrt (sumSquares / static_cast<double> (validCount));
    }
};

void require (bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

float inputFor (const std::string& signal, std::size_t sample, double sampleRate,
                std::mt19937& random, std::uniform_real_distribution<float>& noise)
{
    if (signal == "silence") return 0.0f;
    if (signal == "quiet") return 0.0001f * std::sin (2.0 * pi * 997.0 * sample / sampleRate);
    if (signal == "nominal") return 0.35f * std::sin (2.0 * pi * 997.0 * sample / sampleRate);
    if (signal == "strong") return 1.5f * std::sin (2.0 * pi * 997.0 * sample / sampleRate);
    if (signal == "white_noise") return 0.25f * noise (random);
    if (signal == "impulse") return sample == 0 ? 0.8f : 0.0f;
    return 0.2f; // DC offset
}

void writeMetricsRow (std::ofstream& csv, const std::string& recordType, const std::string& signal,
                      double sampleRate, int frameBatchSize, bool stereo,
                      const DistortionDSP::Parameters& parameters, const Metrics& metrics)
{
    csv << recordType << ',' << signal << ',' << static_cast<int> (sampleRate) << ','
        << frameBatchSize << ',' << (stereo ? "stereo" : "mono") << ','
        << parameters.drive << ',' << parameters.tone << ',' << parameters.bias << ','
        << parameters.body << ',' << parameters.mix << ',' << std::setprecision (10)
        << metrics.dc() << ',' << metrics.rms() << ',' << metrics.peak << ','
        << metrics.nonFinite << ',' << metrics.nearFullScale << ",,,\n";
}

void testSignalAndFrameBatchMatrix (std::ofstream& csv)
{
    const std::array<std::string, 7> signals {
        "silence", "quiet", "nominal", "strong", "white_noise", "impulse", "dc_offset"
    };
    std::size_t cases = 0;
    for (const auto sampleRate : sampleRates)
        for (const auto frameBatchSize : frameBatchSizes)
            for (const bool stereo : { false, true })
                for (const auto& signal : signals)
                {
                    DistortionDSP::Parameters p;
                    p.drive = 5.0f;
                    p.tone = 5.0f;
                    p.bias = 5.0f;
                    p.body = 5.0f;
                    p.mix = 10.0f;
                    p.enabled = true;
                    DistortionDSP dsp;
                    dsp.prepare (sampleRate, p);
                    Metrics left, right;
                    std::mt19937 random (0xD1570u);
                    std::uniform_real_distribution<float> noise (-1.0f, 1.0f);
                    constexpr int batchesToProcess = 4;
                    for (int batch = 0; batch < batchesToProcess; ++batch)
                        for (int offset = 0; offset < frameBatchSize; ++offset)
                        {
                            const auto index = static_cast<std::size_t> (batch * frameBatchSize + offset);
                            const float input = inputFor (signal, index, sampleRate, random, noise);
                            float outLeft = 0.0f, outRight = 0.0f;
                            dsp.processFrame (input, stereo ? -0.5f * input : 0.0f,
                                              stereo, outLeft, outRight);
                            left.add (outLeft);
                            right.add (outRight);
                        }

                    const std::string label = signal + ", " + std::to_string (static_cast<int> (sampleRate))
                                            + " Hz, frame batch " + std::to_string (frameBatchSize)
                                            + (stereo ? ", stereo" : ", mono");
                    require (left.nonFinite == 0 && right.nonFinite == 0, label + ": all outputs must be finite");
                    if (signal == "silence")
                        require (left.peak == 0.0f && right.peak == 0.0f, label + ": silence must remain silent");
                    if (! stereo)
                        require (right.peak == 0.0f, label + ": mono right output must be zero");
                    if (signal != "silence")
                        require (left.rms() > 0.0, label + ": non-silent input must produce measurable RMS");
                    writeMetricsRow (csv, "signal", signal, sampleRate, frameBatchSize, stereo, p, left);
                    ++cases;
                }
    std::cout << "Signal/configuration cases: " << cases
              << " (frame batches group calls to processFrame; they do not exercise a block-processing API)\n";
}

void testDryPathAndEnable()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.tone = 0.0f;
    p.bias = 10.0f;
    p.body = 10.0f;
    p.mix = 0.0f;
    p.enabled = true;
    DistortionDSP dsp;
    dsp.prepare (48000.0, p);
    for (const float input : { -2.0f, -0.25f, 0.0f, 0.25f, 2.0f })
    {
        float left = 0.0f, right = 0.0f;
        dsp.processFrame (input, -input, true, left, right);
        require (left == input && right == -input, "Mix=0 must preserve the exact stereo dry path");
    }

    p.enabled = false;
    p.mix = 10.0f;
    DistortionDSP switched;
    switched.prepare (48000.0, p);
    for (int i = 0; i < 1000; ++i)
    {
        float left = 0.0f, right = 0.0f;
        switched.processFrame (0.25f, -0.5f, true, left, right);
        require (left == 0.25f && right == -0.5f, "disabled distortion must be bit-transparent");
    }
    p.enabled = true;
    p.drive = 10.0f;
    switched.setParameters (p);
    float left = 0.0f, right = 0.0f;
    for (int i = 0; i < 1200; ++i)
        switched.processFrame (0.75f, 0.75f, false, left, right);
    require (std::isfinite (left) && left != 0.75f, "Enable ON must ramp to processed signal");
    require (right == 0.0f, "mono processing must clear the right output");

    p.enabled = false;
    switched.setParameters (p);
    for (int i = 0; i < 1200; ++i)
        switched.processFrame (0.75f, 0.75f, false, left, right);
    require (left == 0.75f && right == 0.0f, "Enable OFF must ramp back to dry signal");
}

void testNonFiniteInputs()
{
    DistortionDSP::Parameters p;
    p.drive = 10.0f;
    p.mix = 10.0f;
    p.enabled = true;
    DistortionDSP dsp;
    dsp.prepare (48000.0, p);
    const std::array<float, 3> inputs {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };
    for (const float input : inputs)
    {
        float left = 1.0f, right = 1.0f;
        dsp.processFrame (input, input, true, left, right);
        require (std::isfinite (left) && std::isfinite (right), "NaN/Inf input must not propagate to wet output");
    }
}

void testParameterExtremes (std::ofstream& csv)
{
    constexpr std::array<float, 3> points { 0.0f, 5.0f, 10.0f };
    std::mt19937 random (0x51A7u);
    std::uniform_real_distribution<float> noise (-1.0f, 1.0f);
    std::size_t combinations = 0;
    for (const float drive : points)
        for (const float tone : points)
            for (const float bias : points)
                for (const float body : points)
                    for (const float mix : points)
                    {
                        DistortionDSP::Parameters p { drive, tone, bias, body, mix, true };
                        DistortionDSP dsp;
                        dsp.prepare (96000.0, p);
                        Metrics output;
                        for (int i = 0; i < 4096; ++i)
                        {
                            const float input = noise (random) * 0.8f;
                            float left = 0.0f, right = 0.0f;
                            dsp.processFrame (input, -input, true, left, right);
                            output.add (left);
                            output.add (right);
                        }
                        require (output.nonFinite == 0, "parameter combination must produce finite output");
                        writeMetricsRow (csv, "parameters", "all_controls", 96000.0, 0, true, p, output);
                        ++combinations;
                    }
    std::cout << "Drive/Tone/Bias/Body/Mix combinations (0, 5, 10): " << combinations << '\n';
}

void benchmarkCpu (std::ofstream& csv)
{
    constexpr int frames = 48000;
    for (const bool stereo : { false, true })
        for (const float drive : { 0.0f, 5.0f, 10.0f })
        {
            DistortionDSP::Parameters p;
            p.drive = drive;
            p.tone = 5.0f;
            p.bias = 5.0f;
            p.body = 5.0f;
            p.mix = 10.0f;
            p.enabled = true;
            DistortionDSP dsp;
            dsp.prepare (48000.0, p);
            volatile float sink = 0.0f;
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < frames; ++i)
            {
                const float input = 0.35f * std::sin (2.0 * pi * 997.0 * i / 48000.0);
                float left = 0.0f, right = 0.0f;
                dsp.processFrame (input, stereo ? -input : 0.0f, stereo, left, right);
                sink = sink + left + right;
            }
            const auto elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
            const double nsPerFrame = elapsed * 1.0e9 / frames;
            const double realtimePercent = elapsed / (static_cast<double> (frames) / 48000.0) * 100.0;
            csv << "cpu_approx,synthetic_sine," << 48000 << ",0," << (stereo ? "stereo" : "mono")
                << ',' << p.drive << ',' << p.tone << ',' << p.bias << ',' << p.body << ',' << p.mix
                << ",,,," << std::setprecision (10) << nsPerFrame << ',' << realtimePercent
                << ",includes_signal_generation_and_loop_overhead\n";
            std::cout << "Approximate CPU benchmark (includes sine generation and loop overhead): "
                      << (stereo ? "stereo" : "mono") << ", drive=" << drive
                      << ", ns/frame=" << nsPerFrame << ", realtime=" << realtimePercent << "%\n";
            (void) sink;
        }
}
}

int main (int argc, char** argv)
{
    const std::string metricsPath = argc > 1 ? argv[1] : "distortion_metrics.csv";
    std::ofstream csv (metricsPath);
    if (! csv)
    {
        std::cerr << "Unable to create metrics file: " << metricsPath << '\n';
        return 2;
    }
    csv << "record_type,signal,sample_rate_hz,frame_batch_size,channels,drive,tone,bias,body,mix,dc_offset,rms,peak,non_finite_samples,near_full_scale_samples,cpu_ns_per_frame,realtime_percent,benchmark_note\n";
    testSignalAndFrameBatchMatrix (csv);
    testDryPathAndEnable();
    testNonFiniteInputs();
    testParameterExtremes (csv);
    benchmarkCpu (csv);
    csv.close();
    if (failures != 0)
    {
        std::cerr << "Distortion DSP: " << failures << " required assertion(s) failed.\n";
        return 1;
    }
    std::cout << "All required Distortion DSP checks passed. Metrics written to " << metricsPath << '\n';
    return 0;
}