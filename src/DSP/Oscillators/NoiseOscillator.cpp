#include "DSP/Oscillators/NoiseOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

namespace omnivy {
namespace dsp {

NoiseOscillator::NoiseOscillator()
{
    m_levelSmoother.setSmoothTime (0.002f);
}

void NoiseOscillator::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate (sampleRate);
}

void NoiseOscillator::reset() { m_levelSmoother.reset (m_level); }

void NoiseOscillator::setFrequency (float) {}
float NoiseOscillator::getFrequency() const { return 0.0f; }

void NoiseOscillator::setWaveform (Waveform) {}
Waveform NoiseOscillator::getWaveform() const { return Waveform::Noise; }

void NoiseOscillator::setLevel (float level) { m_level = level; }
float NoiseOscillator::getLevel() const { return m_level; }

void NoiseOscillator::setPan (float pan) { m_pan = pan; }
float NoiseOscillator::getPan() const { return m_pan; }

void NoiseOscillator::setActive (bool active) { m_active = active; }
bool NoiseOscillator::isActive() const { return m_active; }

void NoiseOscillator::setOctave (int) {}
void NoiseOscillator::setSemitone (int) {}
void NoiseOscillator::setDetuneCents (float) {}
void NoiseOscillator::setFineTuneCents (float) {}

void NoiseOscillator::setInitialPhase (float) {}
float NoiseOscillator::getPhase() const { return 0.0f; }

StereoFrame NoiseOscillator::processSample()
{
    if (!m_active)
        return StereoFrame { 0.0f, 0.0f };

    const float level = m_levelSmoother.getNextValue();
    const float mono = m_random.nextFloat() * 2.0f - 1.0f;
    return applyPan (mono * level, m_pan);
}

} // namespace dsp
} // namespace omnivy
