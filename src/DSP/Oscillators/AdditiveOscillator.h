#pragma once

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Utilities/Smoothing.h"

#include <cmath>
#include <algorithm>
#include <array>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Maximum number of harmonic partials per additive oscillator.
// 256 partials provides ample spectral richness while remaining CPU-affordable
// thanks to the active-partial strategy (only harmonics below Nyquist are
// rendered).
// -----------------------------------------------------------------------------
static constexpr int kMaxAdditivePartials = 256;

// -----------------------------------------------------------------------------
// A single harmonic partial: amplitude, phase, and frequency ratio relative to
// the fundamental. Phase is stored independently per partial so that complex
// spectral shapes can be reconstructed accurately.
// -----------------------------------------------------------------------------
struct AdditivePartial
{
    float amplitude = 0.0f;   // 0..1
    float phase = 0.0f;       // 0..1
    float frequencyRatio = 1.0f; // multiplier of fundamental
};

// -----------------------------------------------------------------------------
// AdditiveOscillator — genuine additive synthesis engine.
//
// Builds waveforms by summing sine-wave partials. Each partial has independent
// amplitude and phase. Two spectra (A and B) can be morphed between with a
// continuous morph control. Anti-aliasing is performed by rendering only
// partials whose frequency is below Nyquist.
//
// All per-harmonic data is preallocated (fixed std::array of kMaxAdditivePartials).
// Spectral editing (amplitude/phase changes, morphing) is intended to happen
// off the audio thread; the audio thread reads only the precomputed per-block
// values.
// -----------------------------------------------------------------------------
class AdditiveOscillator : public OscillatorBase
{
public:
    AdditiveOscillator();
    ~AdditiveOscillator() override = default;

    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setFrequency (float frequencyHz) override;
    float getFrequency() const override;

    void setWaveform (Waveform) override { /* additive ignores waveform */ }
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

    // ---- Additive-specific parameters ------------------------------------
    void setHarmonicCount (int count);
    int getHarmonicCount() const noexcept { return m_harmonicCount; }

    void setSpectralMorph (float morph);   // 0 = Spectrum A, 1 = Spectrum B
    float getSpectralMorph() const noexcept { return m_spectralMorph; }

    void setSpectralTilt (float tilt);     // -1..1, negative = darker
    float getSpectralTilt() const noexcept { return m_spectralTilt; }

    void setOddEvenBalance (float balance); // -1..1, -1 = odd only, 1 = even only
    float getOddEvenBalance() const noexcept { return m_oddEvenBalance; }

    void setFundamentalLevel (float level);
    float getFundamentalLevel() const noexcept { return m_fundamentalLevel; }

    void setOutputLevel (float level);
    float getOutputLevel() const noexcept { return m_outputLevel; }

    // ---- Per-harmonic access (intended for off-thread editing) -----------
    void setPartialAmplitude (int index, float amplitude);
    void setPartialPhase (int index, float phase);
    void setPartialFrequencyRatio (int index, float ratio);

    float getPartialAmplitude (int index) const noexcept;
    float getPartialPhase (int index) const noexcept;
    float getPartialFrequencyRatio (int index) const noexcept;

    // Spectrum A / B access
    void setSpectrumAPartial (int index, const AdditivePartial& partial);
    void setSpectrumBPartial (int index, const AdditivePartial& partial);
    const AdditivePartial& getSpectrumAPartial (int index) const noexcept;
    const AdditivePartial& getSpectrumBPartials (int index) const noexcept;

    // ---- Modulation input consumption ------------------------------------
    void setExternalAudioInput (float sample) noexcept override;
    void setFrequencyModInput (float mod) noexcept override;
    void setPhaseModInput (float mod) noexcept override;
    void setRingModInput (float mod) noexcept override;
    void setHardSyncTrigger (bool trigger) noexcept override;

    StereoFrame processSample() override;

private:
    // Recompute per-block rendered amplitudes/phases from spectra + morph.
    // Called from processSample() after mod inputs are applied.
    void updatePartialsForBlock();

    float effectiveFrequency() const noexcept;

    float m_frequencyHz = 440.0f;

    // ---- Additive engine state -------------------------------------------
    int m_harmonicCount = 8;
    float m_spectralMorph = 0.0f;     // 0 = Spectrum A, 1 = Spectrum B
    float m_spectralTilt = 0.0f;      // -1..1
    float m_oddEvenBalance = 0.0f;    // -1..1
    float m_fundamentalLevel = 1.0f;
    float m_outputLevel = 0.7f;

    // ---- Partial storage (preallocated) ----------------------------------
    AdditivePartial m_spectrumA[kMaxAdditivePartials];
    AdditivePartial m_spectrumB[kMaxAdditivePartials];

    // Rendered per-block values (computed once per block from spectra+morph).
    float m_renderedAmplitude[kMaxAdditivePartials];
    float m_renderedPhase[kMaxAdditivePartials];
    float m_renderedFrequencyRatio[kMaxAdditivePartials];

    int m_activePartials = 0; // count of harmonics below Nyquist

    // ---- Oscillator state ------------------------------------------------
    float m_phase = 0.0f;             // fundamental phase 0..1
    float m_phaseIncrement = 0.0f;    // phase increment per sample
    float m_initialPhase = 0.0f;
    float m_level = 0.7f;
    float m_pan = 0.0f;
    bool  m_active = true;

    int m_octave = 0;
    int m_semitone = 0;
    float m_detuneCents = 0.0f;
    float m_fineTuneCents = 0.0f;

    // ---- Modulation inputs (accumulated per block) -----------------------
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
