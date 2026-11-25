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
        juce::ParameterID { "drive", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Drive")
            .withRange (0.0f, 100.0f)
            .withDefaultValue (20.0f)));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Tone")
            .withRange (2000.0f, 18000.0f)
            .withDefaultValue (8000.0f)
            .withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "wave", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Wave Freq")
            .withRange (40.0f, 4000.0f)
            .withDefaultValue (220.0f)
            .withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "res", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Resonance")
            .withRange (0.0f, 0.97f)
            .withDefaultValue (0.6f)));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ring", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Ring Freq")
            .withRange (0.0f, 2000.0f)
            .withDefaultValue (0.0f)
            .withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Ring Depth")
            .withRange (0.0f, 1.0f)
            .withDefaultValue (0.5f)));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Mix")
            .withRange (0.0f, 1.0f)
            .withDefaultValue (0.5f)));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out", 1 },
        juce::AudioParameterFloat::Attributes()
            .withName ("Output")
            .withRange (-40.0f, 12.0f)
            .withDefaultValue (0.0f)
            .withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 },
        juce::AudioParameterBool::Attributes()
            .withName ("Bypass")
            .withDefaultValue (false)));

    return { params.begin(), params.end() };
}

bool WaveringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void WaveringAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxDelaySamples = static_cast<int> (sr) + 2;
    waveBufL.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
    waveBufR.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
    waveWrite = 0;
    waveZ1L = waveZ1R = toneZ1L = toneZ1R = 0.0f;
    ringPhase = 0.0;
}

void WaveringAudioProcessor::releaseResources()
{
    waveBufL.clear();
    waveBufR.clear();
    maxDelaySamples = 0;
}

void WaveringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numInputCh  = getTotalNumInputChannels();
    const int numOutputCh = getTotalNumOutputChannels();

    for (int ch = numInputCh; ch < numOutputCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (waveBufL.empty())
        prepareToPlay (getSampleRate() > 0.0 ? getSampleRate() : 44100.0, numSamples);

    const float rawDrive = *apvts.getRawParameterValue ("drive");
    const float rawTone  = *apvts.getRawParameterValue ("tone");
    const float rawWave  = *apvts.getRawParameterValue ("wave");
    const float rawRes   = *apvts.getRawParameterValue ("res");
    const float rawRing  = *apvts.getRawParameterValue ("ring");
    const float rawDepth = *apvts.getRawParameterValue ("depth");
    const float rawMixP  = *apvts.getRawParameterValue ("mix");
    const float rawOutP  = *apvts.getRawParameterValue ("out");
    const bool  rawByp   = *apvts.getRawParameterValue ("bypass") > 0.5f;

    const float k = 1.0f - std::exp (-1.0f / (0.01f * static_cast<float> (sr)));
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

    const float pi      = juce::MathConstants<float>::pi;
    const float aTone   = 1.0f - std::exp (-2.0f * pi * smTone / static_cast<float> (sr));
    const float aWg     = 0.45f;
    const float res     = smRes;
    const float mix     = smBypass ? 0.0f : smMix;
    const float depth   = smDepth;
    const float dg      = 1.0f + smDrive * 0.09f;
    const float gain    = std::pow (10.0f, smOut / 20.0f);
    const float ringInc = static_cast<float> (smRing / sr);

    float* dl = buffer.getWritePointer (0);
    float* dr = numOutputCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float peak = 0.0f;
    int w = waveWrite;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = dl[i];
        const float r = (dr != nullptr) ? dr[i] : l;

        float wl = std::tanh (l * dg);
        float wr = std::tanh (r * dg);

        toneZ1L += aTone * (wl - toneZ1L);  wl = toneZ1L;
        toneZ1R += aTone * (wr - toneZ1R);  wr = toneZ1R;

        const int ri = (w - waveDelayLen >= 0) ? (w - waveDelayLen)
                                               : (w - waveDelayLen + maxDelaySamples);
        float yl = waveBufL[ri];
        float yr = waveBufR[ri];

        waveZ1L += aWg * (yl - waveZ1L);
        waveZ1R += aWg * (yr - waveZ1R);

        waveBufL[w] = wl + waveZ1L * res;
        waveBufR[w] = wr + waveZ1R * res;

        yl = waveZ1L;
        yr = waveZ1R;

        ringPhase += ringInc;
        if (ringPhase >= 1.0) ringPhase -= 1.0;
        const float mod = 1.0f - depth + depth * std::sin (2.0f * pi * static_cast<float> (ringPhase));
        yl *= mod;
        yr *= mod;

        float oL = (l * (1.0f - mix) + yl * mix) * gain;
        float oR = (r * (1.0f - mix) + yr * mix) * gain;

        dl[i] = oL;
        if (dr != nullptr) dr[i] = oR;

        peak = juce::jmax (peak, std::abs (oL), std::abs (oR));

        if (++w >= maxDelaySamples) w = 0;
    }

    waveWrite = w;

    ringFreqHz.store (smRing);
    const float db = peak > 1.0e-6f ? 20.0f * std::log10 (peak) : -80.0f;
    outputLevelDb.store (db);
}

void WaveringAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::MemoryOutputStream> mos (new juce::MemoryOutputStream (destData, true));
    state.writeToStream (*mos);
}

void WaveringAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::MemoryInputStream> mis (new juce::MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false));
    const juce::ValueTree state = juce::ValueTree::readFromStream (*mis);
    if (state.isValid())
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
