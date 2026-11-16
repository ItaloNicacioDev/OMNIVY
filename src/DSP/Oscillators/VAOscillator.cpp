#include "DSP/Oscillators/VAOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

VAOscillator::VAOscillator()
{
    m_levelSmoother.setSmoothTime (0.002f); // 2 ms level smoothing
}

void VAOscillator::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate (sampleRate);
    setFrequency (m_frequencyHz);
}

void VAOscillator::reset()
{
    m_phase = m_initialPhase;
    m_levelSmoother.reset (m_level);
}

void VAOscillator::setFrequency (float frequencyHz)
{
    m_frequencyHz = frequencyHz;
    m_phaseIncrement = effectiveFrequency() / static_cast<float> (m_sampleRate);
}

float VAOscillator::getFrequency() const { return m_frequencyHz; }

void VAOscillator::setWaveform (Waveform waveform) { m_waveform = waveform; }
Waveform VAOscillator::getWaveform() const { return m_waveform; }

void VAOscillator::setLevel (float level) { m_level = level; }
float VAOscillator::getLevel() const { return m_level; }

void VAOscillator::setPan (float pan) { m_pan = pan; }
float VAOscillator::getPan() const { return m_pan; }

void VAOscillator::setActive (bool active) { m_active = active; }
bool VAOscillator::isActive() const { return m_active; }

void VAOscillator::setOctave (int octave)       { m_octave = octave; setFrequency (m_frequencyHz); }
void VAOscillator::setSemitone (int semitone)   { m_semitone = semitone; setFrequency (m_frequencyHz); }
void VAOscillator::setDetuneCents (float cents) { m_detuneCents = cents; setFrequency (m_frequencyHz); }
void VAOscillator::setFineTuneCents (float cents) { m_fineTuneCents = cents; setFrequency (m_frequencyHz); }

void VAOscillator::setInitialPhase (float phase)  { m_initialPhase = wrapPhase (phase); m_phase = m_initialPhase; }
float VAOscillator::getPhase() const { return m_phase; }

float VAOscillator::effectiveFrequency() const noexcept
{
    const float ratio = centsToRatio (m_detuneCents + m_fineTuneCents)
                      * centsToRatio (static_cast<float> (m_semitone))
                      * static_cast<float> (std::pow (2.0, m_octave));
    return m_frequencyHz * ratio;
}

StereoFrame VAOscillator::processSample()
{
    if (!m_active)
        return StereoFrame { 0.0f, 0.0f };

    const float level = m_levelSmoother.getNextValue();

    // Advance phase and wrap.
    m_phase += m_phaseIncrement;
    if (m_phase >= 1.0f) m_phase -= 1.0f;
    if (m_phase < 0.0f) m_phase += 1.0f;

    float mono = 0.0f;

    switch (m_waveform)
    {
        case Waveform::Sine:
            mono = std::sin (static_cast<float> (TwoPi) * m_phase);
            break;

        case Waveform::Saw:
            // Classic analog-style saw: -1 at phase 0, +1 at phase 1.
            mono = 2.0f * m_phase - 1.0f;
            break;

        case Waveform::Square:
            mono = (m_phase < 0.5f) ? 1.0f : -1.0f;
            break;

        case Waveform::Triangle:
        {
            float t = m_phase * 2.0f;
            if (t > 1.0f) t = 2.0f - t;
            mono = t * 2.0f - 1.0f;
            break;
        }

        case Waveform::Pulse:
            // 50% width pulse = square.
            mono = (m_phase < 0.5f) ? 1.0f : -1.0f;
            break;

        case Waveform::Noise:
        default:
            mono = 0.0f; // noise handled by NoiseOscillator
            break;
    }

    return applyPan (mono * level, m_pan);
}

} // namespace dsp
} // namespace omnivy
