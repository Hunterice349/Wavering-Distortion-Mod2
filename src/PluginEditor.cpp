#include "PluginEditor.h"

// ============================================================
//  MetalKnob
// ============================================================
MetalKnob::MetalKnob (const juce::String& labelText) : labelText (labelText)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
}

void MetalKnob::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const double len = getRange().getLength();
    const double n = len > 0.0
                   ? juce::jlimit (0.0, 1.0, (getValue() - getRange().getStart()) / len)
                   : 0.0;

    const float d = juce::jmin (b.getWidth(), b.getHeight()) - 16.0f;
    const float cx = b.getCentreX();
    const float cy = b.getCentreY() - 5.0f;
    juce::Rectangle<float> body (cx - d * 0.5f, cy - d * 0.5f, d, d);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (body.translated (2.0f, 4.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9aa0a6), body.getX(), body.getY(),
                                             juce::Colour (0xff2c2f33), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body.expanded (3.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdfe4e8), cx - d * 0.3f, body.getY(),
                                             juce::Colour (0xff43484e), cx + d * 0.3f, body.getBottom(), false));
    g.fillEllipse (body);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f),
                                             cx - d * 0.25f, cy - d * 0.3f,
                                             juce::Colours::white.withAlpha (0.0f),
                                             cx, cy, false));
    g.fillEllipse (body.reduced (5.0f));

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    for (int i = 0; i < 24; ++i)
    {
        const float a = juce::degreesToRadians (225.0f + 270.0f * static_cast<float> (i) / 23.0f);
        g.drawLine (cx + (d * 0.5f - 3.0f) * std::cos (a), cy + (d * 0.5f - 3.0f) * std::sin (a),
                    cx + (d * 0.5f - 6.0f) * std::cos (a), cy + (d * 0.5f - 6.0f) * std::sin (a), 1.0f);
    }

    g.setColour (juce::Colour (0xff35e0ff).withAlpha (0.9f));
    juce::Path arcPath;
    auto arcRect = body.expanded (7.0f);
    arcPath.addArc (arcRect.getX(), arcRect.getY(), arcRect.getWidth(), arcRect.getHeight(),
                    juce::degreesToRadians (225.0f),
                    juce::degreesToRadians (225.0f + 270.0f * static_cast<float> (n)), false);
    g.strokePath (arcPath, juce::PathStrokeType (2.0f));

    const float a = juce::degreesToRadians (225.0f + 270.0f * static_cast<float> (n));
    g.setColour (juce::Colours::black);
    g.drawLine (cx, cy, cx + (d * 0.5f - 9.0f) * std::cos (a), cy + (d * 0.5f - 9.0f) * std::sin (a), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawLine (cx - 1.0f, cy, cx + (d * 0.5f - 9.0f) * std::cos (a) - 1.0f,
                cy + (d * 0.5f - 9.0f) * std::sin (a), 1.0f);

    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawText (labelText, juce::Rectangle<int> (0, static_cast<int> (b.getHeight()) - 14,
                                                 static_cast<int> (b.getWidth()), 14),
                juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText (labelText, juce::Rectangle<int> (0, static_cast<int> (b.getHeight()) - 15,
                                                 static_cast<int> (b.getWidth()), 14),
                juce::Justification::centred);
}

// ============================================================
//  VuMeter
// ============================================================
VuMeter::VuMeter (WaveringAudioProcessor& p) : proc (p) {}

void VuMeter::update()
{
    float target = proc.outputLevelDb.load();

    if (target < -60.0f)
    {
        swayPhase += 0.02;
        target = -38.0f + 2.5f * static_cast<float> (std::sin (swayPhase))
                 + 0.8f * static_cast<float> (std::sin (swayPhase * 2.7));
    }

    const float rate = target > needleDb ? 0.35f : 0.06f;
    needleDb += (target - needleDb) * rate;
    repaint();
}

static float dbToAngle (float db)
{
    const float t = juce::jlimit (0.0f, 1.0f, (db + 24.0f) / 27.0f);
    return juce::degreesToRadians (195.0f + 150.0f * t);
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b, 6.0f);

    const auto face = b.reduced (10.0f, 10.0f);

    // JUCE 8.0.4: радиальный градиент через конструктор ColourGradient.
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0xfffefcf4),
        face.getCentreX(),
        face.getCentreY(),
        juce::Colour (0xffcfc8ba),
        face.getCentreX() + face.getWidth() * 0.7f,
        face.getCentreY(),
        true
    ));
    g.fillRoundedRectangle (face, 4.0f);

    const float cx = face.getCentreX();
    const float cy = face.getBottom() - 8.0f;
    const float R  = face.getHeight() - 34.0f;

    g.setColour (juce::Colours::black);
    juce::Path scalePath;
    juce::Rectangle<float> scaleRect (cx - R, cy - R, R * 2.0f, R * 2.0f);
    scalePath.addArc (scaleRect.getX(), scaleRect.getY(), scaleRect.getWidth(), scaleRect.getHeight(),
                      juce::degreesToRadians (195.0f), juce::degreesToRadians (150.0f), false);
    g.strokePath (scalePath, juce::PathStrokeType (1.0f));

    g.setColour (juce::Colour (0xffd21f1a));
    const float redR = R - 9.0f;
    juce::Path redPath;
    juce::Rectangle<float> redRect (cx - redR, cy - redR, redR * 2.0f, redR * 2.0f);
    redPath.addArc (redRect.getX(), redRect.getY(), redRect.getWidth(), redRect.getHeight(),
                    dbToAngle (0.0f), dbToAngle (3.0f) - dbToAngle (0.0f), false);
    g.strokePath (redPath, juce::PathStrokeType (2.0f));

    static const float majors[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (float db = -24.0f; db <= 3.0f; db += 1.0f)
    {
        const float a = dbToAngle (db);
        bool major = false;
        for (auto m : majors)
            if (std::abs (m - db) < 0.01f)
                major = true;

        const float len = major ? 13.0f : 7.0f;
        const float r1 = R, r2 = R - len;
        g.setColour (db >= 0.0f ? juce::Colour (0xffd21f1a) : juce::Colours::black);
        g.drawLine (cx + r1 * std::cos (a), cy + r1 * std::sin (a),
                    cx + r2 * std::cos (a), cy + r2 * std::sin (a), major ? 2.0f : 1.0f);
    }

    g.setFont (juce::Font (12.0f, juce::Font::bold));
    for (auto db : majors)
    {
        const float a = dbToAngle (db);
        const float lr = R - 30.0f;
        const float lx = cx + lr * std::cos (a);
        const float ly = cy + lr * std::sin (a);

        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (a - juce::degreesToRadians (270.0f), lx, ly));
        g.setColour (db > 0.0f ? juce::Colour (0xffd21f1a) : juce::Colours::black);
        const juce::String label = db < 0.0f ? juce::String (static_cast<int> (std::abs (db)))
                                             : (db == 0.0f ? "0" : "+" + juce::String (static_cast<int> (db)));
        g.drawText (label, juce::Rectangle<float> (lx - 14.0f, ly - 8.0f, 28.0f, 16.0f),
                    juce::Justification::centred);
        g.restoreState();
    }

    g.setFont (juce::Font (8.0f));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    const float pr = R - 17.0f;
    const float pDb[] = { -20, -10, -5, -2, 0 };
    const char* pTx[] = { "20", "40", "60", "80", "100%" };
    for (int i = 0; i < 5; ++i)
    {
        const float a = dbToAngle (pDb[i]);
        g.drawText (pTx[i], juce::Rectangle<float> (cx + pr * std::cos (a) - 12.0f,
                                                    cy + pr * std::sin (a) - 6.0f, 24.0f, 12.0f),
                    juce::Justification::centred);
    }

    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.setColour (juce::Colours::black);
    g.drawText ("-", juce::Rectangle<float> (cx - R - 4.0f, cy - 8.0f, 16.0f, 16.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xffd21f1a));
    g.drawText ("+", juce::Rectangle<float> (cx + R - 12.0f, cy - 8.0f, 16.0f, 16.0f), juce::Justification::centred);

    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.setColour (juce::Colours::black);
    g.drawText ("VU", juce::Rectangle<float> (cx - 30.0f, cy - R * 0.62f, 60.0f, 28.0f), juce::Justification::centred);

    const float na = dbToAngle (needleDb);
    const float nx = cx + (R - 12.0f) * std::cos (na);
    const float ny = cy + (R - 12.0f) * std::sin (na);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (cx + 1.5f, cy + 2.0f, nx + 1.5f, ny + 2.0f, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawLine (cx, cy, nx, ny, 2.0f);

    g.setColour (juce::Colours::black);
    g.fillEllipse (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.fillEllipse (cx - 3.0f, cy - 4.0f, 3.0f, 3.0f);

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());
    juce::Path glass;
    glass.startNewSubPath (face.getX(), face.getBottom());
    glass.lineTo (face.getX() + face.getWidth() * 0.42f, face.getY());
    glass.lineTo (face.getX() + face.getWidth() * 0.60f, face.getY());
    glass.lineTo (face.getX() + face.getWidth() * 0.18f, face.getBottom());
    glass.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f),
                                             face.getX(), face.getY(),
                                             juce::Colours::white.withAlpha (0.02f),
                                             face.getRight(), face.getBottom(), false));
    g.fillPath (glass);
    g.restoreState();
}

