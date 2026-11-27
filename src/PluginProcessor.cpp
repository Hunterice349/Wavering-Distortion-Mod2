#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <memory>

WaveringAudioProcessor::WaveringAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
WaveringAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // DISTORTION

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "tone", "Tone",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "bias", "Bias",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "body", "Body",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        10.0f));

    layout.add (std::make_unique<juce::AudioParameterBool>(
        "distortionEnabled", "Distortion Enabled",
        true));

    // WAVEGUIDE

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveRate", "Waveguide Rate",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveResonance", "Waveguide Resonance",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveFeedback", "Waveguide Feedback",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        4.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveDamping", "Waveguide Damping",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveCharacter", "Waveguide Character",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "waveMix", "Waveguide Mix",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool>(
        "waveguideEnabled", "Waveguide Enabled",
        false));

    return layout;
}

//==============================================================================

void WaveringAudioProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate =
        std::isfinite (sampleRate) && sampleRate > 1000.0
            ? std::clamp (sampleRate, 8000.0, 384000.0)
            : 44100.0;

    const auto distortionParameters = readDistortionParameters();
    const auto waveguideParameters = readWaveguideParameters();

    distortion.prepare (
        currentSampleRate,
        distortionParameters);

    waveguide.prepare (
        currentSampleRate,
        waveguideParameters);
}

//==============================================================================

void WaveringAudioProcessor::releaseResources()
{
    distortion.reset();
    waveguide.reset();
}

//==============================================================================

bool WaveringAudioProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect

    juce::ignoreUnused (layouts);
    return true;

#else

    const auto outputChannels =
        layouts.getMainOutputChannelSet();

    if (outputChannels != juce::AudioChannelSet::mono()
        && outputChannels != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth

    if (outputChannels != layouts.getMainInputChannelSet())
        return false;

#endif

    return true;

#endif
}

//==============================================================================

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

//==============================================================================

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

    const bool stereo = numChannels > 1;

    const auto distortionParameters = readDistortionParameters();
    const auto waveguideParameters = readWaveguideParameters();

    distortion.setParameters (distortionParameters);
    waveguide.setParameters (waveguideParameters);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float inputLeft =
            buffer.getSample (0, sample);

        const float inputRight = stereo
            ? buffer.getSample (1, sample)
            : inputLeft;

        float distortionLeft = 0.0f;
        float distortionRight = 0.0f;

        distortion.processFrame (
            inputLeft,
            inputRight,
            stereo,
            distortionLeft,
            distortionRight);

        float outputLeft = 0.0f;
        float outputRight = 0.0f;

        waveguide.processFrame (
            distortionLeft,
            distortionRight,
            stereo,
            outputLeft,
            outputRight);

        buffer.setSample (0, sample, outputLeft);

        if (stereo)
            buffer.setSample (1, sample, outputRight);
    }

    // Clear any unexpected additional output channels.
    for (int channel = 2; channel < numChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

juce::AudioProcessorEditor*
WaveringAudioProcessor::createEditor()
{
    return new WaveringAudioProcessorEditor (*this);
}

//==============================================================================

void WaveringAudioProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();

    std::unique_ptr<juce::XmlElement> xml (
        state.createXml());

    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

//==============================================================================

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

//==============================================================================

DistortionDSP::Parameters
WaveringAudioProcessor::readDistortionParameters() const noexcept
{
    DistortionDSP::Parameters parameters;

    parameters.drive =
        apvts.getRawParameterValue ("drive")->load();

    parameters.tone =
        apvts.getRawParameterValue ("tone")->load();

    parameters.bias =
        apvts.getRawParameterValue ("bias")->load();

    parameters.body =
        apvts.getRawParameterValue ("body")->load();

    parameters.mix =
        apvts.getRawParameterValue ("mix")->load();

    parameters.enabled =
        apvts.getRawParameterValue ("distortionEnabled")->load() > 0.5f;

    return parameters;
}

//==============================================================================

WaveguideDSP::Parameters
WaveringAudioProcessor::readWaveguideParameters() const noexcept
{
    WaveguideDSP::Parameters parameters;

    parameters.rate =
        apvts.getRawParameterValue ("waveRate")->load();

    parameters.resonance =
        apvts.getRawParameterValue ("waveResonance")->load();

    parameters.feedback =
        apvts.getRawParameterValue ("waveFeedback")->load();

    parameters.damping =
        apvts.getRawParameterValue ("waveDamping")->load();

    parameters.character =
        apvts.getRawParameterValue ("waveCharacter")->load();

    parameters.mix =
        apvts.getRawParameterValue ("waveMix")->load();

    parameters.enabled =
        apvts.getRawParameterValue ("waveguideEnabled")->load() > 0.5f;

    return parameters;
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveringAudioProcessor();
}