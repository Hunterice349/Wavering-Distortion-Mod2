double WaveringAudioProcessor::getTailLengthSeconds() const
{
const auto read = [this] (const char* id) noexcept
{
const auto* parameter = apvts.getRawParameterValue (id);
if (parameter == nullptr)
return 0.0;

    const auto value = parameter->load();
    if (! std::isfinite (value))
        return 0.0;

    return std::clamp (static_cast<double> (value), 0.0, 10.0);
};

const auto isEnabled = [&read] (const char* id) noexcept
{
    return read (id) > 0.5;
};

const auto estimateTail = [] (double delaySeconds, double feedback) noexcept
{
    if (! std::isfinite (delaySeconds) || delaySeconds < 0.0)
        return 0.0;

    if (! std::isfinite (feedback))
        return maxTailSeconds;

    feedback = std::clamp (feedback, 0.0, 0.999999);

    double tailSeconds = delaySeconds;

    if (feedback > 0.0)
    {
        const double repeatsToThreshold =
            std::ceil (std::log (tailThreshold) / std::log (feedback));

        tailSeconds = delaySeconds * (1.0 + repeatsToThreshold);
    }

    return std::isfinite (tailSeconds) ? tailSeconds : maxTailSeconds;
};

double waveguideTail = 0.0;

if (isEnabled ("waveguideEnabled") && read ("waveMix") > 0.0)
{
    const double delaySeconds =
        0.002 * std::pow (60.0, read ("waveRate") / 10.0);

    const double feedback =
        0.94 * (read ("waveFeedback") / 10.0)
        * (0.12 + 0.88 * read ("waveResonance") / 10.0);

    waveguideTail = estimateTail (delaySeconds, feedback);
}

double spectralDelayTail = 0.0;

if (isEnabled ("spectralDelayEnabled")
    && read ("spectralDelayMix") > 0.0)
{
    const double baseDelaySeconds =
        0.01 * std::pow (150.0, read ("spectralDelayTime") / 10.0);

    const double bandFactor =
        0.5 + 0.1 * std::max ({
            read ("spectralDelayLow"),
            read ("spectralDelayMid"),
            read ("spectralDelayHigh")
        });

    const double delaySeconds =
        std::min (baseDelaySeconds * bandFactor, 2.25);

    const double feedback =
        0.92 * read ("spectralDelayFeedback") / 10.0;

    spectralDelayTail = estimateTail (delaySeconds, feedback);
}

const double totalTail = waveguideTail + spectralDelayTail;

if (! std::isfinite (totalTail))
    return maxTailSeconds;

return std::clamp (totalTail, 0.0, maxTailSeconds);

}
