#include "DSP/Wavetable/WavetableOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

WavetableOscillator::WavetableOscillator()
{
    m_levelSmoother.setSmoothTime (0.002f);
}

void WavetableOscillator::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate (sampleRate);
    setFrequency (m_frequencyHz);
}

void WavetableOscillator::reset() { m_phase = m_initialPhase; m_levelSmoother.reset (m_level); }

void WavetableOscillator::setFrequency (float frequencyHz)
{
    m_frequencyHz = frequencyHz;
    m_phaseIncrement = effectiveFrequency() / static_cast<float> (m_sampleRate);
}

float WavetableOscillator::getFrequency() const { return m_frequencyHz; }

void WavetableOscillator::setLevel (float level) { m_level = level; }
float WavetableOscillator::getLevel() const { return m_level; }

void WavetableOscillator::setPan (float pan) { m_pan = pan; }
float WavetableOscillator::getPan() const { return m_pan; }

void WavetableOscillator::setActive (bool active) { m_active = active; }
bool WavetableOscillator::isActive() const { return m_active; }

void WavetableOscillator::setOctave (int octave)       { m_octave = octave; setFrequency (m_frequencyHz); }
void WavetableOscillator::setSemitone (int semitone)   { m_semitone = semitone; setFrequency (m_frequencyHz); }
void WavetableOscillator::setDetuneCents (float cents) { m_detuneCents = cents; setFrequency (m_frequencyHz); }
void WavetableOscillator::setFineTuneCents (float cents) { m_fineTuneCents = cents; setFrequency (m_frequencyHz); }

void WavetableOscillator::setInitialPhase (float phase) { m_initialPhase = wrapPhase (phase); m_phase = m_initialPhase; }
float WavetableOscillator::getPhase() const { return m_phase; }

float WavetableOscillator::effectiveFrequency() const noexcept
{
    const float ratio = centsToRatio (m_detuneCents + m_fineTuneCents)
                      * centsToRatio (static_cast<float> (m_semitone))
                      * static_cast<float> (std::pow (2.0, m_octave));
    return m_frequencyHz * ratio;
}

StereoFrame WavetableOscillator::processSample()
{
    if (!m_active || m_wavetable == nullptr || m_wavetable->getTableSize() <= 0)
        return StereoFrame { 0.0f, 0.0f };

    const float level = m_levelSmoother.getNextValue();

    // Advance phase.
    m_phase += m_phaseIncrement;
    if (m_phase >= 1.0f) m_phase -= 1.0f;
    if (m_phase < 0.0f) m_phase += 1.0f;

    const int size = m_wavetable->getTableSize();
    const int mask = m_wavetable->getTableSizeMask();
    const float* table = m_wavetable->getTable (m_tableIndex);

    // Linear interpolation between adjacent samples.
    const double pos = m_phase * static_cast<double> (size);
    const int i0 = static_cast<int> (pos) & mask;
    const int i1 = (i0 + 1) & mask;
    const float frac = static_cast<float> (pos - static_cast<double> (i0));

    const float s0 = table[i0];
    const float s1 = table[i1];
    const float mono = s0 + frac * (s1 - s0);

    return applyPan (mono * level, m_pan);
}

} // namespace dsp
} // namespace omnivy
