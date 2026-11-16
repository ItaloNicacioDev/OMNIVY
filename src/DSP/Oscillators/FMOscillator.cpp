#include "DSP/Oscillators/FMOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

FMOscillator::FMOscillator()
{
    m_levelSmoother.setSmoothTime (0.002f); // 2 ms level smoothing
}

void FMOscillator::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate (sampleRate);
    setFrequency (m_frequencyHz);
}

void FMOscillator::reset()
{
    m_phase = m_initialPhase;
    m_phaseIncrement = 0.0f;
    m_previousOutput = 0.0f;
    m_levelSmoother.reset (m_level);
}

void FMOscillator::setFrequency (float frequencyHz)
{
    m_frequencyHz = frequencyHz;
    m_phaseIncrement = effectiveFrequency() / static_cast<float> (m_sampleRate);
}

float FMOscillator::getFrequency() const { return m_frequencyHz; }

void FMOscillator::setLevel (float level) { m_level = level; }
float FMOscillator::getLevel() const { return m_level; }

void FMOscillator::setPan (float pan) { m_pan = pan; }
float FMOscillator::getPan() const { return m_pan; }

void FMOscillator::setActive (bool active) { m_active = active; }
bool FMOscillator::isActive() const { return m_active; }

void FMOscillator::setOctave (int octave)       { m_octave = octave; setFrequency (m_frequencyHz); }
void FMOscillator::setSemitone (int semitone)   { m_semitone = semitone; setFrequency (m_frequencyHz); }
void FMOscillator::setDetuneCents (float cents) { m_detuneCents = cents; setFrequency (m_frequencyHz); }
void FMOscillator::setFineTuneCents (float cents) { m_fineTuneCents = cents; setFrequency (m_frequencyHz); }

void FMOscillator::setInitialPhase (float phase)  { m_initialPhase = wrapPhase (phase); m_phase = m_initialPhase; }
float FMOscillator::getPhase() const { return m_phase; }

void FMOscillator::setFrequencyRatio (float ratio)  { m_frequencyRatio = std::max (0.001f, ratio); setFrequency (m_frequencyHz); }
void FMOscillator::setFineRatio (float fineRatio)   { m_fineRatio = std::max (0.001f, fineRatio); setFrequency (m_frequencyHz); }
void FMOscillator::setFmAmount (float amount)       { m_fmAmount = std::clamp (amount, 0.0f, 100.0f); }
void FMOscillator::setPmAmount (float amount)       { m_pmAmount = std::clamp (amount, 0.0f, 10.0f); }
void FMOscillator::setFeedback (float amount)       { m_feedback = std::clamp (amount, -1.0f, 1.0f); }
void FMOscillator::setOutputLevel (float level)     { m_outputLevel = std::clamp (level, 0.0f, 2.0f); }

float FMOscillator::effectiveFrequency() const noexcept
{
    const float ratio = centsToRatio (m_detuneCents + m_fineTuneCents)
                      * centsToRatio (static_cast<float> (m_semitone))
                      * static_cast<float> (std::pow (2.0, m_octave));
    return m_frequencyHz * m_frequencyRatio * m_fineRatio * ratio;
}

// ---- Modulation input consumption ----
void FMOscillator::setExternalAudioInput (float sample) noexcept
{
    m_modExternalAudio += sample;
}

void FMOscillator::setFrequencyModInput (float mod) noexcept
{
    m_modFrequency += mod;
}

void FMOscillator::setPhaseModInput (float mod) noexcept
{
    m_modPhase += mod;
}

void FMOscillator::setRingModInput (float mod) noexcept
{
    m_modRing += mod;
}

void FMOscillator::setHardSyncTrigger (bool trigger) noexcept
{
    m_modHardSync = m_modHardSync || trigger;
}

StereoFrame FMOscillator::processSample()
{
    if (!m_active)
        return StereoFrame { 0.0f, 0.0f };

    // Advance phase with base increment.
    m_phase += m_phaseIncrement;

    // ---- Audio-rate Frequency Modulation ----
    // freqModInput is added directly to phase increment per sample.
    // This creates true FM: the modulating signal changes the carrier
    // frequency instantaneously.
    m_phase += m_modFrequency * m_fmAmount;

    // ---- Phase Modulation ----
    // phaseModInput is added directly to the phase.
    m_phase += m_modPhase * m_pmAmount;

    // Wrap phase.
    m_phase = wrapPhase (m_phase);

    // ---- Hard Sync ----
    // If the source oscillator triggered hard sync, reset phase to initial.
    if (m_modHardSync)
    {
        m_phase = m_initialPhase;
        m_modHardSync = false; // consume trigger
    }

    // ---- FM synthesis with self-feedback ----
    // The feedback path feeds the previous output back into the phase,
    // creating complex, evolving spectra. Feedback is clamped to prevent
    // runaway oscillation.
    const float feedbackPhase = m_previousOutput * m_feedback;
    const float sineValue = std::sin (static_cast<float> (TwoPi) * (m_phase + feedbackPhase));

    // ---- Ring Modulation ----
    // ringModInput multiplies the output signal.
    const float mono = sineValue * (1.0f + m_modRing) * m_outputLevel;

    // ---- External audio input ----
    // External audio is summed into the output (audio routing).
    const float mixedMono = mono + m_modExternalAudio * m_outputLevel;

    m_previousOutput = mixedMono;

    const float smoothedLevel = m_levelSmoother.getNextValue();
    return applyPan (mixedMono * smoothedLevel, m_pan);
}

} // namespace dsp
} // namespace omnivy
