#pragma once

#include <memory>
#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Engine/OscillatorEngine.h"
#include "DSP/Wavetable/WavetableData.h"
#include "DSP/Envelopes/ADSREnvelope.h"
#include "DSP/Filters/StateVariableFilter.h"
#include "Parameters/SynthParameters.h"

namespace omnivy {

// -----------------------------------------------------------------------------
// Single synthesis voice. Owns six independent oscillators, an amplitude
// envelope and a stereo filter. All state is per-voice; voices never share
// oscillator instances. Allocation happens only in the constructor (during
// voice-pool setup), never in the audio callback.
// -----------------------------------------------------------------------------
class SynthVoice
{
public:
    SynthVoice (dsp::WavetableData* wavetableData);
    ~SynthVoice() = default;

    void setSampleRate (double sampleRate);
    void reset();

    // Voice lifecycle.
    void noteOn (int midiNote, float velocity, double sampleRate);
    void noteOff();
    void setPitchBendSemitones (float semitones);
    void updateKeyTrackRatio() noexcept;

    // Called once per block on the audio thread before rendering samples.
    void updateParameters (const SynthParameters& params);

    // Render one stereo sample. Returns silence when the voice is idle.
    dsp::StereoFrame render();

    bool isActive() const noexcept { return m_active; }
    bool isIdle() const noexcept { return !m_active && !m_ampEnv.isActive(); }
    bool isInRelease() const noexcept { return m_ampEnv.isInRelease(); }
    int getNote() const noexcept { return m_note; }
    int getNoteOnOrder() const noexcept { return m_noteOnOrder; }
    void setNoteOnOrder (int order) noexcept { m_noteOnOrder = order; }
    bool isPlayingNote (int midiNote) const noexcept { return m_active && m_note == midiNote; }

    // Access the engine in a given slot (0..5). Used by the routing graph
    // to bind engine nodes for modulation signal transport.
    dsp::OscillatorEngine* getOscillatorEngine (int slot) noexcept
    {
        return m_oscillators[slot].get();
    }

private:
    void updateOscillatorFrequencies();
    float noteToFrequency (int midiNote) const noexcept;

    dsp::WavetableData* m_wavetableData = nullptr;

    // Six independent oscillator engine slots. Each slot hosts its own engine
    // (VA, Wavetable, FM, Additive, Granular) selected per slot; no slot
    // depends on any other slot's engine.
    std::unique_ptr<dsp::OscillatorEngine> m_oscillators[SynthParameters::NumOscillators];

    dsp::ADSREnvelope m_ampEnv;
    dsp::StateVariableFilter m_filterLeft;
    dsp::StateVariableFilter m_filterRight;

    int m_note = -1;
    float m_velocity = 0.0f;
    float m_pitchBendSemitones = 0.0f;
    bool m_active = false;
    bool m_triggered = false;
    int m_noteOnOrder = 0; // monotonic age for deterministic voice stealing

    // Filter state (per voice so key tracking can be applied per note).
    bool m_filterEnabled = true;
    float m_filterCutoffBase = 2000.0f;
    float m_filterKeyTrack = 0.0f;
    float m_keyTrackRatio = 1.0f; // pré-calculado em noteOn() e updateParameters()

    double m_sampleRate = 44100.0;
};

} // namespace omnivy
