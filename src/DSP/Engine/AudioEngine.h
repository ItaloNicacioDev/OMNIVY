#pragma once

#include <cstdint>
#include "DSP/Engine/VoiceManager.h"
#include "DSP/Wavetable/WavetableData.h"
#include "Parameters/SynthParameters.h"

namespace omnivy {

// -----------------------------------------------------------------------------
// Top-level DSP engine. Independent of the UI layer. Owns the voice manager,
// the shared wavetable data and the parameter snapshot. Real-time safe:
// prepare() allocates once; processBlock() performs no dynamic allocation.
//
// Signal flow:
//   MIDI -> VoiceManager (allocation/stealing) -> 6 oscillators/voice -> sum
//   -> filter -> amp envelope -> master gain -> output
// -----------------------------------------------------------------------------
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine() = default;

    // Call once before processing. Allocates the voice pool and wavetables.
    void prepare (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void reset();

    // Process one block. Reads MIDI, allocates/releases voices, renders audio.
    // No dynamic allocation is performed here.
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Parameter / control setters (called from the plugin each block).
    void setParameters (const SynthParameters& parameters);
    void setMasterVolume (float volume);
    void setPitchBendSemitones (float semitones);
    void setPolyphony (int polyphony);

    int getNumActiveVoices() const noexcept;
    int getPolyphony() const noexcept;
    double getSampleRate() const noexcept { return m_sampleRate; }

    // Legacy test tone (phase-continuous 440 Hz sine) used by Phase 1.
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
    SynthParameters m_parameters;

    float m_masterVolume = 0.7f;
    float m_testToneLevel = 0.0f;
    float m_testPhase = 0.0f;

    float m_pitchBendSemitones = 0.0f;
};

} // namespace omnivy
