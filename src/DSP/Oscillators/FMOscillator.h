#pragma once

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Utilities/Smoothing.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// FM (Frequency Modulation) operator engine.
//
// Implements genuine audio-rate FM synthesis: a carrier phase accumulator is
// modulated by an external audio-rate signal applied to the frequency mod
// input, with optional self-feedback and phase modulation. Each instance owns
// independent phase, feedback, and modulation state, so multiple FM operators
// can modulate each other in arbitrary configurations without cross-talk.
//
// The FM formula:
//   phase_increment = base_freq * ratio * fine_ratio / sample_rate
//   phase_increment += freqModInput * fmAmount          (audio-rate FM)
//   phase += phase_increment
//   output = sin(2*Pi*phase + phaseModInput * pmAmount + feedback * prevOut)
// -----------------------------------------------------------------------------
class FMOscillator : public OscillatorBase
{
public:
    FMOscillator();
    ~FMOscillator() override = default;

    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setFrequency (float frequencyHz) override;
    float getFrequency() const override;

    void setWaveform (Waveform) override { /* FM ignores waveform */ }
    Waveform getWaveform() const override { return Waveform::Sine; }

    void setLevel (float level) override;
    float getLevel() const override;
    void setPan (float pan) override;
    float getPan() const override;
    void setActive (bool active) override;
    bool isActive() const override;

    void setOctave (int octave) override;
    void setSemitone (int semitone) override;
    void setDetuneCents (float cents) override;
    void setFineTuneCents (float cents) override;

    void setInitialPhase (float phase) override;
    float getPhase() const override;

    // ---- FM-specific parameters ------------------------------------------
    void setFrequencyRatio (float ratio);
    float getFrequencyRatio() const noexcept { return m_frequencyRatio; }

    void setFineRatio (float fineRatio);
    float getFineRatio() const noexcept { return m_fineRatio; }

    void setFmAmount (float amount);
    float getFmAmount() const noexcept { return m_fmAmount; }

    void setPmAmount (float amount);
    float getPmAmount() const noexcept { return m_pmAmount; }

    void setFeedback (float amount);
    float getFeedback() const noexcept { return m_feedback; }

    void setOutputLevel (float level);
    float getOutputLevel() const noexcept { return m_outputLevel; }

    // ---- Modulation input consumption ------------------------------------
    // These override the base-class no-ops to store values consumed during
    // processSample(). Values are accumulated per block by OscillatorEngine.
    void setExternalAudioInput (float sample) noexcept override;
    void setFrequencyModInput (float mod) noexcept override;
    void setPhaseModInput (float mod) noexcept override;
    void setRingModInput (float mod) noexcept override;
    void setHardSyncTrigger (bool trigger) noexcept override;

    StereoFrame processSample() override;

private:
    float effectiveFrequency() const noexcept;

    float m_frequencyHz = 440.0f;

    float m_frequencyRatio = 1.0f;   // carrier:modulator ratio
    float m_fineRatio = 1.0f;        // fine ratio multiplier
    float m_fmAmount = 0.0f;         // FM modulation index
    float m_pmAmount = 0.0f;         // PM modulation amount
    float m_feedback = 0.0f;         // self-feedback amount
    float m_outputLevel = 0.7f;      // output gain

    float m_phase = 0.0f;            // 0..1 phase accumulator
    float m_phaseIncrement = 0.0f;
    float m_previousOutput = 0.0f;   // for self-feedback

    float m_level = 0.7f;
    float m_pan = 0.0f;
    bool  m_active = true;

    int m_octave = 0;
    int m_semitone = 0;
    float m_detuneCents = 0.0f;
    float m_fineTuneCents = 0.0f;
    float m_initialPhase = 0.0f;

    // Modulation inputs accumulated for this block (set by OscillatorEngine
    // just before processSample() is called).
    float m_modExternalAudio { 0.0f };
    float m_modFrequency { 0.0f };
    float m_modPhase { 0.0f };
    float m_modRing { 0.0f };
    bool  m_modHardSync { false };

    LinearSmoother m_levelSmoother;

    double m_sampleRate = 44100.0;
};

} // namespace dsp
} // namespace omnivy
