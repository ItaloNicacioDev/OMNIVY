#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

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
    // Snapshot current host-automatable parameters into the DSP-friendly
    // structure, then hand it to the engine.
    m_synthParameters.updateFromValueTreeState (apvts);
    m_audioEngine.setParameters (m_synthParameters);
    m_audioEngine.setTestToneLevel (testParameter);
    m_audioEngine.prepare (sampleRate, static_cast<int> (samplesPerBlock));
}

void OmnivyAudioProcessor::releaseResources()
{
    m_audioEngine.releaseResources();
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

    // Refresh the engine's parameter snapshot from the APVTS each block so
    // host automation and UI changes are picked up lock-free.
    m_synthParameters.updateFromValueTreeState (apvts);
    m_audioEngine.setParameters (m_synthParameters);
    m_audioEngine.setTestToneLevel (testParameter);

    // The engine clears outputs, renders voices, handles MIDI and applies the
    // master gain — no dynamic allocation happens inside this callback.
    m_audioEngine.processBlock (buffer, midiMessages);
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

// JUCE plugin factory entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OmnivyAudioProcessor();
}
