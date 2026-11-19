#include "DSP/Engine/OscillatorEngine.h"

#include "DSP/Oscillators/AdditiveOscillator.h"
#include "DSP/Oscillators/FMOscillator.h"
#include "DSP/Oscillators/GranularOscillator.h"
#include "DSP/Oscillators/VAOscillator.h"
#include "DSP/Wavetable/WavetableOscillator.h"

#include <cmath>

namespace omnivy {
namespace dsp {

OscillatorEngine::OscillatorEngine() = default;

void OscillatorEngine::init (dsp::WavetableData* wavetableData)
{
    m_wavetableData = wavetableData;
    recreateOscillator();
}

void OscillatorEngine::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    if (m_oscillator)
        m_oscillator->setSampleRate (sampleRate);
}

void OscillatorEngine::reset()
{
    if (m_oscillator)
        m_oscillator->reset();
}

void OscillatorEngine::setEngineType (EngineType type)
{
    if (m_engineType == type)
        return;
    m_engineType = type;
    recreateOscillator();
}

void OscillatorEngine::recreateOscillator()
{
    switch (m_engineType)
    {
        case EngineType::Wavetable:
            m_oscillator = std::make_unique<WavetableOscillator>();
            break;
        case EngineType::Additive:
            m_oscillator = std::make_unique<AdditiveOscillator>();
            break;
        case EngineType::Granular:
            m_oscillator = std::make_unique<GranularOscillator>();
            break;
        case EngineType::VA:
        default:
            m_oscillator = std::make_unique<VAOscillator>();
            break;
    }

    if (m_wavetableData != nullptr)
    {
        if (auto* wt = dynamic_cast<WavetableOscillator*> (m_oscillator.get()))
            wt->setWavetableData (m_wavetableData);
    }

    m_oscillator->setSampleRate (m_sampleRate);
}

void OscillatorEngine::setFrequency (float frequencyHz)       { if (m_oscillator) m_oscillator->setFrequency (frequencyHz); }
float OscillatorEngine::getFrequency() const                  { return m_oscillator ? m_oscillator->getFrequency() : 0.0f; }
void OscillatorEngine::setWaveform (Waveform waveform)        { if (m_oscillator) m_oscillator->setWaveform (waveform); }
Waveform OscillatorEngine::getWaveform() const                { return m_oscillator ? m_oscillator->getWaveform() : Waveform::Sine; }
void OscillatorEngine::setLevel (float level)                 { if (m_oscillator) m_oscillator->setLevel (level); }
float OscillatorEngine::getLevel() const                      { return m_oscillator ? m_oscillator->getLevel() : 0.0f; }
void OscillatorEngine::setPan (float pan)                     { if (m_oscillator) m_oscillator->setPan (pan); }
float OscillatorEngine::getPan() const                        { return m_oscillator ? m_oscillator->getPan() : 0.0f; }
void OscillatorEngine::setActive (bool active)                { if (m_oscillator) m_oscillator->setActive (active); }
bool OscillatorEngine::isActive() const                       { return m_oscillator ? m_oscillator->isActive() : false; }
void OscillatorEngine::setOctave (int octave)                 { if (m_oscillator) m_oscillator->setOctave (octave); }
void OscillatorEngine::setSemitone (int semitone)             { if (m_oscillator) m_oscillator->setSemitone (semitone); }
void OscillatorEngine::setDetuneCents (float cents)           { if (m_oscillator) m_oscillator->setDetuneCents (cents); }
void OscillatorEngine::setFineTuneCents (float cents)         { if (m_oscillator) m_oscillator->setFineTuneCents (cents); }
void OscillatorEngine::setInitialPhase (float phase)          { if (m_oscillator) m_oscillator->setInitialPhase (phase); }
float OscillatorEngine::getPhase() const                      { return m_oscillator ? m_oscillator->getPhase() : 0.0f; }

void OscillatorEngine::setExternalAudioInput (float sample) noexcept
{
    m_modExternalAudio += sample;
}

void OscillatorEngine::setFrequencyModInput (float mod) noexcept
{
    m_modFrequency += mod;
}

void OscillatorEngine::setPhaseModInput (float mod) noexcept
{
    m_modPhase += mod;
}

void OscillatorEngine::setRingModInput (float mod) noexcept
{
    m_modRing += mod;
}

void OscillatorEngine::setHardSyncTrigger (bool trigger) noexcept
{
    m_modHardSync = m_modHardSync || trigger;
}

StereoFrame OscillatorEngine::processSample()
{
    if (!m_oscillator)
        return StereoFrame { 0.0f, 0.0f };

    // Pass accumulated modulation inputs to the wrapped oscillator before
    // rendering. FM/PM/Ring/Sync engines read these during their own
    // processSample() call.
    m_oscillator->setExternalAudioInput (m_modExternalAudio);
    m_oscillator->setFrequencyModInput (m_modFrequency);
    m_oscillator->setPhaseModInput (m_modPhase);
    m_oscillator->setRingModInput (m_modRing);
    m_oscillator->setHardSyncTrigger (m_modHardSync);

    m_lastOutput = m_oscillator->processSample();

    // Reset accumulators after rendering so the next block starts clean.
    m_modExternalAudio = 0.0f;
    m_modFrequency = 0.0f;
    m_modPhase = 0.0f;
    m_modRing = 0.0f;
    m_modHardSync = false;

    return m_lastOutput;
}

void OscillatorEngine::resetModInputs() noexcept
{
    m_modExternalAudio = 0.0f;
    m_modFrequency = 0.0f;
    m_modPhase = 0.0f;
    m_modRing = 0.0f;
    m_modHardSync = false;
}

void OscillatorEngine::setWavetableData (dsp::WavetableData* data)
{
    m_wavetableData = data;
    if (m_oscillator)
    {
        if (auto* wt = dynamic_cast<WavetableOscillator*> (m_oscillator.get()))
            wt->setWavetableData (data);
    }
}

// ---- FM parameter forwarding ----
void OscillatorEngine::setFrequencyRatio (float ratio)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setFrequencyRatio (ratio);
}

