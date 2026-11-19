#include "DSP/Engine/SynthVoice.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {

SynthVoice::SynthVoice (dsp::WavetableData* wavetableData)
    : m_wavetableData (wavetableData)
{
    for (int i = 0; i < SynthParameters::NumOscillators; ++i)
    {
        m_oscillators[i] = std::make_unique<dsp::OscillatorEngine>();
        m_oscillators[i]->init (m_wavetableData);
        m_oscillators[i]->setEngineType (dsp::EngineType::VA);
    }
}

void SynthVoice::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    for (auto& osc : m_oscillators)
        osc->setSampleRate (sampleRate);
    m_ampEnv.setSampleRate (sampleRate);
    m_filterLeft.setSampleRate (sampleRate);
    m_filterRight.setSampleRate (sampleRate);
}

void SynthVoice::reset()
{
    m_active = false;
    m_triggered = false;
    m_note = -1;
    m_velocity = 0.0f;
    m_pitchBendSemitones = 0.0f;
    m_ampEnv.reset();
    m_filterLeft.reset();
    m_filterRight.reset();
    for (auto& osc : m_oscillators)
        osc->reset();
}

float SynthVoice::noteToFrequency (int midiNote) const noexcept
{
    return static_cast<float> (dsp::MidiNoteToFrequency (midiNote + m_pitchBendSemitones));
}

void SynthVoice::noteOn (int midiNote, float velocity, double /*sampleRate*/)
{
    m_note = midiNote;
    m_velocity = std::clamp (velocity, 0.0f, 1.0f);
    m_pitchBendSemitones = 0.0f;
    m_active = true;
    m_triggered = true;

    m_ampEnv.noteOn();
    updateOscillatorFrequencies();
}

void SynthVoice::noteOff()
{
    if (!m_active && !m_ampEnv.isActive())
        return;
    m_ampEnv.noteOff();
}

void SynthVoice::setPitchBendSemitones (float semitones)
{
    m_pitchBendSemitones = semitones;
    if (m_active)
        updateOscillatorFrequencies();
}

void SynthVoice::updateOscillatorFrequencies()
{
    if (!m_active)
        return;

    const float baseFreq = noteToFrequency (m_note);
    for (int i = 0; i < SynthParameters::NumOscillators; ++i)
        m_oscillators[i]->setFrequency (baseFreq);
}

void SynthVoice::updateParameters (const SynthParameters& params)
{
    // Envelope (shared across voices, applied per voice).
    m_ampEnv.setAttack (params.getEnvelope().attack);
    m_ampEnv.setDecay (params.getEnvelope().decay);
    m_ampEnv.setSustain (params.getEnvelope().sustain);
    m_ampEnv.setRelease (params.getEnvelope().release);

    // Filter shared settings (cutoff key-tracked per voice in render).
    m_filterEnabled = params.getFilter().enabled;
    m_filterCutoffBase = params.getFilter().cutoff;
    m_filterKeyTrack = params.getFilter().keyTrack;
    m_filterLeft.setType (static_cast<dsp::FilterType> (params.getFilter().type));
    m_filterRight.setType (static_cast<dsp::FilterType> (params.getFilter().type));
    m_filterLeft.setResonance (params.getFilter().resonance);
    m_filterRight.setResonance (params.getFilter().resonance);

    // Per-oscillator settings.
    for (int i = 0; i < SynthParameters::NumOscillators; ++i)
    {
        const auto& osc = params.getOscillator (i);
        auto* o = m_oscillators[i].get();

        o->setActive (osc.active);
        o->setWaveform (static_cast<dsp::Waveform> (osc.waveform));
        o->setLevel (osc.level);
        o->setPan (osc.pan);
        o->setOctave (osc.octave);
        o->setSemitone (osc.semitone);
        o->setDetuneCents (osc.detune);
        o->setFineTuneCents (osc.fineTune);
        // Engine type selected per slot. OscillatorEngine handles attaching
        // shared wavetable data internally when the engine is Wavetable.
        o->setEngineType (static_cast<dsp::EngineType> (params.getOscillatorEngine (i)));
        if (m_triggered)
            o->setInitialPhase (osc.phase);

        // Apply FM-specific parameters when the engine is FM.
        const auto& fm = params.getFmParams (i);
        o->setFrequencyRatio (fm.frequencyRatio);
        o->setFineRatio (fm.fineRatio);
        o->setFmAmount (fm.fmAmount);
        o->setPmAmount (fm.pmAmount);
        o->setFeedback (fm.feedback);
        o->setOutputLevel (fm.outputLevel);

        // Apply additive-specific parameters when the engine is Additive.
        const auto& add = params.getAdditiveParams (i);
        o->setHarmonicCount (add.harmonicCount);
        o->setSpectralMorph (add.spectralMorph);
        o->setSpectralTilt (add.spectralTilt);
        o->setOddEvenBalance (add.oddEvenBalance);
        o->setFundamentalLevel (add.fundamentalLevel);
        o->setAddOutputLevel (add.outputLevel);

        // Apply granular-specific parameters when the engine is Granular.
        const auto& gran = params.getGranularParams (i);
        o->setGrainSize (gran.grainSize);
        o->setGrainDensity (gran.grainDensity);
        o->setPosition (gran.position);
        o->setPositionSpray (gran.positionSpray);
        o->setPitch (gran.pitch);
        o->setPitchRandomization (gran.pitchRandomization);
        o->setGrainEnvelope (static_cast<dsp::GranularOscillator::GrainEnvelope> (gran.grainEnvelope));
        o->setStereoSpread (gran.stereoSpread);
        o->setPlaybackDirection (static_cast<dsp::GranularOscillator::PlaybackDirection> (gran.playbackDirection));
    }

    m_triggered = false;
}

dsp::StereoFrame SynthVoice::render()
{
    if (!m_active)
        return dsp::StereoFrame { 0.0f, 0.0f };

    if (m_triggered)
    {
        // Re-trigger path: ensure phase reset already done in noteOn/updateParameters.
        m_triggered = false;
    }

    // Sum six independent oscillators into stereo (each applies own level/pan).
    dsp::StereoFrame mix { 0.0f, 0.0f };
    bool anyActive = false;
    for (int i = 0; i < SynthParameters::NumOscillators; ++i)
    {
        if (m_oscillators[i]->isActive())
        {
            const auto f = m_oscillators[i]->processSample();
            mix.left += f.left;
            mix.right += f.right;
            anyActive = true;
        }
    }

    if (!anyActive)
    {
        m_active = false;
        return dsp::StereoFrame { 0.0f, 0.0f };
    }

    // Apply key-tracked cutoff per voice (note 69 = 440 Hz reference).
    const float keyTrackRatio = static_cast<float> (std::pow (2.0,
        (static_cast<double> (m_note) - 69.0) / 12.0 * m_filterKeyTrack));
    const float cutoff = m_filterCutoffBase * keyTrackRatio;
    m_filterLeft.setCutoff (cutoff);
    m_filterRight.setCutoff (cutoff);

    // Filter each channel independently (preserves stereo image), or bypass.
    float l = mix.left, r = mix.right;
    if (m_filterEnabled)
    {
        l = m_filterLeft.processSample (l);
        r = m_filterRight.processSample (r);
    }

    // Amplitude envelope.
    const float env = m_ampEnv.processSample();

    dsp::StereoFrame out { l * env * m_velocity, r * env * m_velocity };

    // Voice is done when envelope finished and no oscillators active.
    if (m_ampEnv.getStage() == dsp::ADSREnvelope::Stage::Finished)
        m_active = false;

    return out;
}

} // namespace omnivy
