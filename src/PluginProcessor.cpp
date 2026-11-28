
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

WaveringAudioProcessor::WaveringAudioProcessor()
    : AudioProcessor (
        BusesProperties()
            .withInput (
                "Input",
                juce::AudioChannelSet::stereo(),
                true)
            .withOutput (
                "Output",
                juce::AudioChannelSet::stereo(),
                true))
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
WaveringAudioProcessor::createParameterLayout()
{
    using FloatAttributes = juce::AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    const juce::NormalisableRange<float> controlRange (
        0.0f,
        10.0f,
        0.01f);

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "drive", 1 },
            "Drive",
            controlRange,
            0.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "tone", 1 },
            "Tone",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "bias", 1 },
            "Bias",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "body", 1 },
            "Body",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "mix", 1 },
            "Mix",
            controlRange,
            10.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "distortionEnabled", 1 },
            "Distortion On",
            true));

    parameters.push_back (
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "distortionMode", 1 },
            "Distortion Mode",
            juce::StringArray {
                "Drive",
                "Overdrive",
                "Distortion",
                "Fuzz",
                "Hard Clip"
            },
            0));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveRate", 1 },
            "Waveguide Rate",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveResonance", 1 },
            "Waveguide Resonance",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveFeedback", 1 },
            "Waveguide Feedback",
            controlRange,
            4.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveDamping", 1 },
            "Waveguide Damping",
            controlRange,
            5.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveCharacter", 1 },
            "Waveguide Character",
            controlRange,
            0.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "waveMix", 1 },
            "Waveguide Mix",
            controlRange,
            0.0f,
            FloatAttributes().withLabel ("")));

    parameters.push_back (
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "waveguideEnabled", 1 },
            "Waveguide On",
            false));

    return { parameters.begin(), parameters.end() };
}

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

    const int modeIndex = juce::roundToInt (
        apvts.getRawParameterValue ("distortionMode")->load());

    parameters.mode = static_cast<DistortionDSP::Mode>(
        juce::jlimit (0, 4, modeIndex));

    return parameters;
}

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

double WaveringAudioProcessor::getTailLengthSeconds() const
{
    const auto* enabledParameter =
        apvts.getRawParameterValue ("waveguideEnabled");

    const auto* mixParameter =
        apvts.getRawParameterValue ("waveMix");

    const auto* rateParameter =
        apvts.getRawParameterValue ("waveRate");

    const auto* feedbackParameter =
        apvts.getRawParameterValue ("waveFeedback");

    const auto* resonanceParameter =
        apvts.getRawParameterValue ("waveResonance");

    if (enabledParameter == nullptr
        || mixParameter == nullptr
        || rateParameter == nullptr
        || feedbackParameter == nullptr
        || resonanceParameter == nullptr)
        return 0.0;

    if (enabledParameter->load() <= 0.5f)
        return 0.0;

    const float mix = mixParameter->load();

    if (! std::isfinite (mix) || mix <= 0.001f)
        return 0.0;

    const float rawRate = rateParameter->load();
    const float rawFeedback = feedbackParameter->load();
    const float rawResonance = resonanceParameter->load();

    if (! std::isfinite (rawRate)
        || ! std::isfinite (rawFeedback)
        || ! std::isfinite (rawResonance))
        return 0.0;

    const double rate = juce::jlimit (
        0.0,
        1.0,
        static_cast<double> (rawRate) * 0.1);

    const double feedback = juce::jlimit (
        0.0,
        1.0,
        static_cast<double> (rawFeedback) * 0.1);

    const double resonance = juce::jlimit (
        0.0,
        1.0,
        static_cast<double> (rawResonance) * 0.1);

    const double delaySeconds =
        0.002 * std::pow (60.0, rate);

    constexpr double maximumFeedback = 0.94;
    constexpr double silenceThreshold = 0.001;
    constexpr double smoothingSeconds = 0.015;
    constexpr double maximumTailSeconds = 30.0;

    const double feedbackGain =
        maximumFeedback * feedback * (0.12 + 0.88 * resonance);

    if (! std::isfinite (delaySeconds)
        || ! std::isfinite (feedbackGain)
        || delaySeconds <= 0.0)
        return 0.0;

    if (feedbackGain <= silenceThreshold)
        return std::min (
            delaySeconds + smoothingSeconds,
            maximumTailSeconds);

    const double repeatCount = std::ceil (
        std::log (silenceThreshold) / std::log (feedbackGain));

    if (! std::isfinite (repeatCount) || repeatCount < 0.0)
        return maximumTailSeconds;

    const double tailSeconds =
        delaySeconds * (1.0 + repeatCount) + smoothingSeconds;

    return std::min (tailSeconds, maximumTailSeconds);
}

