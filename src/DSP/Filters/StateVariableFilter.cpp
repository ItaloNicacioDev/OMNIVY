#include "DSP/Filters/StateVariableFilter.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

StateVariableFilter::StateVariableFilter()
{
    updateCoefficients();
}

void StateVariableFilter::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    updateCoefficients();
}

void StateVariableFilter::reset()
{
    m_lp = 0.0;
    m_bp = 0.0;
    m_hp = 0.0;
}

void StateVariableFilter::setType (FilterType type) { m_type = type; }

void StateVariableFilter::setCutoff (float frequencyHz)
{
    m_cutoff = frequencyHz;
    updateCoefficients();
}

void StateVariableFilter::setResonance (float resonance)
{
    m_resonance = resonance;
    updateCoefficients();
}

void StateVariableFilter::updateCoefficients()
{
    // Clamp cutoff to a sane range relative to sample rate.
    const double sr = m_sampleRate > 0.0 ? m_sampleRate : 44100.0;
    double fc = std::clamp (static_cast<double> (m_cutoff), 20.0, sr * 0.49);

    // Warp-free-ish approximation for the SVF.
    m_f = 2.0 * std::sin (Pi * fc / sr);

    // Resonance: q in [0, 2], mapped to damping. Higher q = more resonance.
    double q = std::clamp (static_cast<double> (m_resonance), 0.0, 2.0);
    m_q = 1.0 / (1.0 + q); // damping factor
}

float StateVariableFilter::processSample (float sample)
{
    const double input = static_cast<double> (sample);

    // SVF difference equations.
    m_hp = input - m_lp - m_q * m_bp;
    m_bp = m_f * m_hp + m_bp;
    m_lp = m_f * m_bp + m_lp;

    // Clamp to avoid runaway resonance at extreme settings.
    m_lp = std::clamp (m_lp, -10.0, 10.0);
    m_bp = std::clamp (m_bp, -10.0, 10.0);
    m_hp = std::clamp (m_hp, -10.0, 10.0);

    switch (m_type)
    {
        case FilterType::LowPass:   return static_cast<float> (m_lp);
        case FilterType::HighPass:  return static_cast<float> (m_hp);
        case FilterType::BandPass:  return static_cast<float> (m_bp);
        default:                    return static_cast<float> (m_lp);
    }
}

} // namespace dsp
} // namespace omnivy
