#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <juce_data_structures/juce_data_structures.h>

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
    return new omnivy::gui::OmnivyAudioProcessorEditor (*this);
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
    juce::MemoryOutputStream stream (destData, true);
    // Mantém o legacy testParameter nos primeiros 4 bytes (compatibilidade).
    stream.writeFloat (testParameter);
    // Persiste a árvore completa de parâmetros (todos os parâmetros reais do
    // APVTS), para que salvar/carregar preset preserve o estado do sintetizador.
    apvts.state.writeToStream (stream);
}

void OmnivyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const char* d = static_cast<const char*> (data);
    if (sizeInBytes >= 4)
        testParameter = *reinterpret_cast<const float*> (d);

    // Restaura a árvore de parâmetros, se presente no bloco (após os 4 bytes
    // do legacy testParameter). Em vez de substituir apvts.state (o que
    // deixaria os parâmetros com referências pendentes), aplica cada
    // propriedade carregada ao parâmetro correspondente, preservando a ligação
    // ValueTree -> AudioProcessorParameter e notificando listeners/attachments.
    if (sizeInBytes > 4)
    {
        juce::ValueTree tree = juce::ValueTree::readFromData (d + 4, static_cast<size_t> (sizeInBytes - 4));
        if (tree.isValid())
        {
            for (int i = 0; i < tree.getNumProperties(); ++i)
            {
                juce::Identifier propName = tree.getPropertyName (i);
                if (auto* param = apvts.getParameter (propName.toString()))
                    param->setValue (static_cast<float> (tree.getProperty (propName)));
            }
        }
    }
}

// JUCE plugin factory entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OmnivyAudioProcessor();
}