// ============================================================
//  RingScope
// ============================================================
RingScope::RingScope (WaveringAudioProcessor& p) : proc (p) {}

void RingScope::update()
{
    t += 0.016;
    const float lvl = proc.outputLevelDb.load();
    const float target = juce::jlimit (0.0f, 1.0f, std::pow (10.0f, lvl / 20.0f) * 1.6f);
    amp += (target - amp) * 0.2f;
    repaint();
}

void RingScope::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b, 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a1a22), b.getX(), b.getY(),
                                             juce::Colour (0xff000000), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (3.0f), 4.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine (static_cast<int> (b.getX() + b.getWidth() * i / 4.0f), b.getY(), b.getBottom());
    g.drawHorizontalLine (static_cast<int> (b.getCentreY()), b.getX(), b.getRight());

    const float f = proc.ringFreqHz.load();
    const float cycles = 1.0f + 6.0f * (f / 2000.0f);
    const float mid = b.getCentreY();
    const float A = amp * b.getHeight() * 0.38f;

    juce::Path p;
    for (float x = 0.0f; x <= b.getWidth(); x += 2.0f)
    {
        const float ph = static_cast<float> (std::sin (2.0f * juce::MathConstants<float>::pi
                                                       * (cycles * (t + x / 1000.0f))));
        const float y = mid + A * ph;
        if (x == 0.0f) p.startNewSubPath (b.getX() + x, y);
        else           p.lineTo (b.getX() + x, y);
    }
    g.setColour (juce::Colour (0xff35e0ff).withAlpha (0.25f));
    g.strokePath (p, juce::PathStrokeType (4.0f));
    g.setColour (juce::Colour (0xff35e0ff));
    g.strokePath (p, juce::PathStrokeType (1.5f));
}

