
auto drawPanel = [this, &g] (
    juce::Rectangle<int> bounds,
    const juce::String& title)
{
    juce::ignoreUnused(title);

    juce::ColourGradient plateGrad(
        gothicLF.steel,
        static_cast<float>(bounds.getX()),
        static_cast<float>(bounds.getY()),
        gothicLF.graphite,
        static_cast<float>(bounds.getX()),
        static_cast<float>(bounds.getBottom()),
        false
    );

    g.setGradientFill(plateGrad);
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);

    g.setColour(gothicLF.bloodRed.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds.toFloat(), 10.0f, 2.0f);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(2).toFloat(), 10.0f, 1.0f);
};
