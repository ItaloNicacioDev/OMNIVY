#pragma once

#include <memory>
#include "DSP/Utilities/DSPUtilities.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Abstract oscillator interface. Every oscillator owns its own phase and state;
// instances never share internal state. All methods are real-time safe and
// perform no dynamic allocation.
// -----------------------------------------------------------------------------
class OscillatorBase
{
public:
    virtual ~OscillatorBase() = default;

    virtual void setSampleRate (double sampleRate) = 0;
    virtual void reset() = 0;

    // Set the base note frequency (Hz). Detune/octave/semitone are applied on
    // top of this by derived classes.
    virtual void setFrequency (float frequencyHz) = 0;
    virtual float getFrequency() const = 0;

    virtual void setWaveform (Waveform waveform) = 0;
    virtual Waveform getWaveform() const = 0;

    virtual void setLevel (float level) = 0;
    virtual float getLevel() const = 0;
    virtual void setPan (float pan) = 0;
    virtual float getPan() const = 0;
    virtual void setActive (bool active) = 0;
    virtual bool isActive() const = 0;

    // Pitch offsets in semitones / cents.
    virtual void setOctave (int octave) = 0;
    virtual void setSemitone (int semitone) = 0;
    virtual void setDetuneCents (float cents) = 0;
    virtual void setFineTuneCents (float cents) = 0;

    // Initial phase (0..1) applied at note start.
    virtual void setInitialPhase (float phase) = 0;
    virtual float getPhase() const = 0;

    // Render one stereo frame. Level and pan are applied internally so each
    // oscillator is fully independent and self-contained.
    virtual StereoFrame processSample() = 0;

protected:
    double m_sampleRate = 44100.0;
};

} // namespace dsp
} // namespace omnivy
