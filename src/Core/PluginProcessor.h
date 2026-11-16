#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/Engine/AudioEngine.h"
#include "Parameters/SynthParameters.h"

class OmnivyAudioProcessor : public juce::AudioProcessor
{
public:
    OmnivyAudioProcessor();
    ~OmnivyAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Test parameter (Phase 1 legacy): drives the engine's test tone.
    float testParameter = 0.5f;

    // Host-automatable parameter tree. Layout is defined in SynthParameters.
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters",
        omnivy::SynthParameters::createParameterLayout() };

private:
    omnivy::SynthParameters m_synthParameters;
    omnivy::AudioEngine m_audioEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyAudioProcessor)
};