float OscillatorEngine::getFrequencyRatio() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getFrequencyRatio();
    return 0.0f;
}

void OscillatorEngine::setFineRatio (float fineRatio)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setFineRatio (fineRatio);
}

float OscillatorEngine::getFineRatio() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getFineRatio();
    return 0.0f;
}

void OscillatorEngine::setFmAmount (float amount)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setFmAmount (amount);
}

float OscillatorEngine::getFmAmount() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getFmAmount();
    return 0.0f;
}

void OscillatorEngine::setPmAmount (float amount)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setPmAmount (amount);
}

float OscillatorEngine::getPmAmount() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getPmAmount();
    return 0.0f;
}

void OscillatorEngine::setFeedback (float amount)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setFeedback (amount);
}

float OscillatorEngine::getFeedback() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getFeedback();
    return 0.0f;
}

void OscillatorEngine::setOutputLevel (float level)
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        fm->setOutputLevel (level);
}

float OscillatorEngine::getOutputLevel() const noexcept
{
    if (auto* fm = dynamic_cast<FMOscillator*> (m_oscillator.get()))
        return fm->getOutputLevel();
    return 0.0f;
}

// ---- Additive parameter forwarding ----
void OscillatorEngine::setHarmonicCount (int count)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setHarmonicCount (count);
}

int OscillatorEngine::getHarmonicCount() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getHarmonicCount();
    return 0;
}

void OscillatorEngine::setSpectralMorph (float morph)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setSpectralMorph (morph);
}

float OscillatorEngine::getSpectralMorph() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getSpectralMorph();
    return 0.0f;
}

void OscillatorEngine::setSpectralTilt (float tilt)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setSpectralTilt (tilt);
}

float OscillatorEngine::getSpectralTilt() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getSpectralTilt();
    return 0.0f;
}

void OscillatorEngine::setOddEvenBalance (float balance)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setOddEvenBalance (balance);
}

float OscillatorEngine::getOddEvenBalance() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getOddEvenBalance();
    return 0.0f;
}

void OscillatorEngine::setFundamentalLevel (float level)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setFundamentalLevel (level);
}

float OscillatorEngine::getFundamentalLevel() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getFundamentalLevel();
    return 0.0f;
}

