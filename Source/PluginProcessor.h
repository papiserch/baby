#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class BabyAirAudioProcessor : public juce::AudioProcessor
{
public:
    BabyAirAudioProcessor();
    ~BabyAirAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BabyAir"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Nivel de salida (0..1) que usa el bebé para animarse
    std::atomic<float> level { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static float harmonics (float x, float amount);

    using Filter = juce::dsp::StateVariableTPTFilter<float>;
    Filter mhHP, mhLP, mhPost, hHP, hPost, hLP;

    juce::SmoothedValue<float> mhSmooth, hSmooth, mixSmooth, outSmooth;

    std::atomic<float>* pMH  = nullptr;
    std::atomic<float>* pH   = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOut = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabyAirAudioProcessor)
};