// ============================================================
//  Led
// ============================================================
void Led::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);

    // JUCE 8.0.4: радиальный градиент через конструктор ColourGradient.
    g.setGradientFill (juce::ColourGradient (
        colour.withAlpha (0.5f * glow),
        b.getCentreX(),
        b.getCentreY(),
        juce::Colours::transparentBlack,
        b.getCentreX() + b.getWidth() * 1.4f,
        b.getCentreY(),
        true
    ));
    g.fillEllipse (b.expanded (b.getWidth()));

    g.setGradientFill (juce::ColourGradient (colour.brighter (0.6f), b.getX(), b.getY(),
                                             colour.darker (0.7f), b.getRight(), b.getBottom(), false));
    g.fillEllipse (b);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (b, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.5f + 0.5f * glow));
    g.fillEllipse (b.getX() + b.getWidth() * 0.22f, b.getY() + b.getHeight() * 0.12f,
                   b.getWidth() * 0.3f, b.getHeight() * 0.3f);
}

// ============================================================
//  BypassSwitch
// ============================================================
BypassSwitch::BypassSwitch (WaveringAudioProcessor& p) : proc (p) {}

void BypassSwitch::mouseDown (const juce::MouseEvent&)
{
    auto* param = proc.apvts.getParameter ("bypass");

    if (param != nullptr)
    {
        param->setValue (param->getValue() > 0.5f ? 0.0f : 1.0f);
        repaint();
    }
}

