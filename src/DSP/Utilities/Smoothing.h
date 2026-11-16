#pragma once

#include <cmath>
#include "DSPUtilities.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// One-pole (exponential) smoother. Click-free parameter changes on the audio
// thread. Allocated once, no per-sample allocation.
// -----------------------------------------------------------------------------
class LinearSmoother
{
public:
    LinearSmoother() = default;

    void setSampleRate (double sampleRate) noexcept
    {
        m_sampleRate = sampleRate;
        recalculate();
    }

    void setSmoothTime (float seconds) noexcept
    {
        m_smoothTime = seconds;
        recalculate();
    }

    // Call once per block before rendering.
    void setTarget (float value) noexcept { m_target = value; }

    // Returns the next smoothed sample.
    float getNextValue() noexcept
    {
        m_current += (m_target - m_current) * m_coeff;
        return m_current;
    }

    float getCurrent() const noexcept { return m_current; }
    float getTarget() const noexcept { return m_target; }

    void reset (float value) noexcept
    {
        m_current = value;
        m_target = value;
    }

    bool isSmoothing() const noexcept
    {
        return std::fabs (m_target - m_current) > 1e-6f;
    }

private:
    void recalculate() noexcept
    {
        if (m_sampleRate > 0.0 && m_smoothTime > 0.0f)
            m_coeff = static_cast<float> (1.0 - std::exp (-1.0 / (m_smoothTime * m_sampleRate)));
        else
            m_coeff = 1.0f;
    }

    double m_sampleRate = 44100.0;
    float m_smoothTime = 0.005f; // 5 ms default
    float m_coeff = 1.0f;
    float m_current = 0.0f;
    float m_target = 0.0f;
};

} // namespace dsp
} // namespace omnivy