bool WaveringAudioProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    const auto& output = layouts.getMainOutputChannelSet();
    const auto& input = layouts.getMainInputChannelSet();

    return input == output
        && (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo());
}

void WaveringAudioProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate =
        std::isfinite (sampleRate) && sampleRate > 1.0
            ? sampleRate
            : 44100.0;

    distortion.prepare (
        currentSampleRate,
        readDistortionParameters());

    waveguide.prepare (
        currentSampleRate,
        readWaveguideParameters());

    for (auto& sample : visualizationInput)
        sample.store (0.0f, std::memory_order_relaxed);

    for (auto& sample : visualizationOutput)
        sample.store (0.0f, std::memory_order_relaxed);

    visualizationWritePosition.store (
        0,
        std::memory_order_relaxed);
}

void WaveringAudioProcessor::releaseResources()
{
    distortion.reset();
    waveguide.reset();
}

void WaveringAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    const int inputChannels = getTotalNumInputChannels();

    const int outputChannels = getTotalNumOutputChannels();

    for (int channel = inputChannels;
         channel < outputChannels;
         ++channel)
    {
        buffer.clear (channel, 0, numSamples);
    }

    if (inputChannels <= 0 || outputChannels <= 0)
        return;

    distortion.setParameters (readDistortionParameters());

    waveguide.setParameters (readWaveguideParameters());

    const bool stereo =
        outputChannels > 1 && inputChannels > 1;

    float* left = buffer.getWritePointer (0);

    float* right =
        stereo ? buffer.getWritePointer (1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float inputLeft = left[sample];

        const float inputRight =
            stereo ? right[sample] : inputLeft;

        const float visualInput =
            stereo
                ? 0.5f * (inputLeft + inputRight)
                : inputLeft;

        float distortedLeft = 0.0f;
        float distortedRight = 0.0f;

        distortion.processFrame (
            inputLeft,
            inputRight,
            stereo,
            distortedLeft,
            distortedRight);

        float outputLeft = 0.0f;
        float outputRight = 0.0f;

        waveguide.processFrame (
            distortedLeft,
            distortedRight,
            stereo,
            outputLeft,
            outputRight);

        left[sample] = outputLeft;

        if (stereo)
            right[sample] = outputRight;

        const float visualOutput =
            stereo
                ? 0.5f * (outputLeft + outputRight)
                : outputLeft;

        const unsigned int position =
            visualizationWritePosition.load (
                std::memory_order_relaxed);

        const unsigned int index =
            position & (visualizationBufferSize - 1);

        visualizationInput[index].store (
            std::isfinite (visualInput) ? visualInput : 0.0f,
            std::memory_order_relaxed);

        visualizationOutput[index].store (
            std::isfinite (visualOutput) ? visualOutput : 0.0f,
            std::memory_order_relaxed);

        visualizationWritePosition.store (
            position + 1,
            std::memory_order_release);
    }
}

void WaveringAudioProcessor::copyVisualizationData (
    VisualizationData& destination) const noexcept
{
    const unsigned int writePosition =
        visualizationWritePosition.load (
            std::memory_order_acquire);

    const unsigned int available =
        std::min (
            writePosition,
            static_cast<unsigned int> (visualizationBufferSize));

    destination.numSamples =
        static_cast<int> (available);

    const unsigned int startPosition =
        writePosition - available;

    for (unsigned int i = 0; i < available; ++i)
    {
        const unsigned int index =
            (startPosition + i)
            & (visualizationBufferSize - 1);

        destination.input[i] =
            visualizationInput[index].load (
                std::memory_order_relaxed);

        destination.output[i] =
            visualizationOutput[index].load (
                std::memory_order_relaxed);
    }
}

void WaveringAudioProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();

    juce::MemoryOutputStream stream (destData, false);

    state.writeToStream (stream);
}

void WaveringAudioProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    juce::MemoryInputStream stream (
        data,
        static_cast<size_t> (sizeInBytes),
        false);

    const auto state =
        juce::ValueTree::readFromStream (stream);

    if (state.isValid() && state.hasType ("Parameters"))
        apvts.replaceState (state);
}

juce::AudioProcessorEditor*
WaveringAudioProcessor::createEditor()
{
    return new WaveringAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveringAudioProcessor();
}

