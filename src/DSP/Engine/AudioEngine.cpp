#include "DSP/Engine/AudioEngine.h"
#include "DSP/Utilities/DSPUtilities.h"
#include "DSP/Wavetable/WavetableGenerator.h"

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
    if (m_parameters != nullptr)
        m_voiceManager.setPolyphony (m_parameters->getPolyphony());
    else
        m_voiceManager.setPolyphony (16);

    // Bind all voice engines to the routing graph (node = voice*6 + slot).
    m_voiceManager.visitEngines ([this] (dsp::OscillatorEngine* engine, dsp::NodeId nodeId)
    {
        m_routingGraph.bindEngine (nodeId, engine);
    });

    m_testPhase = 0.0f;

    // Inicializa o smoother do master volume com o valor atual.
    m_masterVolumeSmoother.setSampleRate (sampleRate);
    m_masterVolumeSmoother.setSmoothTime (0.005f); // 5 ms
    m_masterVolumeSmoother.reset (m_masterVolume);

    m_prepared = true;
}

void AudioEngine::releaseResources()
{
    m_prepared = false;
    m_voiceManager.reset();
    // O SamplePublisher é destruído com o AudioEngine (via ~SamplePublisher
    // no destructor), que garante que a thread de áudio já parou. Não desligar
    // aqui preserva o sample publicado através de ciclos de parada e
    // reinicialização do processamento (prepareToPlay / releaseResources).
}

void AudioEngine::reset() noexcept
{
    m_voiceManager.reset();
    m_testPhase = 0.0f;
}

void AudioEngine::setParameters (const SynthParameters& parameters)
{
    m_parameters = &parameters; // engine does not own; plugin guarantees lifetime
    m_voiceManager.setPolyphony (parameters.getPolyphony());
}

void AudioEngine::setMasterVolume (float volume)   { m_masterVolume.store (volume); }
void AudioEngine::setPitchBendSemitones (float semitones)
{
    m_pitchBendSemitones = semitones;
    m_voiceManager.setPitchBendSemitones (semitones);
}
void AudioEngine::setPolyphony (int polyphony) { m_voiceManager.setPolyphony (polyphony); }

int AudioEngine::getNumActiveVoices() const noexcept { return m_voiceManager.getNumActiveVoices(); }

void AudioEngine::setTestToneLevel (float level) { m_testToneLevel = level; }

void AudioEngine::publishSample (std::unique_ptr<dsp::SampleData> sample) noexcept
{
    m_samplePublisher.publish (std::move (sample));
}

const dsp::SampleData* AudioEngine::acquireSample() noexcept
{
    return m_samplePublisher.acquire();
}

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

    // Adquire o snapshot do sample uma vez por bloco (RCU). O ponteiro é válido
    // durante todo o processBlock e é partilhado por todas as vozes/slots.
    m_currentSample = acquireSample();

    // Define o sample em todos os engines granulares (no-op para engines não-granulares).
    m_voiceManager.visitEngines ([this] (dsp::OscillatorEngine* engine, dsp::NodeId)
    {
        engine->setGranularSample (m_currentSample);
    });

    handleMidi (midiMessages);

    // Voice parameters (polyphony, per-osc, filter, envelope) applied each block.
    if (m_parameters != nullptr)
        m_voiceManager.updateParameters (*m_parameters);

    // Apply routing modulation before rendering voices.
    m_routingGraph.applyModulations();

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

    // Apply master volume (suavizado para evitar clicks em automação).
    m_masterVolumeSmoother.setTarget (m_masterVolume.load ());
    const float smoothedMasterVolume = m_masterVolumeSmoother.getNextValue();
    if (smoothedMasterVolume != 1.0f)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int sample = 0; sample < numSamples; ++sample)
                channelData[sample] *= smoothedMasterVolume;
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
        else if (message.isPitchWheel())
        {
            // Pitch wheel: -8192..8197 -> -2..+2 semitones.
            const float bend = static_cast<float> (message.getPitchWheelValue());
            m_pitchBendSemitones = bend / 8192.0f * 2.0f;
            m_voiceManager.setPitchBendSemitones (m_pitchBendSemitones);
        }
        else if (message.isController())
        {
            // All notes off (CC 123) - stuck-note prevention.
            if (message.getControllerNumber() == 123)
                m_voiceManager.noteOffAll();
        }
    }
}

} // namespace omnivy
