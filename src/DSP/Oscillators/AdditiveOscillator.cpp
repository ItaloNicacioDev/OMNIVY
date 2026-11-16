#include "DSP/Oscillators/AdditiveOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>
#include <algorithm>

namespace omnivy {
namespace dsp {

AdditiveOscillator::AdditiveOscillator()
{
    m_levelSmoother.setSmoothTime (0.002f); // 2 ms level smoothing

    // Initialize Spectrum A as a harmonic series (1/n amplitude decay).
    // Spectrum B starts empty; the user can edit it off-thread.
    for (int i = 0; i < kMaxAdditivePartials; ++i)
    {
        int harmonic = i + 1;
        m_spectrumA[i].amplitude = 1.0f / static_cast<float> (harmonic);
        m_spectrumA[i].phase = 0.0f;
        m_spectrumA[i].frequencyRatio = static_cast<float> (harmonic);

        m_spectrumB[i].amplitude = 0.0f;
        m_spectrumB[i].phase = 0.0f;
        m_spectrumB[i].frequencyRatio = static_cast<float> (harmonic);
    }
}

void AdditiveOscillator::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate (sampleRate);
    setFrequency (m_frequencyHz);
}

void AdditiveOscillator::reset()
{
    m_phase = m_initialPhase;
    m_levelSmoother.reset (m_level);
}

void AdditiveOscillator::setFrequency (float frequencyHz)
{
    m_frequencyHz = frequencyHz;
    m_phaseIncrement = static_cast<float> (effectiveFrequency() / m_sampleRate);
    updatePartialsForBlock();
}

float AdditiveOscillator::getFrequency() const { return m_frequencyHz; }

void AdditiveOscillator::setLevel (float level) { m_level = level; }
float AdditiveOscillator::getLevel() const { return m_level; }

void AdditiveOscillator::setPan (float pan) { m_pan = pan; }
float AdditiveOscillator::getPan() const { return m_pan; }

void AdditiveOscillator::setActive (bool active) { m_active = active; }
bool AdditiveOscillator::isActive() const { return m_active; }

void AdditiveOscillator::setOctave (int octave)       { m_octave = octave; setFrequency (m_frequencyHz); }
void AdditiveOscillator::setSemitone (int semitone)   { m_semitone = semitone; setFrequency (m_frequencyHz); }
void AdditiveOscillator::setDetuneCents (float cents) { m_detuneCents = cents; setFrequency (m_frequencyHz); }
void AdditiveOscillator::setFineTuneCents (float cents) { m_fineTuneCents = cents; setFrequency (m_frequencyHz); }

void AdditiveOscillator::setInitialPhase (float phase)  { m_initialPhase = wrapPhase (phase); m_phase = m_initialPhase; }
float AdditiveOscillator::getPhase() const { return m_phase; }

// ---- Additive-specific parameters ----
void AdditiveOscillator::setHarmonicCount (int count)
{
    m_harmonicCount = std::clamp (count, 1, kMaxAdditivePartials);
    updatePartialsForBlock();
}

void AdditiveOscillator::setSpectralMorph (float morph)
{
    m_spectralMorph = std::clamp (morph, 0.0f, 1.0f);
    updatePartialsForBlock();
}

void AdditiveOscillator::setSpectralTilt (float tilt)
{
    m_spectralTilt = std::clamp (tilt, -1.0f, 1.0f);
    updatePartialsForBlock();
}

void AdditiveOscillator::setOddEvenBalance (float balance)
{
    m_oddEvenBalance = std::clamp (balance, -1.0f, 1.0f);
    updatePartialsForBlock();
}

void AdditiveOscillator::setFundamentalLevel (float level)
{
    m_fundamentalLevel = std::clamp (level, 0.0f, 2.0f);
}

void AdditiveOscillator::setOutputLevel (float level)
{
    m_outputLevel = std::clamp (level, 0.0f, 2.0f);
}

// ---- Per-harmonic access ----
void AdditiveOscillator::setPartialAmplitude (int index, float amplitude)
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return;
    m_spectrumA[index].amplitude = std::clamp (amplitude, 0.0f, 1.0f);
    m_spectrumB[index].amplitude = m_spectrumA[index].amplitude;
    updatePartialsForBlock();
}

void AdditiveOscillator::setPartialPhase (int index, float phase)
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return;
    m_spectrumA[index].phase = wrapPhase (phase);
    m_spectrumB[index].phase = m_spectrumA[index].phase;
    updatePartialsForBlock();
}

void AdditiveOscillator::setPartialFrequencyRatio (int index, float ratio)
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return;
    m_spectrumA[index].frequencyRatio = std::max (0.001f, ratio);
    m_spectrumB[index].frequencyRatio = m_spectrumA[index].frequencyRatio;
    updatePartialsForBlock();
}

float AdditiveOscillator::getPartialAmplitude (int index) const noexcept
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return 0.0f;
    return m_spectrumA[index].amplitude;
}

float AdditiveOscillator::getPartialPhase (int index) const noexcept
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return 0.0f;
    return m_spectrumA[index].phase;
}

float AdditiveOscillator::getPartialFrequencyRatio (int index) const noexcept
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return 0.0f;
    return m_spectrumA[index].frequencyRatio;
}

void AdditiveOscillator::setSpectrumAPartial (int index, const AdditivePartial& partial)
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return;
    m_spectrumA[index] = partial;
    updatePartialsForBlock();
}

void AdditiveOscillator::setSpectrumBPartial (int index, const AdditivePartial& partial)
{
    if (index < 0 || index >= kMaxAdditivePartials)
        return;
    m_spectrumB[index] = partial;
    updatePartialsForBlock();
}

