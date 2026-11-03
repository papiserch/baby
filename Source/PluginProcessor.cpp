#include "PluginProcessor.h"
#include "PluginEditor.h"

BabyAirAudioProcessor::BabyAirAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pMH  = apvts.getRawParameterValue ("midhigh");
    pH   = apvts.getRawParameterValue ("high");
    pMix = apvts.getRawParameterValue ("mix");
    pOut = apvts.getRawParameterValue ("output");
}

juce::AudioProcessorValueTreeState::ParameterLayout BabyAirAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "midhigh", 1 }, "Mid-High",
                                                        NormalisableRange<float> (0.f, 100.f, 0.1f), 30.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "high", 1 }, "High (Air)",
                                                        NormalisableRange<float> (0.f, 100.f, 0.1f), 40.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                        NormalisableRange<float> (0.f, 100.f, 0.1f), 100.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                                                        NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f));
    return { p.begin(), p.end() };
}

void BabyAirAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    auto setup = [&] (Filter& f, juce::dsp::StateVariableTPTFilterType t, float freq)
    {
        f.prepare (spec);
        f.setType (t);
        f.setCutoffFrequency (freq);
        f.setResonance (0.707f);
        f.reset();
    };

    const float nyq = (float) sampleRate * 0.45f;

    // Banda Mid-High: 2.5 kHz - 8 kHz
    setup (mhHP,   juce::dsp::StateVariableTPTFilterType::highpass, 2500.f);
    setup (mhLP,   juce::dsp::StateVariableTPTFilterType::lowpass,  8000.f);
    setup (mhPost, juce::dsp::StateVariableTPTFilterType::highpass, 4500.f); // deja solo los armonicos nuevos

    // Banda High (Air): > 8 kHz
    setup (hHP,   juce::dsp::StateVariableTPTFilterType::highpass, 8000.f);
    setup (hPost, juce::dsp::StateVariableTPTFilterType::highpass, 10000.f);
    setup (hLP,   juce::dsp::StateVariableTPTFilterType::lowpass,  juce::jmin (20000.f, nyq));

    mhSmooth.reset (sampleRate, 0.03);
    hSmooth.reset  (sampleRate, 0.03);
    mixSmooth.reset (sampleRate, 0.03);
    outSmooth.reset (sampleRate, 0.03);

    mhSmooth.setCurrentAndTargetValue  (*pMH  / 100.f);
    hSmooth.setCurrentAndTargetValue   (*pH   / 100.f);
    mixSmooth.setCurrentAndTargetValue (*pMix / 100.f);
    outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
}

bool BabyAirAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

// Solo devuelve la parte NUEVA (armonicos), sin la senal original
float BabyAirAudioProcessor::harmonics (float x, float amount)
{
    const float d = 1.0f + amount * 10.0f;
    // termino x*x genera armonicos pares (calidez/aire), tanh los impares
    const float u = d * x + 0.4f * amount * d * d * x * x;
    return std::tanh (u) / d - x;
}

void BabyAirAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    mhSmooth.setTargetValue  (pMH->load()  / 100.f);
    hSmooth.setTargetValue   (pH->load()   / 100.f);
    mixSmooth.setTargetValue (pMix->load() / 100.f);
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    float peak = 0.f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float mh  = mhSmooth.getNextValue();
        const float h   = hSmooth.getNextValue();
        const float mix = mixSmooth.getNextValue();
        const float og  = outSmooth.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float x = data[n];

            const float bandMH = mhLP.processSample (ch, mhHP.processSample (ch, x));
            const float bandH  = hHP.processSample (ch, x);

            const float harmMH = mhPost.processSample (ch, harmonics (bandMH, mh));
            const float harmH  = hLP.processSample (ch, hPost.processSample (ch, harmonics (bandH, h)));

            const float wet = x + 3.0f * mh * harmMH + 3.0f * h * harmH;
            const float y = (x + mix * (wet - x)) * og;

            data[n] = y;
            peak = juce::jmax (peak, std::abs (y));
        }
    }

    // nivel con caida suave para animar al bebe
    float cur = level.load();
    cur = (peak > cur) ? peak : cur * 0.90f;
    level.store (juce::jlimit (0.f, 1.f, cur));
}

juce::AudioProcessorEditor* BabyAirAudioProcessor::createEditor()
{
    return new BabyAirAudioProcessorEditor (*this);
}

void BabyAirAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BabyAirAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BabyAirAudioProcessor();
}