void BypassSwitch::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Защита от nullptr: параметр может отсутствовать или ещё не быть доступен.
    auto* bypassParameter = proc.apvts.getRawParameterValue ("bypass");
    const bool bypassed = bypassParameter != nullptr && bypassParameter->load() > 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (b.translated (1.5f, 2.5f), 8.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9bec4), b.getX(), b.getY(),
                                             juce::Colour (0xff4d5257), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (b.reduced (1.0f), 7.0f, 1.0f);

    const float lx = bypassed ? b.getX() + 6.0f : b.getRight() - 26.0f;
    juce::Rectangle<float> lever (lx, b.getCentreY() - 11.0f, 20.0f, 22.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (lever.translated (1.0f, 2.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e6e6), lever.getX(), lever.getY(),
                                             juce::Colour (0xff777c82), lever.getRight(), lever.getBottom(), false));
    g.fillRoundedRectangle (lever, 5.0f);

    const juce::Colour c = bypassed ? juce::Colour (0xff3a1210) : juce::Colour (0xff35ff7a);
    g.setColour (c);
    g.fillEllipse (b.getX() + 8.0f, b.getY() + 6.0f, 8.0f, 8.0f);

    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawText ("BYPASS", b, juce::Justification::centred);
}

// ============================================================
//  Editor
// ============================================================
WaveringAudioProcessorEditor::WaveringAudioProcessorEditor (WaveringAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), bypass (p)
{
    setResizable (false, false);
    setSize (460, 800);

    auto attach = [this] (const juce::String& id, juce::Slider& s)
    {
        attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                               (audioProcessor.apvts, id, s));
    };

    attach ("drive", driveKnob);
    attach ("wave",  waveKnob);
    attach ("ring",  ringKnob);
    attach ("res",   resKnob);
    attach ("tone",  toneKnob);
    attach ("mix",   mixKnob);
    attach ("out",   outKnob);

    vu    = std::make_unique<VuMeter>   (p);
    scope = std::make_unique<RingScope> (p);

    addAndMakeVisible (*vu);
    addAndMakeVisible (*scope);
    addAndMakeVisible (driveKnob); addAndMakeVisible (waveKnob); addAndMakeVisible (ringKnob);
    addAndMakeVisible (resKnob);   addAndMakeVisible (toneKnob); addAndMakeVisible (mixKnob);
    addAndMakeVisible (outKnob);
    addAndMakeVisible (bypass);
    addAndMakeVisible (powerLed);
    addAndMakeVisible (clipLed);

    buildTextures();
    startTimer (16);
}

WaveringAudioProcessorEditor::~WaveringAudioProcessorEditor() = default;

void WaveringAudioProcessorEditor::buildTextures()
{
    brushedTex = juce::Image (juce::Image::ARGB, 420, 800, true);
    {
        juce::Graphics g (brushedTex);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9ced4), 0.0f, 0.0f,
                                                 juce::Colour (0xff5f656b), 0.0f, 800.0f, false));
        g.fillAll();

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), 110.0f, 0.0f,
                                                 juce::Colours::white.withAlpha (0.0f), 260.0f, 0.0f, false));
        g.fillRect (0, 0, 420, 800);

        juce::Random rnd;
        for (int i = 0; i < 2600; ++i)
        {
            const float y = static_cast<float> (rnd.nextDouble()) * 800.0f;
            const float a = 0.02f + 0.05f * static_cast<float> (rnd.nextDouble());
            g.setColour (rnd.nextBool() ? juce::Colours::white.withAlpha (a)
                                        : juce::Colours::black.withAlpha (a));
            g.fillRect (0.0f, y, 420.0f, 1.0f);
        }
    }

    woodTex = juce::Image (juce::Image::ARGB, 24, 800, true);
    {
        juce::Graphics g (woodTex);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b3a1a), 0.0f, 0.0f,
                                                 juce::Colour (0xff331a08), 24.0f, 0.0f, false));
        g.fillAll();

        juce::Random rnd;
        for (int i = 0; i < 900; ++i)
        {
            const float x = static_cast<float> (rnd.nextDouble()) * 24.0f;
            const float a = 0.03f + 0.07f * static_cast<float> (rnd.nextDouble());
            g.setColour (rnd.nextBool() ? juce::Colours::white.withAlpha (a * 0.5f)
                                        : juce::Colours::black.withAlpha (a));
            g.fillRect (x, 0.0f, 1.0f, 800.0f);
        }
    }
}

