#include "PluginProcessor.h"
#include "PluginEditor.h"

WaveringAudioProcessor::WaveringAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

WaveringAudioProcessor::~WaveringAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout WaveringAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive", juce::NormalisableRange<float> (0.0f, 100.0f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone", juce::NormalisableRange<float> (2000.0f, 18000.0f), 8000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "wave", 1 }, "Wave Freq", juce::NormalisableRange<float> (40.0f, 4000.0f), 220.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "res", 1 }, "Resonance", juce::NormalisableRange<float> (0.0f, 0.97f), 0.6f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ring", 1 }, "Ring Freq", juce::NormalisableRange<float> (0.0f, 2000.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Ring Depth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out", 1 }, "Output", juce::NormalisableRange<float> (-40.0f, 12.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

bool WaveringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (in != out)
        return false;

    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void WaveringAudioProcessor::prepareToPlay (double sampleRate, int)
{
    // Allocate and reset DSP storage only on the host's preparation thread, never in processBlock().
    sr = std::isfinite (sampleRate) && sampleRate >= 8000.0
       ? juce::jmin (sampleRate, 384000.0) : 44100.0;
    maxDelaySamples = static_cast<int> (sr) + 2;
    waveBufL.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
    waveBufR.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
    waveWrite = 0;
    waveDelayLen = 1;
    waveZ1L = waveZ1R = toneZ1L = toneZ1R = 0.0f;
    ringPhase = 0.0;

    const auto readParameter = [this] (const char* id, float fallback)
    {
        const auto* value = apvts.getRawParameterValue (id);
        return value != nullptr ? value->load (std::memory_order_relaxed) : fallback;
    };
    smDrive = readParameter ("drive", 20.0f);
    smTone  = readParameter ("tone", 8000.0f);
    smWave  = readParameter ("wave", 220.0f);
    smRes   = readParameter ("res", 0.6f);
    smRing  = readParameter ("ring", 0.0f);
    smDepth = readParameter ("depth", 0.5f);
    smMix   = readParameter ("mix", 0.5f);
    smOut   = readParameter ("out", 0.0f);
    smBypass = readParameter ("bypass", 0.0f) > 0.5f;
}

void WaveringAudioProcessor::releaseResources()
{
    waveBufL.clear();
    waveBufR.clear();
    maxDelaySamples = 0;
    waveWrite = 0;
    waveDelayLen = 1;
}

void WaveringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const int numInputCh = juce::jmin (getTotalNumInputChannels(), numChannels);
    const int numOutputCh = juce::jmin (getTotalNumOutputChannels(), numChannels);

    for (int ch = numInputCh; ch < numOutputCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples <= 0 || numOutputCh <= 0)
        return;

    // A host must call prepareToPlay before processing. If it does not, return silence
    // instead of allocating or mutating DSP state on the real-time audio thread.
    if (maxDelaySamples < 2 || waveBufL.size() < static_cast<size_t> (maxDelaySamples)
                            || waveBufR.size() < static_cast<size_t> (maxDelaySamples))
    {
        for (int ch = 0; ch < numOutputCh; ++ch)
            buffer.clear (ch, 0, numSamples);
        return;
    }

    const auto readParameter = [this] (const char* id, float fallback)
    {
        const auto* value = apvts.getRawParameterValue (id);
        return value != nullptr ? value->load (std::memory_order_relaxed) : fallback;
    };
    const float rawDrive = readParameter ("drive", 20.0f);
    const float rawTone  = readParameter ("tone", 8000.0f);
    const float rawWave  = readParameter ("wave", 220.0f);
    const float rawRes   = readParameter ("res", 0.6f);
    const float rawRing  = readParameter ("ring", 0.0f);
    const float rawDepth = readParameter ("depth", 0.5f);
    const float rawMixP  = readParameter ("mix", 0.5f);
    const float rawOutP  = readParameter ("out", 0.0f);
    const bool rawByp    = readParameter ("bypass", 0.0f) > 0.5f;

    const float safeSampleRate = static_cast<float> (juce::jmax (sr, 1.0));
    const float k = 1.0f - std::exp (-1.0f / (0.01f * safeSampleRate));
    smDrive += (rawDrive - smDrive) * k;
    smTone  += (rawTone  - smTone)  * k;
    smWave  += (rawWave  - smWave)  * k;
    smRes   += (rawRes   - smRes)   * k;
    smRing  += (rawRing  - smRing)  * k;
    smDepth += (rawDepth - smDepth) * k;
    smMix   += (rawMixP  - smMix)   * k;
    smOut   += (rawOutP  - smOut)   * k;
    smBypass = rawByp;

    const int d = juce::jlimit (1, maxDelaySamples - 1,
                    static_cast<int> (std::lround (sr / static_cast<double> (juce::jmax (20.0f, smWave)))));
    waveDelayLen = d;

    const float pi = juce::MathConstants<float>::pi;
    const float toneHz = juce::jlimit (0.0f, safeSampleRate * 0.45f, smTone);
    const float aTone = 1.0f - std::exp (-2.0f * pi * toneHz / safeSampleRate);
    const float aWg = 0.45f;
    const float res = smRes;
    const float mix = smBypass ? 0.0f : smMix;
    const float depth = smDepth;
    const float dg = 1.0f + smDrive * 0.09f;
    const float gain = std::pow (10.0f, smOut / 20.0f);
    const float ringHz = juce::jlimit (0.0f, safeSampleRate * 0.5f, smRing);
    const float ringInc = ringHz / safeSampleRate;

    float* dl = buffer.getWritePointer (0);
    float* dr = numOutputCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float peak = 0.0f;
    int w = waveWrite;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = dl[i];
        const float r = dr != nullptr ? dr[i] : l;
        float wl = std::tanh (l * dg);
        float wr = std::tanh (r * dg);

        toneZ1L += aTone * (wl - toneZ1L); wl = toneZ1L;
        toneZ1R += aTone * (wr - toneZ1R); wr = toneZ1R;

        const int ri = w - waveDelayLen >= 0 ? w - waveDelayLen : w - waveDelayLen + maxDelaySamples;
        float yl = waveBufL[static_cast<size_t> (ri)];
        float yr = waveBufR[static_cast<size_t> (ri)];
        waveZ1L += aWg * (yl - waveZ1L);
        waveZ1R += aWg * (yr - waveZ1R);
        waveBufL[static_cast<size_t> (w)] = wl + waveZ1L * res;
        waveBufR[static_cast<size_t> (w)] = wr + waveZ1R * res;

        yl = waveZ1L;
        yr = waveZ1R;
        ringPhase += ringInc;
        if (ringPhase >= 1.0)
            ringPhase -= 1.0;
        const float mod = 1.0f - depth + depth * std::sin (2.0f * pi * static_cast<float> (ringPhase));
        yl *= mod;
        yr *= mod;

        const float oL = (l * (1.0f - mix) + yl * mix) * gain;
        const float oR = (r * (1.0f - mix) + yr * mix) * gain;
        dl[i] = oL;
        if (dr != nullptr)
            dr[i] = oR;

        peak = juce::jmax (peak, std::abs (oL), std::abs (oR));
        if (++w >= maxDelaySamples)
            w = 0;
    }

    waveWrite = w;
    ringFreqHz.store (smRing, std::memory_order_relaxed);
    const float db = peak > 1.0e-6f ? 20.0f * std::log10 (peak) : -80.0f;
    outputLevelDb.store (db, std::memory_order_relaxed);
}

void WaveringAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    juce::MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void WaveringAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Reject empty, null, unreasonable, and malformed host data before constructing a stream.
    constexpr int maxStateSize = 16 * 1024 * 1024;
    if (data == nullptr || sizeInBytes <= 0 || sizeInBytes > maxStateSize)
        return;

    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);
    const auto state = juce::ValueTree::readFromStream (stream);
    if (state.isValid() && state.hasType (juce::Identifier ("Parameters")))
        apvts.replaceState (state);
}

juce::AudioProcessorEditor* WaveringAudioProcessor::createEditor()
{
    return new WaveringAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveringAudioProcessor();
}
