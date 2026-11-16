#pragma once

#include <atomic>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Engine/OscillatorEngine.h"
#include "ParameterIDs.h"

namespace omnivy {

// -----------------------------------------------------------------------------
// Holds the current value of every synth parameter as lock-free atomics so the
// DSP engine can read them on the audio thread without mutexes. The
// AudioProcessor owns the juce::AudioProcessorValueTreeState and calls
// updateFromValueTreeState() at the start of each processBlock.
// -----------------------------------------------------------------------------
class SynthParameters
{
public:
    SynthParameters();
    ~SynthParameters() = default;

    // Total number of independent oscillators (mirrors params::NumOscillators
    // so the engine can reference SynthParameters::NumOscillators).
    static constexpr int NumOscillators = params::NumOscillators;

    // Builds the APVTS ParameterLayout. Called by the plugin constructor.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Reads values from the APVTS (lock-free atomic loads). Call once per block
    // on the audio thread before rendering.
    void updateFromValueTreeState (juce::AudioProcessorValueTreeState& vts);

    // ---- Accessors ------------------------------------------------------------
    float getMasterVolume() const noexcept { return m_masterVolume; }
    int   getPolyphony() const noexcept { return m_polyphony; }

    struct Oscillator
    {
        bool active  = true;
        int  waveform = 0;          // Waveform enum value
        float level  = 0.7f;
        float pan    = 0.0f;
        int  octave  = 0;
        int  semitone = 0;
        float detune = 0.0f;        // cents
        float fineTune = 0.0f;      // cents
        float phase  = 0.0f;        // 0..1 initial phase offset
    };

    const Oscillator& getOscillator (int index) const noexcept { return m_oscillators[index]; }

    // Engine type per oscillator slot. VA by default; FM/Additive/Granular
    // wired up in later steps without changing this struct.
    dsp::EngineType getOscillatorEngine (int index) const noexcept { return m_oscEngine[index]; }

    // ---- FM synthesis parameters per oscillator -----------------------------
    struct FmParams
    {
        float frequencyRatio = 1.0f;   // carrier:modulator ratio
        float fineRatio = 1.0f;        // fine ratio multiplier
        float fmAmount = 0.0f;         // FM modulation index (0..100)
        float pmAmount = 0.0f;         // PM modulation amount (0..10)
        float feedback = 0.0f;         // self-feedback (-1..1)
        float outputLevel = 0.7f;      // output gain (0..2)
    };

    const FmParams& getFmParams (int index) const noexcept { return m_fmParams[index]; }

    struct Filter
    {
        bool enabled = true;
        int  type = 0;              // 0=LowPass, 1=HighPass, 2=BandPass
        float cutoff = 2000.0f;     // Hz
        float resonance = 0.5f;
        float keyTrack = 0.0f;      // 0..1
    };

    const Filter& getFilter() const noexcept { return m_filter; }

    struct Envelope
    {
        float attack = 0.01f;   // seconds
        float decay = 0.1f;
        float sustain = 0.7f;   // 0..1
        float release = 0.2f;   // seconds
    };

    const Envelope& getEnvelope() const noexcept { return m_envelope; }

    // ---- Additive synthesis parameters per oscillator -----------------------
    struct AdditiveParams
    {
        int    harmonicCount = 8;
        float  spectralMorph = 0.0f;   // 0 = Spectrum A, 1 = Spectrum B
        float  spectralTilt = 0.0f;    // -1..1
        float  oddEvenBalance = 0.0f;  // -1..1
        float  fundamentalLevel = 1.0f;
        float  outputLevel = 0.7f;
    };

    const AdditiveParams& getAdditiveParams (int index) const noexcept { return m_addParams[index]; }

private:
    std::atomic<float> m_masterVolume { 0.7f };
    std::atomic<int>   m_polyphony    { 16 };

    Oscillator m_oscillators[params::NumOscillators];
    dsp::EngineType m_oscEngine[params::NumOscillators] = { dsp::EngineType::VA };
    FmParams m_fmParams[params::NumOscillators];

    AdditiveParams m_addParams[params::NumOscillators];
    Filter     m_filter;
    Envelope   m_envelope;
};

} // namespace omnivy
