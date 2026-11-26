#include "PluginProcessor.h"
#include "PluginEditor.h"

WaveringAudioProcessor::WaveringAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

juce::AudioProcessorValueTreeState::ParameterLayout WaveringAudioProcessor::createParameterLayout()
{
    using FloatAttributes = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    const juce::NormalisableRange<float> controlRange (0.0f, 10.0f, 0.01f);

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive", controlRange, 0.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bias", 1 }, "Bias", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "body", 1 }, "Body", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", controlRange, 10.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "distortionEnabled", 1 }, "Distortion On", true));

    return { parameters.begin(), parameters.end() };
}

DistortionDSP::Parameters WaveringAudioProcessor::readParameters() const noexcept
{
    DistortionDSP::Parameters parameters;
    parameters.drive = apvts.getRawParameterValue ("drive")->load();
    parameters.tone = apvts.getRawParameterValue ("tone")->load();
    parameters.bias = apvts.getRawParameterValue ("bias")->load();
    parameters.body = apvts.getRawParameterValue ("body")->load();
    parameters.mix = apvts.getRawParameterValue ("mix")->load();
    parameters.enabled = apvts.getRawParameterValue ("distortionEnabled")->load() > 0.5f;
    return parameters;
}

bool WaveringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& output = layouts.getMainOutputChannelSet();
    const auto& input = layouts.getMainInputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono()
                               || output == juce::AudioChannelSet::stereo());
}

void WaveringAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = std::isfinite (sampleRate) && sampleRate > 1.0 ? sampleRate : 44100.0;
    distortion.prepare (currentSampleRate, readParameters());
}

void WaveringAudioProcessor::releaseResources()
{
    distortion.reset();
}

void WaveringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();

    for (int channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    if (inputChannels <= 0 || outputChannels <= 0)
        return;

    distortion.setParameters (readParameters());
    const bool stereo = outputChannels > 1 && inputChannels > 1;
    float* left = buffer.getWritePointer (0);
    float* right = stereo ? buffer.getWritePointer (1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float inputLeft = left[sample];
        const float inputRight = stereo ? right[sample] : inputLeft;
        float outputLeft = 0.0f, outputRight = 0.0f;
        distortion.processFrame (inputLeft, inputRight, stereo, outputLeft, outputRight);
        left[sample] = outputLeft;
        if (stereo)
            right[sample] = outputRight;
    }
}

void WaveringAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    juce::MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void WaveringAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);
    const auto state = juce::ValueTree::readFromStream (stream);
    if (state.isValid() && state.hasType ("Parameters"))
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
