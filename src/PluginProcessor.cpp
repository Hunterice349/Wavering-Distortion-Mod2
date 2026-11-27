
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

WaveringAudioProcessor::WaveringAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

WaveringAudioProcessor::~WaveringAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
WaveringAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "DISTORTION_DRIVE",
        "Distortion Drive",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "DISTORTION_TONE",
        "Distortion Tone",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "DISTORTION_MIX",
        "Distortion Mix",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        10.0f));

    layout.add (std::make_unique<juce::AudioParameterBool>(
        "DISTORTION_ENABLED",
        "Distortion Enabled",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_RATE",
        "Waveguide Rate",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_RESONANCE",
        "Waveguide Resonance",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_FEEDBACK",
        "Waveguide Feedback",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        4.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_DAMPING",
        "Waveguide Damping",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_CHARACTER",
        "Waveguide Character",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "WAVEGUIDE_MIX",
        "Waveguide Mix",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool>(
        "WAVEGUIDE_ENABLED",
        "Waveguide Enabled",
        false));

    return layout;
}

void WaveringAudioProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate =
        std::isfinite (sampleRate) && sampleRate > 1.0
            ? sampleRate
            : 44100.0;

    distortion.prepare (currentSampleRate, samplesPerBlock);
    waveguide.prepare (currentSampleRate, samplesPerBlock);
}

void WaveringAudioProcessor::releaseResources()
{
    distortion.reset();
    waveguide.reset();
}

double WaveringAudioProcessor::getTailLengthSeconds() const
{
    const auto parameters = readWaveguideParameters();

    if (! parameters.enabled || parameters.mix <= 0.0001f)
        return 0.0;

    const double rate =
        static_cast<double> (
            std::clamp (parameters.rate, 0.0f, 10.0f)) * 0.1;

    const double delaySeconds =
        0.002 * std::pow (60.0, rate);

    const double resonance =
        static_cast<double> (
            std::clamp (parameters.resonance, 0.0f, 10.0f)) * 0.1;

    const double feedback =
        0.94
        * (static_cast<double> (
            std::clamp (parameters.feedback, 0.0f, 10.0f)) * 0.1)
        * (0.12 + 0.88 * resonance);

    if (! std::isfinite (feedback) || feedback <= 0.0)
        return delaySeconds;

    constexpr double tailThreshold = 0.001;

    const double cycles =
        std::ceil (
            std::log (tailThreshold) / std::log (feedback));

    const double tailSeconds =
        delaySeconds * (1.0 + std::max (0.0, cycles));

    return std::isfinite (tailSeconds)
        ? std::clamp (tailSeconds, 0.0, 30.0)
        : 30.0;
}

void WaveringAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    juce::ignoreUnused (midiMessages);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    const auto distortionParameters = readDistortionParameters();
    const auto waveguideParameters = readWaveguideParameters();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = buffer.getSample (0, sample);
        float right = numChannels > 1
            ? buffer.getSample (1, sample)
            : left;

        distortion.processFrame (
            left,
            right,
            distortionParameters);

        waveguide.processFrame (
            left,
            right,
            waveguideParameters);

        buffer.setSample (0, sample, left);

        if (numChannels > 1)
            buffer.setSample (1, sample, right);
    }

    for (int channel = 2; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < numSamples; ++sample)
            channelData[sample] = 0.0f;
    }
}

juce::AudioProcessorEditor*
WaveringAudioProcessor::createEditor()
{
    return new WaveringAudioProcessorEditor (*this);
}

bool WaveringAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String WaveringAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WaveringAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool WaveringAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool WaveringAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double WaveringAudioProcessor::getProgramLengthInSeconds() const
{
    return 0.0;
}

int WaveringAudioProcessor::getNumPrograms()
{
    return 1;
}

int WaveringAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WaveringAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String WaveringAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void WaveringAudioProcessor::changeProgramName (
    int index,
    const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void WaveringAudioProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    std::unique_ptr<juce::XmlElement> xml (
        state.createXml());

    copyXmlToBinary (*xml, destData);
}

void WaveringAudioProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (
        getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr
        && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (
            juce::ValueTree::fromXml (*xml));
    }
}

WaveringAudioProcessor::DistortionParameters
WaveringAudioProcessor::readDistortionParameters() const
{
    DistortionParameters parameters;

    parameters.drive =
        apvts.getRawParameterValue ("DISTORTION_DRIVE")->load();

    parameters.tone =
        apvts.getRawParameterValue ("DISTORTION_TONE")->load();

    parameters.mix =
        apvts.getRawParameterValue ("DISTORTION_MIX")->load();

    parameters.enabled =
        apvts.getRawParameterValue ("DISTORTION_ENABLED")->load() > 0.5f;

    return parameters;
}

WaveguideDSP::Parameters
WaveringAudioProcessor::readWaveguideParameters() const
{
    WaveguideDSP::Parameters parameters;

    parameters.rate =
        apvts.getRawParameterValue ("WAVEGUIDE_RATE")->load();

    parameters.resonance =
        apvts.getRawParameterValue ("WAVEGUIDE_RESONANCE")->load();

    parameters.feedback =
        apvts.getRawParameterValue ("WAVEGUIDE_FEEDBACK")->load();

    parameters.damping =
        apvts.getRawParameterValue ("WAVEGUIDE_DAMPING")->load();

    parameters.character =
        apvts.getRawParameterValue ("WAVEGUIDE_CHARACTER")->load();

    parameters.mix =
        apvts.getRawParameterValue ("WAVEGUIDE_MIX")->load();

    parameters.enabled =
        apvts.getRawParameterValue ("WAVEGUIDE_ENABLED")->load() > 0.5f;

    return parameters;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveringAudioProcessor();
}