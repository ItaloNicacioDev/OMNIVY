#pragma once

#include <vector>
#include <algorithm>
#include <memory>

#include "DSP/Engine/SynthVoice.h"

namespace omnivy {

// -----------------------------------------------------------------------------
// Voice manager. Owns a fixed, pre-allocated pool of voices (up to 64) so the
// audio callback never allocates. Implements note-on allocation, note-off
// release, intelligent voice stealing, and stuck-note prevention.
//
// Stealing strategy (deterministic):
//   1. Prefer a voice already in release with the lowest envelope value (almost
//      silent, safe to steal).
//   2. Otherwise steal the oldest active voice (FIFO by note-on time).
//   3. Re-trigger an already-playing voice for the same note instead of
//      allocating a new one (prevents duplicate voices / stuck notes).
// -----------------------------------------------------------------------------
class VoiceManager
{
public:
    VoiceManager (dsp::WavetableData* wavetableData);
    ~VoiceManager() = default;

    VoiceManager (const VoiceManager&) = delete;
    VoiceManager& operator= (const VoiceManager&) = delete;

    void setSampleRate (double sampleRate);
    void setPolyphony (int polyphony);

    // MIDI voice control.
    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void noteOffAll();
    void reset();

    // Pitch bend applied to all active voices (semitones).
    void setPitchBendSemitones (float semitones);

    // Called once per block on the audio thread before rendering samples.
    void updateParameters (const SynthParameters& params);

    // Render one stereo sample by summing all active voices.
    dsp::StereoFrame renderNextSample();

    int getNumActiveVoices() const noexcept;
    int getPolyphony() const noexcept { return m_polyphony; }
    bool isVoicePlayingNote (int midiNote) const noexcept;

private:
    SynthVoice* findVoiceForNote (int midiNote) noexcept;
    SynthVoice* findFreeVoice() noexcept;
    SynthVoice* selectStealVictim() noexcept;

    dsp::WavetableData* m_wavetableData = nullptr;
    std::vector<std::unique_ptr<SynthVoice>> m_voices;
    int m_polyphony = 16;
    float m_pitchBendSemitones = 0.0f;
    int m_noteOnCounter = 0; // monotonic, for deterministic FIFO stealing
    double m_sampleRate = 44100.0;
};

} // namespace omnivy
