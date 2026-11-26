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

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveRate", 1 }, "Waveguide Rate", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveResonance", 1 }, "Waveguide Resonance", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveFeedback", 1 }, "Waveguide Feedback", controlRange, 4.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveDamping", 1 }, "Waveguide Damping", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveCharacter", 1 }, "Waveguide Character", controlRange, 0.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "waveMix", 1 }, "Waveguide Mix", controlRange, 0.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "waveguideEnabled", 1 }, "Waveguide On", false));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ringRate", 1 }, "Ring Mod Rate", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ringDepth", 1 }, "Ring Mod Depth", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ringWaveform", 1 }, "Ring Mod Waveform",
        juce::StringArray { "Sine", "Triangle", "Square", "Saw" }, 0));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ringFine", 1 }, "Ring Mod Fine", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ringMix", 1 }, "Ring Mod Mix", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ringEnabled", 1 }, "Ring Mod On", false));

    // Preserve the serialized legacy indices: 0=50%, 1=100%, 2=200%.
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "uiScale", 1 }, "UI Scale",
        juce::StringArray { "50%", "100%", "200%", "60%", "70%", "80%", "90%" }, 1));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayTime", 1 }, "Spectral Delay Time", controlRange, 3.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayLow", 1 }, "Spectral Delay Low", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayMid", 1 }, "Spectral Delay Mid", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayHigh", 1 }, "Spectral Delay High", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayFeedback", 1 }, "Spectral Delay Feedback", controlRange, 3.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayWidth", 1 }, "Spectral Delay Width", controlRange, 5.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralDelayMix", 1 }, "Spectral Delay Mix", controlRange, 0.0f,
        FloatAttributes().withLabel ("")));
    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "spectralDelayEnabled", 1 }, "Spectral Delay On", false));

    return { parameters.begin(), parameters.end() };
}

DistortionDSP::Parameters WaveringAudioProcessor::readDistortionParameters() const noexcept
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

WaveguideDSP::Parameters WaveringAudioProcessor::readWaveguideParameters() const noexcept
{
    WaveguideDSP::Parameters parameters;
    parameters.rate = apvts.getRawParameterValue ("waveRate")->load();
    parameters.resonance = apvts.getRawParameterValue ("waveResonance")->load();
    parameters.feedback = apvts.getRawParameterValue ("waveFeedback")->load();
    parameters.damping = apvts.getRawParameterValue ("waveDamping")->load();
    parameters.character = apvts.getRawParameterValue ("waveCharacter")->load();
    parameters.mix = apvts.getRawParameterValue ("waveMix")->load();
    parameters.enabled = apvts.getRawParameterValue ("waveguideEnabled")->load() > 0.5f;
    return parameters;
}

RingModulatorDSP::Parameters WaveringAudioProcessor::readRingModulatorParameters() const noexcept
{
    RingModulatorDSP::Parameters parameters;
    parameters.rate = apvts.getRawParameterValue ("ringRate")->load();
    parameters.depth = apvts.getRawParameterValue ("ringDepth")->load();
    parameters.waveform = static_cast<int> (std::lround (apvts.getRawParameterValue ("ringWaveform")->load()));
    parameters.fine = apvts.getRawParameterValue ("ringFine")->load();
    parameters.mix = apvts.getRawParameterValue ("ringMix")->load();
    parameters.enabled = apvts.getRawParameterValue ("ringEnabled")->load() > 0.5f;
    return parameters;
}

SpectralDelayDSP::Parameters WaveringAudioProcessor::readSpectralDelayParameters() const noexcept
{
    SpectralDelayDSP::Parameters parameters;
    parameters.time = apvts.getRawParameterValue ("spectralDelayTime")->load();
    parameters.low = apvts.getRawParameterValue ("spectralDelayLow")->load();
    parameters.mid = apvts.getRawParameterValue ("spectralDelayMid")->load();
    parameters.high = apvts.getRawParameterValue ("spectralDelayHigh")->load();
    parameters.feedback = apvts.getRawParameterValue ("spectralDelayFeedback")->load();
    parameters.width = apvts.getRawParameterValue ("spectralDelayWidth")->load();
    parameters.mix = apvts.getRawParameterValue ("spectralDelayMix")->load();
    parameters.enabled = apvts.getRawParameterValue ("spectralDelayEnabled")->load() > 0.5f;
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
    distortion.prepare (currentSampleRate, readDistortionParameters());
    waveguide.prepare (currentSampleRate, readWaveguideParameters());
    ringModulator.prepare (currentSampleRate, readRingModulatorParameters());
    spectralDelay.prepare (currentSampleRate, readSpectralDelayParameters());
}

void WaveringAudioProcessor::releaseResources()
{
    distortion.reset();
    waveguide.reset();
    ringModulator.reset();
    spectralDelay.reset();
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

    distortion.setParameters (readDistortionParameters());
    waveguide.setParameters (readWaveguideParameters());
    ringModulator.setParameters (readRingModulatorParameters());
    spectralDelay.setParameters (readSpectralDelayParameters());
    const bool stereo = outputChannels > 1 && inputChannels > 1;
    float* left = buffer.getWritePointer (0);
    float* right = stereo ? buffer.getWritePointer (1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float inputLeft = left[sample];
        const float inputRight = stereo ? right[sample] : inputLeft;
        float distortedLeft = 0.0f, distortedRight = 0.0f;
        distortion.processFrame (inputLeft, inputRight, stereo, distortedLeft, distortedRight);

        float resonatedLeft = 0.0f, resonatedRight = 0.0f;
        waveguide.processFrame (distortedLeft, distortedRight, stereo, resonatedLeft, resonatedRight);

        float ringLeft = 0.0f, ringRight = 0.0f;
        ringModulator.processFrame (resonatedLeft, resonatedRight, stereo, ringLeft, ringRight);

        float outputLeft = 0.0f, outputRight = 0.0f;
        spectralDelay.processFrame (ringLeft, ringRight, stereo, outputLeft, outputRight);
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
