#pragma once

#include "FilterBase.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// State-variable filter. Produces low-pass, high-pass and band-pass outputs
// simultaneously from a single structure. Musical, resonant, and stable.
// -----------------------------------------------------------------------------
class StateVariableFilter : public FilterBase
{
public:
    StateVariableFilter();
    ~StateVariableFilter() override = default;

    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setType (FilterType type) override;
    FilterType getType() const override { return m_type; }

    void setCutoff (float frequencyHz) override;
    float getCutoff() const override { return m_cutoff; }

    void setResonance (float resonance) override;
    float getResonance() const override { return m_resonance; }

    float processSample (float sample) override;

    // Convenience accessors to individual tap outputs (set by last processSample).
    float getLowPass() const  { return m_lp; }
    float getHighPass() const { return m_hp; }
    float getBandPass() const { return m_bp; }

private:
    void updateCoefficients();

    double m_sampleRate = 44100.0;
    FilterType m_type = FilterType::LowPass;
    float m_cutoff = 1000.0f;
    float m_resonance = 0.5f;

    // SVF state.
    double m_lp = 0.0;
    double m_bp = 0.0;
    double m_hp = 0.0;

    double m_f = 0.0;      // filter frequency coefficient
    double m_q = 0.0;      // resonance coefficient
};

} // namespace dsp
} // namespace omnivy
