#pragma once

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Wavetable/WavetableData.h"
#include "DSP/Utilities/Smoothing.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Wavetable oscillator with linear interpolation between samples and optional
// table selection / morphing (table index can be fractional for future morph).
// -----------------------------------------------------------------------------
class WavetableOscillator : public OscillatorBase
{
public:
    WavetableOscillator();
    ~WavetableOscillator() override = default;

    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setFrequency (float frequencyHz) override;
    float getFrequency() const override;

    void setWaveform (Waveform) override { /* wavetable ignores waveform */ }
    Waveform getWaveform() const override { return Waveform::Sine; }

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

    // Select which table to read (0 = first). Morphing support reserved.
    void setTableIndex (int index) { m_tableIndex = index; }

    StereoFrame processSample() override;

    // Attach the shared wavetable data (not owned).
    void setWavetableData (WavetableData* data) { m_wavetable = data; }

private:
    float effectiveFrequency() const noexcept;

    float m_frequencyHz = 440.0f;
    float m_phase = 0.0f;
    float m_phaseIncrement = 0.0f;

    float m_level = 0.7f;
    float m_pan = 0.0f;
    bool  m_active = true;

    int m_octave = 0;
    int m_semitone = 0;
    float m_detuneCents = 0.0f;
    float m_fineTuneCents = 0.0f;
    float m_initialPhase = 0.0f;

    int m_tableIndex = 0;
    WavetableData* m_wavetable = nullptr;

    LinearSmoother m_levelSmoother;
};

} // namespace dsp
} // namespace omnivy
