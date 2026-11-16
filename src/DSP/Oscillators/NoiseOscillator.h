#pragma once

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Utilities/Smoothing.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// White-noise oscillator. Produces a new random sample each cycle. Independent
// state per instance; no shared buffers.
// -----------------------------------------------------------------------------
class NoiseOscillator : public OscillatorBase
{
public:
    NoiseOscillator();
    ~NoiseOscillator() override = default;

    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setFrequency (float frequencyHz) override;
    float getFrequency() const override;

    void setWaveform (Waveform waveform) override;
    Waveform getWaveform() const override;

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

    StereoFrame processSample() override;

private:
    juce::Random m_random;
    float m_level = 0.7f;
    float m_pan = 0.0f;
    bool  m_active = true;
    LinearSmoother m_levelSmoother;
};

} // namespace dsp
} // namespace omnivy
