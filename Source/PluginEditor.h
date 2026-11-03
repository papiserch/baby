#pragma once
#include "PluginProcessor.h"

class BabyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BabyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

class BabyAirAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit BabyAirAudioProcessorEditor (BabyAirAudioProcessor&);
    ~BabyAirAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Heart { float x, y, speed, size, drift, alpha; };

    BabyAirAudioProcessor& proc;
    BabyLookAndFeel lnf;
    juce::Image babyImg;
    juce::Rectangle<float> imageArea;

    float phase = 0.f, smoothLevel = 0.f, airAmount = 0.f;
    std::vector<Heart> hearts;
    juce::Random rng;

    juce::Slider mhSlider, hSlider, mixSlider, outSlider;
    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attach> mhAtt, hAtt, mixAtt, outAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabyAirAudioProcessorEditor)
};
