
#include "DistortionDSP.h"

namespace
{
constexpr float pi = 3.14159265358979323846f;

constexpr float maximumWet = 1.0f;
constexpr float maximumInput = 16.0f;

constexpr float minimumSampleRate = 8000.0f;
constexpr float maximumSampleRate = 384000.0f;

constexpr double smoothingTimeSeconds = 0.015;

constexpr float bodyCutoffHz = 180.0f;
}

void DistortionDSP::Smoother::reset(
    int rampSamples,
    float initial) noexcept
{
    rampLength = std::max(1, rampSamples);

    current = initial;
    target = initial;

    increment = 0.0f;
    remaining = 0;
}

void DistortionDSP::Smoother::setTarget(float value) noexcept
{
    if (!std::isfinite(value) || value == target)
        return;

    target = value;
    remaining = rampLength;

    increment = (target - current)
              / static_cast<float>(remaining);
}

float DistortionDSP::Smoother::next() noexcept
{
    if (remaining > 0)
    {
        current += increment;
        --remaining;

        if (remaining == 0)
            current = target;
    }

    return current;
}

float DistortionDSP::finiteOrZero(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float DistortionDSP::clampParameter(
    float value,
    float low,
    float high) noexcept
{
    return std::clamp(finiteOrZero(value), low, high);
}

int DistortionDSP::validModeIndex(Mode mode) noexcept
{
    const int value = static_cast<int>(mode);
    return value >= 0 && value <= 4 ? value : 0;
}

int DistortionDSP::validCharacterIndex(Character character) noexcept
{
    const int value = static_cast<int>(character);
    return value >= 0 && value <= 2 ? value : 0;
}

void DistortionDSP::prepare(
    double newSampleRate,
    const Parameters& initial) noexcept
{
    if (!std::isfinite(newSampleRate)
        || newSampleRate < minimumSampleRate)
    {
        sampleRate = 44100.0;
    }
    else
    {
        sampleRate = std::clamp(
            newSampleRate,
            static_cast<double>(minimumSampleRate),
            static_cast<double>(maximumSampleRate));
    }

    const int rampSamples = std::max(
        1,
        static_cast<int>(sampleRate * smoothingTimeSeconds));

    driveSmoother.reset(
        rampSamples,
        clampParameter(initial.drive, 0.0f, 10.0f));

    toneSmoother.reset(
        rampSamples,
        clampParameter(initial.tone, 0.0f, 10.0f));

    biasSmoother.reset(
        rampSamples,
        clampParameter(initial.bias, 0.0f, 10.0f));

    bodySmoother.reset(
        rampSamples,
        clampParameter(initial.body, 0.0f, 10.0f));

    mixSmoother.reset(
        rampSamples,
        clampParameter(initial.mix, 0.0f, 10.0f));

    modeSmoother.reset(
        rampSamples,
        static_cast<float>(validModeIndex(initial.mode)));

    characterSmoother.reset(
        rampSamples,
        static_cast<float>(validCharacterIndex(initial.character)));

    enableSmoother.reset(
        rampSamples,
        initial.enabled ? 1.0f : 0.0f);

    reset();
}

void DistortionDSP::reset() noexcept
{
    toneStateLeft = 0.0f;
    toneStateRight = 0.0f;

    bodyStateLeft = 0.0f;
    bodyStateRight = 0.0f;
}

void DistortionDSP::setParameters(
    const Parameters& parameters) noexcept
{
    driveSmoother.setTarget(
        clampParameter(parameters.drive, 0.0f, 10.0f));

    toneSmoother.setTarget(
        clampParameter(parameters.tone, 0.0f, 10.0f));

    biasSmoother.setTarget(
        clampParameter(parameters.bias, 0.0f, 10.0f));

    bodySmoother.setTarget(
        clampParameter(parameters.body, 0.0f, 10.0f));

    mixSmoother.setTarget(
        clampParameter(parameters.mix, 0.0f, 10.0f));

    modeSmoother.setTarget(
        static_cast<float>(validModeIndex(parameters.mode)));

    characterSmoother.setTarget(
        static_cast<float>(validCharacterIndex(parameters.character)));

    enableSmoother.setTarget(
        parameters.enabled ? 1.0f : 0.0f);
}

float DistortionDSP::processWaveshaper(
    float input,
    Mode mode,
    Character character) noexcept
{
    float shaped = 0.0f;

    switch (mode)
    {
        case Mode::drive:
            shaped = std::tanh(input);
            break;

        case Mode::overdrive:
        {
            const float asymmetric =
                input >= 0.0f
                    ? input * 1.08f
                    : input * 0.92f;

            shaped = std::tanh(asymmetric * 1.45f)
                   / std::tanh(1.45f);
            break;
        }

        case Mode::distortion:
            shaped = (2.0f / pi) * std::atan(input * 3.2f);
            break;

        case Mode::fuzz:
        {
            const float boosted = input * 4.5f;
            const float soft = std::tanh(boosted);

            const float folded = std::sin(
                std::clamp(boosted, -pi * 0.5f, pi * 0.5f));

            shaped = std::clamp(
                0.78f * soft + 0.22f * folded,
                -1.0f,
                1.0f);
            break;
        }

        case Mode::hardClip:
            shaped = std::clamp(input * 2.2f, -1.0f, 1.0f);
            break;
    }

    switch (character)
    {
        case Character::tube:
        {
            // Soft tube-style saturation.
            // At neutral bias this preserves the selected mode's
            // basic response while rounding its peaks.
            const float tube = std::tanh(shaped * 1.18f)
                             / std::tanh(1.18f);

            return finiteOrZero(tube);
        }

        case Character::digital:
        {
            // Firm digital clipping with a deliberately harder knee.
            return finiteOrZero(
                std::clamp(shaped * 1.35f, -1.0f, 1.0f));
        }

        case Character::hybrid:
        {
            const float tube = std::tanh(shaped * 1.18f)
                             / std::tanh(1.18f);

            const float digital =
                std::clamp(shaped * 1.35f, -1.0f, 1.0f);

            return finiteOrZero(
                0.5f * tube + 0.5f * digital);
        }
    }

    return finiteOrZero(shaped);
}

float DistortionDSP::processChannel(
    float input,
    float& toneState,
    float& bodyState,
    float tone,
    float drive,
    float bias,
    float body,
    float modePosition,
    float characterPosition) noexcept
{
    const float boundedInput = std::clamp(
        finiteOrZero(input),
        -maximumInput,
        maximumInput);

    const float driveGain = 1.0f + drive * 2.0f;

    // BIAS: 0 = negative asymmetry, 5 = neutral,
    // 10 = positive asymmetry.
    const float biasAmount = (bias - 5.0f) * 0.055f;

    const float biasedInput =
        boundedInput + biasAmount;

    const float positiveGain =
        1.0f + std::max(0.0f, biasAmount) * 0.8f;

    const float negativeGain =
        1.0f + std::max(0.0f, -biasAmount) * 0.8f;

    const float asymmetricInput =
        biasedInput >= 0.0f
            ? biasedInput * positiveGain
            : biasedInput * negativeGain;

    const float driven = finiteOrZero(
        asymmetricInput * driveGain);

    const float boundedMode = std::clamp(
        modePosition, 0.0f, 4.0f);

    const int lowerMode = static_cast<int>(boundedMode);
    const int upperMode = std::min(lowerMode + 1, 4);

    const float modeFraction =
        boundedMode - static_cast<float>(lowerMode);

    const float lowerShape = processWaveshaper(
        driven,
        static_cast<Mode>(lowerMode),
        Character::tube);

    const float upperShape = processWaveshaper(
        driven,
        static_cast<Mode>(upperMode),
        Character::tube);

    const float modeShaped =
        lowerShape + (upperShape - lowerShape) * modeFraction;

    const float boundedCharacter = std::clamp(
        characterPosition, 0.0f, 2.0f);

    const int lowerCharacter =
        static_cast<int>(boundedCharacter);

    const int upperCharacter =
        std::min(lowerCharacter + 1, 2);

    const float characterFraction =
        boundedCharacter - static_cast<float>(lowerCharacter);

    const float tubeShape = processWaveshaper(
        driven,
        static_cast<Mode>(lowerMode),
        Character::tube);

    const float digitalShape = processWaveshaper(
        driven,
        static_cast<Mode>(lowerMode),
        Character::digital);

    const float hybridShape = processWaveshaper(
        driven,
        static_cast<Mode>(lowerMode),
        Character::hybrid);

    float characterShape = tubeShape;

    if (lowerCharacter == 0)
        characterShape = tubeShape;
    else if (lowerCharacter == 1)
        characterShape = digitalShape;
    else
        characterShape = hybridShape;

    if (upperCharacter != lowerCharacter)
    {
        const float upperShape =
            upperCharacter == 1 ? digitalShape : hybridShape;

        characterShape +=
            (upperShape - characterShape) * characterFraction;
    }

    // Blend mode interpolation and character selection.
    // The character selector changes the saturation structure,
    // independently from the five-position distortion selector.
    const float modeOnly = modeShaped;

    const float wetBeforeTone =
        lowerMode == upperMode
            ? characterShape
            : 0.5f * modeOnly + 0.5f * characterShape;

    const float safeTone = std::clamp(tone, 0.0f, 10.0f) * 0.1f;

    const float cutoff =
        500.0f * std::pow(36.0f, safeTone);

    const float safeCutoff = std::min(
        cutoff,
        static_cast<float>(sampleRate * 0.45));

    const float toneCoefficient = 1.0f - std::exp(
        -2.0f * pi * safeCutoff
        / static_cast<float>(sampleRate));

    toneState += toneCoefficient * (wetBeforeTone - toneState);
    toneState = finiteOrZero(toneState);

    // BODY: a controlled low-frequency shelf-like enhancement.
    // The centre setting is neutral. It is not a master output gain.
    const float bodyCoefficient = 1.0f - std::exp(
        -2.0f * pi * bodyCutoffHz
        / static_cast<float>(sampleRate));

    bodyState += bodyCoefficient * (toneState - bodyState);
    bodyState = finiteOrZero(bodyState);

    const float bodyAmount =
        (std::clamp(body, 0.0f, 10.0f) - 5.0f) * 0.10f;

    const float bodyProcessed =
        toneState + bodyState * bodyAmount;

    return finiteOrZero(bodyProcessed);
}

void DistortionDSP::processFrame(
    float inputLeft,
    float inputRight,
    bool stereo,
    float& outputLeft,
    float& outputRight) noexcept
{
    const float drive = driveSmoother.next();
    const float tone = toneSmoother.next();
    const float bias = biasSmoother.next();
    const float body = bodySmoother.next();

    const float mix = mixSmoother.next() * 0.1f;

    const float modePosition = modeSmoother.next();
    const float characterPosition = characterSmoother.next();

    const float enabled = enableSmoother.next();

    const float dryLeft = finiteOrZero(inputLeft);

    const float dryRight = stereo
        ? finiteOrZero(inputRight)
        : dryLeft;

    if (mix <= 0.0f || enabled <= 0.0f)
    {
        outputLeft = dryLeft;
        outputRight = stereo ? dryRight : 0.0f;
        return;
    }

    const float wetLeft = std::clamp(
        processChannel(
            dryLeft,
            toneStateLeft,
            bodyStateLeft,
            tone,
            drive,
            bias,
            body,
            modePosition,
            characterPosition),
        -maximumWet,
        maximumWet);

    const float wetRight = stereo
        ? std::clamp(
            processChannel(
                dryRight,
                toneStateRight,
                bodyStateRight,
                tone,
                drive,
                bias,
                body,
                modePosition,
                characterPosition),
            -maximumWet,
            maximumWet)
        : wetLeft;

    const float wetMix = std::clamp(
        mix * enabled,
        0.0f,
        1.0f);

    outputLeft = finiteOrZero(
        dryLeft + (wetLeft - dryLeft) * wetMix);

    outputRight = stereo
        ? finiteOrZero(
            dryRight + (wetRight - dryRight) * wetMix)
        : 0.0f;
}
