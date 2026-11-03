#include "PluginEditor.h"
#include "BinaryData.h"

static const juce::Colour pink (0xffff3d8b);

//==============================================================================
BabyLookAndFeel::BabyLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void BabyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a1830), c.x, c.y - r,
                                             juce::Colour (0xff1a0a14), c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);

    juce::Path bg;
    bg.addCentredArc (c.x, c.y, r - 4.f, r - 4.f, 0.f, startA, endA, true);
    g.setColour (juce::Colour (0xff55263f));
    g.strokePath (bg, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 4.f, r - 4.f, 0.f, startA, angle, true);
    g.setColour (pink);
    g.strokePath (val, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path ptr;
    ptr.addRoundedRectangle (-2.f, -r + 10.f, 4.f, r * 0.35f, 2.f);
    g.setColour (juce::Colours::white);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
BabyAirAudioProcessorEditor::BabyAirAudioProcessorEditor (BabyAirAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    babyImg = juce::ImageCache::getFromMemory (BinaryData::baby_png, BinaryData::baby_pngSize);

    auto setup = [this] (juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        s.setTextValueSuffix (suffix);
        s.setLookAndFeel (&lnf);
        addAndMakeVisible (s);
    };
    setup (mhSlider, " %");
    setup (hSlider, " %");
    setup (mixSlider, " %");
    setup (outSlider, " dB");

    mhAtt  = std::make_unique<Attach> (proc.apvts, "midhigh", mhSlider);
    hAtt   = std::make_unique<Attach> (proc.apvts, "high",    hSlider);
    mixAtt = std::make_unique<Attach> (proc.apvts, "mix",     mixSlider);
    outAtt = std::make_unique<Attach> (proc.apvts, "output",  outSlider);

    setSize (420, 680);
    startTimerHz (60);
}

BabyAirAudioProcessorEditor::~BabyAirAudioProcessorEditor()
{
    stopTimer();
    for (auto* s : { &mhSlider, &hSlider, &mixSlider, &outSlider })
        s->setLookAndFeel (nullptr);
}

void BabyAirAudioProcessorEditor::resized()
{
    imageArea = juce::Rectangle<float> (10.f, 10.f, (float) getWidth() - 20.f, 470.f);

    auto row = juce::Rectangle<int> (10, 490, getWidth() - 20, 140);
    const int w = row.getWidth() / 4;
    mhSlider.setBounds  (row.removeFromLeft (w));
    hSlider.setBounds   (row.removeFromLeft (w));
    mixSlider.setBounds (row.removeFromLeft (w));
    outSlider.setBounds (row);
}

void BabyAirAudioProcessorEditor::timerCallback()
{
    const float target = juce::jlimit (0.f, 1.f, proc.level.load() * 1.6f);
    smoothLevel += (target - smoothLevel) * 0.35f;

    airAmount = juce::jlimit (0.f, 1.f, (float) (mhSlider.getValue() + hSlider.getValue()) / 200.f);

    phase += 0.05f + smoothLevel * 0.25f;

    // Corazoncitos suben cuando hay audio
    if (smoothLevel > 0.03f && rng.nextFloat() < 0.15f + smoothLevel * 0.5f)
    {
        hearts.push_back ({ imageArea.getX() + rng.nextFloat() * imageArea.getWidth(),
                            imageArea.getBottom(),
                            1.0f + rng.nextFloat() * 2.0f + smoothLevel * 2.f,
                            6.f + rng.nextFloat() * 10.f,
                            (rng.nextFloat() - 0.5f) * 0.8f,
                            0.9f });
    }

    for (auto& h : hearts)
    {
        h.y -= h.speed;
        h.x += h.drift + std::sin (h.y * 0.05f) * 0.5f;
        h.alpha -= 0.004f;
    }
    hearts.erase (std::remove_if (hearts.begin(), hearts.end(),
                                  [] (const Heart& h) { return h.alpha <= 0.f; }),
                  hearts.end());

    repaint();
}

static juce::Path makeHeart (float size)
{
    juce::Path p;
    p.startNewSubPath (0.f, 0.4f);
    p.cubicTo (-0.9f, -0.2f, -0.5f, -0.9f, 0.f, -0.35f);
    p.cubicTo (0.5f, -0.9f, 0.9f, -0.2f, 0.f, 0.4f);
    p.closeSubPath();
    p.applyTransform (juce::AffineTransform::scale (size));
    return p;
}

void BabyAirAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14080f));

    const float cx = imageArea.getCentreX();
    const float cy = imageArea.getCentreY();

    g.saveState();

    juce::Path clip;
    clip.addRoundedRectangle (imageArea, 18.f);
    g.reduceClipRegion (clip, juce::AffineTransform());

    // --- Bebe animado: flota, respira, se agranda y tiembla con el audio ---
    g.saveState();
    const float scale = 1.08f + 0.012f * std::sin (phase * 2.f) + smoothLevel * 0.10f;
    const float dy    = std::sin (phase) * 8.f - smoothLevel * 10.f;
    const float rot   = std::sin (phase * 0.5f) * 0.03f
                        + (rng.nextFloat() - 0.5f) * smoothLevel * 0.02f;

    auto t = juce::AffineTransform::scale (scale, scale, cx, cy)
                 .followedBy (juce::AffineTransform::rotation (rot, cx, cy))
                 .followedBy (juce::AffineTransform::translation (0.f, dy));
    g.addTransform (t);

    if (babyImg.isValid())
        g.drawImage (babyImg, imageArea,
                     juce::RectanglePlacement (juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination));
    g.restoreState();

    // --- Brillo rosa que respira con la cantidad de "air" y el nivel ---
    const float glow = 0.08f + airAmount * 0.15f + smoothLevel * 0.35f;
    g.setGradientFill (juce::ColourGradient (pink.withAlpha (glow), cx, imageArea.getBottom(),
                                             pink.withAlpha (0.f), cx, imageArea.getY(), false));
    g.fillRect (imageArea);

    // --- Corazones ---
    for (const auto& h : hearts)
    {
        g.setColour (pink.brighter (0.4f).withAlpha (juce::jlimit (0.f, 1.f, h.alpha)));
        g.fillPath (makeHeart (h.size), juce::AffineTransform::translation (h.x, h.y));
    }

    g.restoreState();

    // Borde
    g.setColour (pink.withAlpha (0.5f + smoothLevel * 0.5f));
    g.drawRoundedRectangle (imageArea, 18.f, 2.f);

    // Etiquetas
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::FontOptions (13.f, juce::Font::bold));
    const char* names[] = { "MID-HIGH", "HIGH", "MIX", "OUTPUT" };
    const int w = (getWidth() - 20) / 4;
    for (int i = 0; i < 4; ++i)
        g.drawText (names[i], 10 + i * w, 636, w, 18, juce::Justification::centred);

    g.setColour (pink);
    g.setFont (juce::FontOptions (12.f, juce::Font::bold));
    g.drawText ("BabyAir", 0, 658, getWidth(), 16, juce::Justification::centred);
}
