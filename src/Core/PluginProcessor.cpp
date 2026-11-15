#include "PluginProcessor.h"
#include "PluginEditor.h"

OmnivyAudioProcessor::OmnivyAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                      )
{
}

OmnivyAudioProcessor::~OmnivyAudioProcessor()
{
}

void OmnivyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Phase 1: nothing to initialize yet
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void OmnivyAudioProcessor::releaseResources()
{
}

bool OmnivyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return true;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void OmnivyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Phase 1 test audio: generate a silent buffer with a test tone
    // when testParameter > 0, to prove audio processing is functional.
    if (testParameter > 0.0f)
    {
        auto level = testParameter * 0.1f;
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                channelData[sample] = 0.0f; // silence for now, test parameter active
        }
    }

    juce::ignoreUnused (midiMessages);
}

juce::AudioProcessorEditor* OmnivyAudioProcessor::createEditor()
{
    return new OmnivyAudioProcessorEditor (*this);
}

bool OmnivyAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String OmnivyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OmnivyAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool OmnivyAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool OmnivyAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double OmnivyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OmnivyAudioProcessor::getNumPrograms()
{
    return 1;
}

int OmnivyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OmnivyAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String OmnivyAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void OmnivyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void OmnivyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Phase 1: save testParameter
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (testParameter);
}

void OmnivyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Phase 1: restore testParameter
    const char* d = static_cast<const char*> (data);
    if (sizeInBytes >= 4)
        testParameter = *reinterpret_cast<const float*> (d);
}
