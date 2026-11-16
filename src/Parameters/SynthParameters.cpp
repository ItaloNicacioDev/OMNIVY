#include "SynthParameters.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <algorithm>

namespace omnivy {
using namespace params;
using namespace dsp;

SynthParameters::SynthParameters()
{
    for (int i = 0; i < params::NumOscillators; ++i)
    {
        // Default: all six oscillators active, saw at 0.7 level, detuned slightly
        // per oscillator for a richer unison-like default mix.
        m_oscillators[i].active = true;
        m_oscillators[i].waveform = static_cast<int> (Waveform::Saw);
        m_oscillators[i].level = 0.6f;
        m_oscillators[i].pan = static_cast<float> (i - (params::NumOscillators - 1) / 2)
                             / static_cast<float> (params::NumOscillators);
        m_oscillators[i].octave = 0;
        m_oscillators[i].semitone = 0;
        m_oscillators[i].detune = static_cast<float> (i - (params::NumOscillators - 1) / 2) * 7.0f;
        m_oscillators[i].fineTune = 0.0f;
        m_oscillators[i].phase = 0.0f;
        m_oscEngine[i] = EngineType::VA;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SynthParameters::createParameterLayout()
{
    using namespace juce;
    using namespace params;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // ---- Global ---------------------------------------------------------------
    params.push_back (std::make_unique<AudioParameterFloat> (MasterVolume, "Master Volume",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back (std::make_unique<AudioParameterInt> (Polyphony, "Polyphony",
        1, 64, 16));

    params.push_back (std::make_unique<AudioParameterInt> (UnisonVoices, "Unison Voices",
        1, 8, 1));

    params.push_back (std::make_unique<AudioParameterFloat> (UnisonDetune, "Unison Detune",
        NormalisableRange<float> (0.0f, 50.0f, 0.1f), 7.0f));

    // ---- Oscillators ----------------------------------------------------------
    for (int i = 0; i < NumOscillators; ++i)
    {
        const juce::String num = juce::String (i + 1);
        const juce::String base = oscPrefix (i);

        params.push_back (std::make_unique<AudioParameterBool> (oscActive (i),
            "OSC " + num + " Active", true));

        params.push_back (std::make_unique<AudioParameterInt> (oscWaveform (i),
            "OSC " + num + " Waveform", 0, static_cast<int> (Waveform::Count) - 1, 0));

        params.push_back (std::make_unique<AudioParameterFloat> (oscLevel (i),
            "OSC " + num + " Level",
            NormalisableRange<float> (MinLevel, MaxLevel, 0.01f), 0.6f));

        params.push_back (std::make_unique<AudioParameterFloat> (oscPan (i),
            "OSC " + num + " Pan",
            NormalisableRange<float> (MinPan, MaxPan, 0.01f), 0.0f));

        params.push_back (std::make_unique<AudioParameterInt> (oscOctave (i),
            "OSC " + num + " Octave", MinOctave, MaxOctave, 0));

        params.push_back (std::make_unique<AudioParameterInt> (oscSemitone (i),
            "OSC " + num + " Semitone", MinSemi, MaxSemi, 0));

        params.push_back (std::make_unique<AudioParameterFloat> (oscDetune (i),
            "OSC " + num + " Detune",
            NormalisableRange<float> (MinDetune, MaxDetune, 0.1f), 0.0f));

        params.push_back (std::make_unique<AudioParameterFloat> (oscFineTune (i),
            "OSC " + num + " Fine Tune",
            NormalisableRange<float> (MinDetune, MaxDetune, 0.1f), 0.0f));

        params.push_back (std::make_unique<AudioParameterFloat> (oscPhase (i),
            "OSC " + num + " Phase",
            NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<AudioParameterInt> (oscEngine (i),
            "OSC " + num + " Engine",
            0, static_cast<int> (EngineType::Count) - 1, 0)); // 0 = VA
    }

    // ---- Filter ---------------------------------------------------------------
    params.push_back (std::make_unique<AudioParameterBool> (FilterEnabled, "Filter Enabled", true));

    params.push_back (std::make_unique<AudioParameterInt> (FilterType, "Filter Type",
        0, 2, 0)); // 0 LP, 1 HP, 2 BP

    params.push_back (std::make_unique<AudioParameterFloat> (FilterCutoff, "Filter Cutoff",
        NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (FilterResonance, "Filter Resonance",
        NormalisableRange<float> (0.0f, 2.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat> (FilterKeyTrack, "Filter Key Tracking",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    // ---- Amplitude envelope ---------------------------------------------------
    params.push_back (std::make_unique<AudioParameterFloat> (EnvAttack, "Env Attack",
        NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

    params.push_back (std::make_unique<AudioParameterFloat> (EnvDecay, "Env Decay",
        NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.1f));

    params.push_back (std::make_unique<AudioParameterFloat> (EnvSustain, "Env Sustain",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back (std::make_unique<AudioParameterFloat> (EnvRelease, "Env Release",
        NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

    return { params.begin(), params.end() };
}

void SynthParameters::updateFromValueTreeState (juce::AudioProcessorValueTreeState& vts)
{
    m_masterVolume.store (*vts.getRawParameterValue (MasterVolume));
    m_polyphony.store (static_cast<int> (*vts.getRawParameterValue (Polyphony)));

    for (int i = 0; i < params::NumOscillators; ++i)
    {
        auto& osc = m_oscillators[i];
        osc.active  = *vts.getRawParameterValue (oscActive (i)) > 0.5f;
        osc.waveform = static_cast<int> (*vts.getRawParameterValue (oscWaveform (i)));
        osc.level   = *vts.getRawParameterValue (oscLevel (i));
        osc.pan     = *vts.getRawParameterValue (oscPan (i));
        osc.octave  = static_cast<int> (*vts.getRawParameterValue (oscOctave (i)));
        osc.semitone = static_cast<int> (*vts.getRawParameterValue (oscSemitone (i)));
        osc.detune   = *vts.getRawParameterValue (oscDetune (i));
        osc.fineTune = *vts.getRawParameterValue (oscFineTune (i));
        osc.phase    = *vts.getRawParameterValue (oscPhase (i));
        m_oscEngine[i] = static_cast<EngineType> (*vts.getRawParameterValue (oscEngine (i)));
    }

    m_filter.enabled   = *vts.getRawParameterValue (FilterEnabled) > 0.5f;
    m_filter.type      = static_cast<int> (*vts.getRawParameterValue (FilterType));
    m_filter.cutoff    = *vts.getRawParameterValue (FilterCutoff);
    m_filter.resonance = *vts.getRawParameterValue (FilterResonance);
    m_filter.keyTrack  = *vts.getRawParameterValue (FilterKeyTrack);

    m_envelope.attack  = *vts.getRawParameterValue (EnvAttack);
    m_envelope.decay   = *vts.getRawParameterValue (EnvDecay);
    m_envelope.sustain = *vts.getRawParameterValue (EnvSustain);
    m_envelope.release = *vts.getRawParameterValue (EnvRelease);
}

} // namespace omnivy
