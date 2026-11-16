#include "DSP/Engine/AudioEngine.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace omnivy {

AudioEngine::AudioEngine()
    : m_voiceManager (&m_wavetable)
{
}

void AudioEngine::prepare (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;

    // Build a single sine wavetable shared by any wavetable oscillators.
    m_wavetable.allocate (1, 2048);
    dsp::WavetableGenerator::generateSine (m_wavetable, 0);

    m_voiceManager.setSampleRate (sampleRate);
    m_voiceManager.setPolyphony (m_parameters.getPolyphony());

    m_testPhase = 0.0f;
    m_prepared = true;
}

void AudioEngine::releaseResources()
{
    m_prepared = false;
    m_voiceManager.reset();
}

void AudioEngine::reset()
{
    m_voiceManager.reset();
    m_testPhase = 0.0f;
}

void AudioEngine::setParameters (const SynthParameters& parameters)
{
    m_parameters = parameters; // copies atomic members? atomics are not copyable.
    // Instead, pull current values into our snapshot atomically:
    m_parameters.updateFromValueTreeState /* not available */;
    (void) parameters;
}

void AudioEngine::setMasterVolume (float volume)   { m_masterVolume = volume; }
void AudioEngine::setPitchBendSemitones (float semitones)
{
    m_pitchBendSemitones = semitones;
    m_voiceManager.setPitchBendSemitones (semitones);
}
void AudioEngine::setPolyphony (int polyphony) { m_voiceManager.setPolyphony (polyphony); }

int AudioEngine::getNumActiveVoices() const noexcept { return m_voiceManager.getNumActiveVoices(); }
int AudioEngine::getPolyphony() const noexcept { return m_voiceManager.getPolyphony(); }

void AudioEngine::setTestToneLevel (float level) { m_testToneLevel = level; }

void AudioEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumOutputChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear all output channels first.
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (!m_prepared)
        return;

    handleMidi (midiMessages);

    // Voice parameters (polyphony, per-osc, filter, envelope) applied each block.
    m_voiceManager.updateParameters (m_parameters);

    // Render voices into output channels 0 and 1.
    renderVoices (buffer);

    // Legacy Phase 1 test tone: phase-continuous 440 Hz sine when enabled.
    if (m_testToneLevel > 0.0f)
    {
        const double delta = 2.0 * dsp::Pi * 440.0 / m_sampleRate;
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                channelData[sample] += static_cast<float> (std::sin (m_testPhase)) * m_testToneLevel;
                m_testPhase += static_cast<float> (delta);
            }
        }
    }

    // Apply master volume.
    if (m_masterVolume != 1.0f)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int sample = 0; sample < numSamples; ++sample)
                channelData[sample] *= m_masterVolume;
        }
    }
}

void AudioEngine::renderVoices (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int totalNumOutputChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto frame = m_voiceManager.renderNextSample();
        if (totalNumOutputChannels > 0)
            *buffer.getWritePointer (0, sample) += frame.left;
        if (totalNumOutputChannels > 1)
            *buffer.getWritePointer (1, sample) += frame.right;
    }
}

void AudioEngine::handleMidi (const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const juce::MidiMessage& message = metadata.getMessage();

        if (message.isNoteOn())
        {
            m_voiceManager.noteOn (message.getNoteNumber(),
                                   static_cast<float> (message.getVelocity() / 127.0f));
        }
        else if (message.isNoteOff())
        {
            m_voiceManager.noteOff (message.getNoteNumber());
        }
        else if (message.isChannelPressure() || message.isPolyPressure())
        {
            // Reserved for future modulation; ignored in Phase 2.
        }
        else if (message.isPitchWheel())
        {
            // Pitch wheel: -8192..8197 -> -2..+2 semitones.
            const float bend = static_cast<float> (message.getPitchWheelValue());
            m_pitchBendSemitones = bend / 8192.0f * 2.0f;
            m_voiceManager.setPitchBendSemitones (m_pitchBendSemitones);
        }
        else if (message.isController())
        {
            const int controller = message.getControllerNumber();
            const int value = message.getControllerValue();

            // All notes off (CC 123).
            if (controller == 123)
                m_voiceManager.noteOffAll();
        }
    }
}

} // namespace omnivy