static void drawScrew (juce::Graphics& g, float cx, float cy)
{
    juce::Rectangle<float> r (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (r.translated (1.0f, 1.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), cx - 5.0f, cy - 5.0f,
                                             juce::Colour (0xff606060), cx + 5.0f, cy + 5.0f, false));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (r, 1.0f);
    g.drawLine (cx - 4.0f, cy, cx + 4.0f, cy, 1.5f);
    g.drawLine (cx, cy - 4.0f, cx, cy + 4.0f, 1.5f);
}

void WaveringAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff241305));

    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    g.drawImageWithin (woodTex, 0.0f, 0.0f, 20.0f, h, juce::RectanglePlacement::stretchToFit);
    g.drawImageWithin (woodTex, w - 20.0f, 0.0f, 20.0f, h, juce::RectanglePlacement::stretchToFit);
    g.drawImageWithin (brushedTex, 20.0f, 0.0f, w - 40.0f, h, juce::RectanglePlacement::stretchToFit);

    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine (1, 20.0f, w - 20.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawHorizontalLine (static_cast<int> (h) - 2, 20.0f, w - 20.0f);

    drawScrew (g, 34.0f, 26.0f);  drawScrew (g, w - 34.0f, 26.0f);
    drawScrew (g, 34.0f, h - 26.0f); drawScrew (g, w - 34.0f, h - 26.0f);

    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawText ("WAVERING DISTORTION MOD", juce::Rectangle<int> (20, 366, static_cast<int> (w) - 40, 22),
                juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawText ("WAVERING DISTORTION MOD", juce::Rectangle<int> (20, 365, static_cast<int> (w) - 40, 22),
                juce::Justification::centred);

    g.setFont (juce::Font (10.0f));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawText ("WAVEGUIDE RESONATOR  •  DISTORTION  •  RING MODULATOR",
                juce::Rectangle<int> (20, 388, static_cast<int> (w) - 40, 16),
                juce::Justification::centred);
}

void WaveringAudioProcessorEditor::resized()
{
    vu->setBounds    (52, 44, 356, 244);
    scope->setBounds (150, 296, 160, 62);

    driveKnob.setBounds (66, 404, 88, 88);
    waveKnob.setBounds  (186, 404, 88, 88);
    ringKnob.setBounds  (306, 404, 88, 88);

    resKnob.setBounds   (66, 516, 88, 88);
    toneKnob.setBounds  (186, 516, 88, 88);
    mixKnob.setBounds   (306, 516, 88, 88);

    outKnob.setBounds   (186, 628, 88, 88);

    bypass.setBounds    (180, 736, 100, 44);
    powerLed.setBounds  (40, 748, 14, 14);
    clipLed.setBounds   (406, 748, 14, 14);
}

void WaveringAudioProcessorEditor::timerCallback()
{
    animTime += 0.016;

    vu->update();
    scope->update();

    const float lvl = audioProcessor.outputLevelDb.load();

    powerLed.setState (juce::Colour (0xff35ff7a),
                       0.55f + 0.45f * static_cast<float> (std::sin (animTime * 1.8)));

    clipLed.setState (lvl > -0.5f ? juce::Colour (0xffff3b30) : juce::Colour (0xff3a1210),
                      lvl > -0.5f ? 1.0f : 0.15f);
}