const AdditivePartial& AdditiveOscillator::getSpectrumAPartial (int index) const noexcept
{
    static const AdditivePartial empty;
    if (index < 0 || index >= kMaxAdditivePartials)
        return empty;
    return m_spectrumA[index];
}

const AdditivePartial& AdditiveOscillator::getSpectrumBPartials (int index) const noexcept
{
    static const AdditivePartial empty;
    if (index < 0 || index >= kMaxAdditivePartials)
        return empty;
    return m_spectrumB[index];
}

// ---- Modulation input consumption ----
void AdditiveOscillator::setExternalAudioInput (float sample) noexcept
{
    m_modExternalAudio += sample;
}

void AdditiveOscillator::setFrequencyModInput (float mod) noexcept
{
    m_modFrequency += mod;
}

void AdditiveOscillator::setPhaseModInput (float mod) noexcept
{
    m_modPhase += mod;
}

void AdditiveOscillator::setRingModInput (float mod) noexcept
{
    m_modRing += mod;
}

void AdditiveOscillator::setHardSyncTrigger (bool trigger) noexcept
{
    m_modHardSync = m_modHardSync || trigger;
}

float AdditiveOscillator::effectiveFrequency() const noexcept
{
    const float ratio = centsToRatio (m_detuneCents + m_fineTuneCents)
                      * centsToRatio (static_cast<float> (m_semitone))
                      * static_cast<float> (std::pow (2.0, m_octave));
    return m_frequencyHz * ratio;
}

// ---- Per-block partial preparation (called from processSample) ----
void AdditiveOscillator::updatePartialsForBlock()
{
    const double nyquist = m_sampleRate * 0.49; // stay below Nyquist

    m_activePartials = 0;

    // Morph factor: interpolate between Spectrum A and B.
    const float morph = m_spectralMorph;
    const float invMorph = 1.0f - morph;

    // Tilt: apply spectral shaping (negative = darker, positive = brighter).
    const float tilt = m_spectralTilt;

    // Odd/even balance: -1 = odd only, 0 = neutral, 1 = even only.
    const float oddEven = m_oddEvenBalance;

    for (int i = 0; i < kMaxAdditivePartials; ++i)
    {
        const int harmonic = i + 1;

        if (harmonic > m_harmonicCount)
        {
            m_renderedAmplitude[i] = 0.0f;
            m_renderedPhase[i] = 0.0f;
            m_renderedFrequencyRatio[i] = 0.0f;
            continue;
        }

        // Morph between spectra.
        const float ampA = m_spectrumA[i].amplitude;
        const float ampB = m_spectrumB[i].amplitude;
        const float phaseA = m_spectrumA[i].phase;
        const float phaseB = m_spectrumB[i].phase;
        const float ratioA = m_spectrumA[i].frequencyRatio;
        const float ratioB = m_spectrumB[i].frequencyRatio;

        float amp = ampA * invMorph + ampB * morph;
        float phase = phaseA * invMorph + phaseB * morph;
        float ratio = ratioA * invMorph + ratioB * morph;

        // Apply spectral tilt: amp *= harmonic^tilt (approx).
        if (tilt != 0.0f)
        {
            const float tiltFactor = static_cast<float> (std::pow (static_cast<double> (harmonic), tilt * 0.5));
            amp *= tiltFactor;
        }

        // Apply odd/even balance.
        const bool isOdd = (harmonic % 2) == 1;
        if (oddEven < 0.0f && !isOdd)
            amp *= (1.0f + oddEven); // reduce even
        else if (oddEven > 0.0f && isOdd)
            amp *= (1.0f - oddEven); // reduce odd

        // Apply fundamental level emphasis.
        if (harmonic == 1)
            amp *= m_fundamentalLevel;

        amp = std::clamp (amp, 0.0f, 1.0f);

        const float partialFreq = m_frequencyHz * ratio;

        // Anti-aliasing: only include partials below Nyquist.
        if (partialFreq > nyquist)
        {
            m_renderedAmplitude[i] = 0.0f;
            m_renderedPhase[i] = 0.0f;
            m_renderedFrequencyRatio[i] = 0.0f;
            continue;
        }

        m_renderedAmplitude[i] = amp;
        m_renderedPhase[i] = wrapPhase (phase);
        m_renderedFrequencyRatio[i] = ratio;
        ++m_activePartials;
    }
}

StereoFrame AdditiveOscillator::processSample()
{
    if (!m_active)
        return StereoFrame { 0.0f, 0.0f };

    const float level = m_levelSmoother.getNextValue();

    // Advance fundamental phase with base increment + FM modulation.
    m_phase += m_phaseIncrement + m_modFrequency;

    // ---- Phase modulation ----
    m_phase += m_modPhase;

    // Wrap fundamental phase.
    m_phase = wrapPhase (m_phase);

    // ---- Hard sync ----
    if (m_modHardSync)
    {
        m_phase = m_initialPhase;
        m_modHardSync = false;
    }

    // ---- Sum partials ----
    float mono = 0.0f;

    for (int i = 0; i < m_activePartials; ++i)
    {
        const float amp = m_renderedAmplitude[i];
        if (amp <= 0.0f)
            continue;

        // Partial phase = fundamental phase * frequency ratio + partial phase offset.
        const float partialPhase = wrapPhase (m_phase * m_renderedFrequencyRatio[i] + m_renderedPhase[i]);

        mono += amp * std::sin (static_cast<float> (TwoPi) * partialPhase);
    }

    // ---- Ring modulation ----
    mono *= (1.0f + m_modRing);

    // ---- External audio input ----
    mono += m_modExternalAudio;

    // Apply output level.
    mono *= m_outputLevel;

    const float smoothedLevel = m_levelSmoother.getNextValue();
    return applyPan (mono * smoothedLevel * m_level, m_pan);
}

} // namespace dsp
} // namespace omnivy
