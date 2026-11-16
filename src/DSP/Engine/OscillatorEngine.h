#pragma once

#include <memory>

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Wavetable/WavetableData.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Synthesis engine type. Each oscillator slot can independently select any
// engine; engines are fully self-contained and never depend on each other.
// -----------------------------------------------------------------------------
enum class EngineType : int
{
    VA = 0,          // Virtual Analog
    Wavetable = 1,   // Wavetable
    FM = 2,          // Frequency Modulation (P3.3)
    Additive = 3,    // Additive (P3.4)
    Granular = 4,    // Granular (P3.5)

    Count
};

// -----------------------------------------------------------------------------
// OscillatorEngine is the universal host for one oscillator slot. It owns a
// single OscillatorBase instance and forwards all control calls to it, while
// also providing the modulation input bus used by the routing graph.
//
// Engines are interchangeable: swapping the engine type replaces the wrapped
// oscillator without touching any other slot or layer.
// -----------------------------------------------------------------------------
class OscillatorEngine
{
public:
    OscillatorEngine();
    ~OscillatorEngine() = default;

    OscillatorEngine (const OscillatorEngine&) = delete;
    OscillatorEngine& operator= (const OscillatorEngine&) = delete;

    // Initialise with shared wavetable data (not owned). Called once during
    // voice-pool setup, never on the audio thread.
    void init (dsp::WavetableData* wavetableData);

    void setSampleRate (double sampleRate);
    void reset();

    void setEngineType (EngineType type);
    EngineType getEngineType() const noexcept { return m_engineType; }

    // ---- Oscillator control (delegated to wrapped oscillator) --------------
    void setFrequency (float frequencyHz);
    float getFrequency() const;
    void setWaveform (Waveform waveform);
    Waveform getWaveform() const;
    void setLevel (float level);
    float getLevel() const;
    void setPan (float pan);
    float getPan() const;
    void setActive (bool active);
    bool isActive() const;
    void setOctave (int octave);
    void setSemitone (int semitone);
    void setDetuneCents (float cents);
    void setFineTuneCents (float cents);
    void setInitialPhase (float phase);
    float getPhase() const;

    // ---- Modulation inputs (real-time safe, lock-free writes) --------------
    // Written by the routing/modulation layer. Values accumulate across
    // multiple connections targeting the same input; reset each block via
    // resetModInputs(). Default no-op for engines that don't consume a given
    // input; FM/PM/Ring/Sync engines override in later steps.
    virtual void setExternalAudioInput (float sample) noexcept;
    virtual void setFrequencyModInput (float mod) noexcept;
    virtual void setPhaseModInput (float mod) noexcept;
    virtual void setRingModInput (float mod) noexcept;
    virtual void setHardSyncTrigger (bool trigger) noexcept;

    // Render one stereo frame. Each engine applies its own level/pan.
    StereoFrame processSample();

    // Last rendered output (for audio connections to read). Updated each
    // processSample() call.
    StereoFrame getLastOutput() const noexcept { return m_lastOutput; }

    // Reset all modulation inputs to zero/default. Called at the start of
    // each audio block before the routing graph applies new values.
    void resetModInputs() noexcept;

    // Attach shared wavetable data (not owned).
    void setWavetableData (dsp::WavetableData* data);

    // ---- FM parameter forwarding (no-op for non-FM engines) ---------------
    // Forwarded to the wrapped FM oscillator when the engine type is FM;
    // no-op otherwise. This keeps SynthVoice independent of concrete engine
    // classes while allowing per-slot FM configuration.
    void setFrequencyRatio (float ratio);
    float getFrequencyRatio() const noexcept;
    void setFineRatio (float fineRatio);
    float getFineRatio() const noexcept;
    void setFmAmount (float amount);
    float getFmAmount() const noexcept;
    void setPmAmount (float amount);
    float getPmAmount() const noexcept;
    void setFeedback (float amount);
    float getFeedback() const noexcept;
    void setOutputLevel (float level);
    float getOutputLevel() const noexcept;

private:
    void recreateOscillator();

    EngineType m_engineType = EngineType::VA;
    std::unique_ptr<OscillatorBase> m_oscillator;
    dsp::WavetableData* m_wavetableData = nullptr;
    double m_sampleRate = 44100.0;
    StereoFrame m_lastOutput { 0.0f, 0.0f };

    // Accumulated modulation inputs for this block. Multiple connections
    // targeting the same engine/input are summed here by the routing graph.
    float m_modExternalAudio { 0.0f };
    float m_modFrequency { 0.0f };
    float m_modPhase { 0.0f };
    float m_modRing { 0.0f };
    bool  m_modHardSync { false };
};

} // namespace dsp
} // namespace omnivy
