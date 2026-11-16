#pragma once

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Classic ADSR envelope. Real-time safe, no allocation. Stages: Attack, Decay,
// Sustain, Release. Returns 0..1 amplitude. Call noteOn() on MIDI note-on and
// noteOff() on MIDI note-off; isActive() reports whether the envelope is still
// sounding (used by the voice manager to free voices).
// -----------------------------------------------------------------------------
class ADSREnvelope
{
public:
    enum class Stage : int { Idle, Attack, Decay, Sustain, Release, Finished };

    ADSREnvelope();
    ~ADSREnvelope() = default;

    void setSampleRate (double sampleRate);
    void reset();

    void setAttack (float seconds);
    void setDecay (float seconds);
    void setSustain (float level);   // 0..1
    void setRelease (float seconds);

    // Triggered from the message/audio thread at note boundaries.
    void noteOn();
    void noteOff();

    // Returns the next envelope sample (0..1). Returns 0 and stays Finished
    // once the release has completed.
    float processSample();

    Stage getStage() const noexcept { return m_stage; }
    bool isActive() const noexcept { return m_stage != Stage::Idle && m_stage != Stage::Finished; }
    bool isInRelease() const noexcept { return m_stage == Stage::Release; }
    float getValue() const noexcept { return m_value; }

private:
    void enterStage (Stage newStage);

    double m_sampleRate = 44100.0;
    Stage m_stage = Stage::Idle;

    float m_attack = 0.01f;
    float m_decay = 0.1f;
    float m_sustain = 0.7f;
    float m_release = 0.2f;

    float m_value = 0.0f;
    float m_increment = 0.0f;  // per-sample increment for linear stages
    float m_target = 0.0f;
};

} // namespace dsp
} // namespace omnivy