void OscillatorEngine::setAddOutputLevel (float level)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setOutputLevel (level);
}

float OscillatorEngine::getAddOutputLevel() const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getOutputLevel();
    return 0.0f;
}

// ---- Additive partial access forwarding ----
void OscillatorEngine::setPartialAmplitude (int index, float amplitude)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setPartialAmplitude (index, amplitude);
}

float OscillatorEngine::getPartialAmplitude (int index) const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getPartialAmplitude (index);
    return 0.0f;
}

void OscillatorEngine::setPartialPhase (int index, float phase)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setPartialPhase (index, phase);
}

float OscillatorEngine::getPartialPhase (int index) const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getPartialPhase (index);
    return 0.0f;
}

void OscillatorEngine::setPartialFrequencyRatio (int index, float ratio)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setPartialFrequencyRatio (index, ratio);
}

float OscillatorEngine::getPartialFrequencyRatio (int index) const noexcept
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getPartialFrequencyRatio (index);
    return 0.0f;
}

void OscillatorEngine::setSpectrumAPartial (int index, const AdditivePartial& partial)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setSpectrumAPartial (index, partial);
}

void OscillatorEngine::setSpectrumBPartial (int index, const AdditivePartial& partial)
{
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        ad->setSpectrumBPartial (index, partial);
}

const AdditivePartial& OscillatorEngine::getSpectrumAPartial (int index) const noexcept
{
    static const AdditivePartial empty;
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getSpectrumAPartial (index);
    return empty;
}

const AdditivePartial& OscillatorEngine::getSpectrumBPartials (int index) const noexcept
{
    static const AdditivePartial empty;
    if (auto* ad = dynamic_cast<AdditiveOscillator*> (m_oscillator.get()))
        return ad->getSpectrumBPartials (index);
    return empty;
}

// ---- Granular parameter forwarding ---
void OscillatorEngine::setGrainSize (float seconds)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setGrainSize (seconds);
}
float OscillatorEngine::getGrainSize() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getGrainSize();
    return 0.0f;
}
void OscillatorEngine::setGrainDensity (float grainsPerSecond)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setGrainDensity (grainsPerSecond);
}
float OscillatorEngine::getGrainDensity() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getGrainDensity();
    return 0.0f;
}
void OscillatorEngine::setPosition (float position)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setPosition (position);
}
float OscillatorEngine::getPosition() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getPosition();
    return 0.0f;
}
void OscillatorEngine::setPositionSpray (float spray)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setPositionSpray (spray);
}
float OscillatorEngine::getPositionSpray() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getPositionSpray();
    return 0.0f;
}
void OscillatorEngine::setPitch (float semitones)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setPitch (semitones);
}
float OscillatorEngine::getPitch() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getPitch();
    return 0.0f;
}
void OscillatorEngine::setPitchRandomization (float semitones)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setPitchRandomization (semitones);
}
float OscillatorEngine::getPitchRandomization() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getPitchRandomization();
    return 0.0f;
}
void OscillatorEngine::setGrainEnvelope (GranularOscillator::GrainEnvelope envelope)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setGrainEnvelope (envelope);
}
GranularOscillator::GrainEnvelope OscillatorEngine::getGrainEnvelope() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getGrainEnvelope();
    return GranularOscillator::GrainEnvelope::None;
}
void OscillatorEngine::setStereoSpread (float spread)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setStereoSpread (spread);
}
float OscillatorEngine::getStereoSpread() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getStereoSpread();
    return 0.0f;
}
void OscillatorEngine::setPlaybackDirection (GranularOscillator::PlaybackDirection direction)
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setPlaybackDirection (direction);
}
GranularOscillator::PlaybackDirection OscillatorEngine::getPlaybackDirection() const noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        return gr->getPlaybackDirection();
    return GranularOscillator::PlaybackDirection::Forward;
}

void OscillatorEngine::setGranularSample (const SampleData* sample) noexcept
{
    if (auto* gr = dynamic_cast<GranularOscillator*> (m_oscillator.get()))
        gr->setSample (sample);
}

} // namespace dsp
} // namespace omnivy
