#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RoomReverbAudioProcessor::RoomReverbAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

RoomReverbAudioProcessor::~RoomReverbAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
RoomReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Room size  0–1  (maps directly to Reverb::Parameters::roomSize)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "roomSize", "Room Size", 0.0f, 1.0f, 0.5f));

    // Damping    0–1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "damping", "Damping", 0.0f, 1.0f, 0.5f));

    // Wet mix    0–1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "wet", "Wet Mix", 0.0f, 1.0f, 0.33f));

    // Width      0–1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "width", "Width", 0.0f, 1.0f, 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void RoomReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;

    reverb.prepare (spec);
}

void RoomReverbAudioProcessor::releaseResources() {}

bool RoomReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void RoomReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Pull current parameter values every block
    juce::Reverb::Parameters p;
    p.roomSize   = *apvts.getRawParameterValue ("roomSize");
    p.damping    = *apvts.getRawParameterValue ("damping");
    p.wetLevel   = *apvts.getRawParameterValue ("wet");
    p.dryLevel   = 1.0f - p.wetLevel;
    p.width      = *apvts.getRawParameterValue ("width");
    p.freezeMode = 0.0f;
    reverb.setParameters (p);

    juce::dsp::AudioBlock<float>       block (buffer);
    juce::dsp::ProcessContextReplacing ctx   (block);
    reverb.process (ctx);
}

//==============================================================================
juce::AudioProcessorEditor* RoomReverbAudioProcessor::createEditor()
{
    return new RoomReverbAudioProcessorEditor (*this);
}

//==============================================================================
void RoomReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RoomReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RoomReverbAudioProcessor();
}
