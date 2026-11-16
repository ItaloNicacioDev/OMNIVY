#include "DSP/Envelopes/ADSREnvelope.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

ADSREnvelope::ADSREnvelope() {}

void ADSREnvelope::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
}

void ADSREnvelope::reset()
{
    m_value = 0.0f;
    m_stage = Stage::Idle;
    m_increment = 0.0f;
    m_target = 0.0f;
}

void ADSREnvelope::setAttack (float seconds)  { m_attack  = std::max (0.0001f, seconds); }
void ADSREnvelope::setDecay (float seconds)   { m_decay   = std::max (0.0001f, seconds); }
void ADSREnvelope::setSustain (float level)   { m_sustain = std::clamp (level, 0.0f, 1.0f); }
void ADSREnvelope::setRelease (float seconds) { m_release = std::max (0.0001f, seconds); }

void ADSREnvelope::enterStage (Stage newStage)
{
    m_stage = newStage;

    switch (newStage)
    {
        case Stage::Attack:
            // Linear ramp from 0 to 1 over attack seconds.
            m_increment = (m_attack > 0.0f) ? (1.0f / (m_attack * static_cast<float> (m_sampleRate)))
                                            : 1.0f;
            m_target = 1.0f;
            break;
        case Stage::Decay:
            // Linear ramp from current value down to sustain over decay seconds.
            m_increment = (m_decay > 0.0f) ? ((m_sustain - m_value) / (m_decay * static_cast<float> (m_sampleRate)))
                                           : (m_sustain - m_value);
            m_target = m_sustain;
            break;
        case Stage::Sustain:
            m_increment = 0.0f;
            m_target = m_sustain;
            break;
        case Stage::Release:
            // Linear ramp from current value down to 0 over release seconds.
            m_increment = (m_release > 0.0f) ? ((0.0f - m_value) / (m_release * static_cast<float> (m_sampleRate)))
                                             : -m_value;
            m_target = 0.0f;
            break;
        case Stage::Finished:
        case Stage::Idle:
            m_increment = 0.0f;
            m_target = 0.0f;
            break;
    }
}

void ADSREnvelope::noteOn()
{
    m_value = 0.0f;
    enterStage (Stage::Attack);
}

void ADSREnvelope::noteOff()
{
    if (m_stage == Stage::Idle)
        return; // nothing sounding
    enterStage (Stage::Release);
}

float ADSREnvelope::processSample()
{
    switch (m_stage)
    {
        case Stage::Attack:
        case Stage::Decay:
        case Stage::Release:
            m_value += m_increment;
            // Clamp to target depending on direction.
            if (m_increment >= 0.0f)
            {
                if (m_value >= m_target) { m_value = m_target; enterStage (Stage::Sustain); }
            }
            else
            {
                if (m_value <= m_target)
                {
                    m_value = m_target;
                    if (m_stage == Stage::Release)
                        enterStage (Stage::Finished);
                    else
                        enterStage (Stage::Sustain);
                }
            }
            break;
        case Stage::Sustain:
            m_value = m_sustain;
            break;
        case Stage::Idle:
        case Stage::Finished:
            m_value = 0.0f;
            break;
    }

    return std::clamp (m_value, 0.0f, 1.0f);
}

} // namespace dsp
} // namespace omnivy
