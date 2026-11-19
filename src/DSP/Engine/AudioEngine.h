#pragma once

#include <cstdint>
#include <memory>
#include "DSP/Engine/VoiceManager.h"
#include "DSP/Engine/OscillatorEngine.h"
#include "DSP/Engine/OscillatorEngineFactory.h"
#include "DSP/Routing/RoutingGraph.h"
#include "DSP/Oscillators/SampleData.h"
#include "DSP/Oscillators/SamplePublisher.h"
#include "DSP/Wavetable/WavetableData.h"
#include "Parameters/SynthParameters.h"

namespace omnivy {

// -----------------------------------------------------------------------------
// Top-level DSP engine.
// -----------------------------------------------------------------------------
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void reset() noexcept;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    void setParameters (const SynthParameters& parameters);
    void setMasterVolume (float volume);
    void setPitchBendSemitones (float semitones);
    void setPolyphony (int polyphony);

    // --- Routing access (message thread) ----------------------------------
    dsp::RoutingGraph& getRoutingGraph() noexcept { return m_routingGraph; }

    // --- Sample access ------------------------------------------------------
    // Publica um sample (message thread). O áudio adquire o snapshot via
    // acquireSample() no início de cada processBlock.
    void publishSample (std::unique_ptr<dsp::SampleData> sample) noexcept;
    const dsp::SampleData* acquireSample() noexcept;

    int getNumActiveVoices() const noexcept;
    int getPolyphony() const noexcept;
    double getSampleRate() const noexcept { return m_sampleRate; }

    void setTestToneLevel (float level);
    float getTestToneLevel() const noexcept { return m_testToneLevel; }

private:
    void renderVoices (juce::AudioBuffer<float>& buffer);
    void handleMidi (const juce::MidiBuffer& midiMessages);

    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    bool m_prepared = false;

    dsp::WavetableData m_wavetable;
    VoiceManager m_voiceManager;
    dsp::RoutingGraph m_routingGraph;
    dsp::SamplePublisher m_samplePublisher;
    const SynthParameters* m_parameters = nullptr;

    float m_masterVolume = 0.7f;
    float m_testToneLevel = 0.0f;
    float m_testPhase = 0.0f;

    float m_pitchBendSemitones = 0.0f;

    // Sample adquirido uma vez por processBlock via acquireSample().
    // Válido durante todo o bloco; não-owning.
    const dsp::SampleData* m_currentSample = nullptr;
};

} // namespace omnivy
