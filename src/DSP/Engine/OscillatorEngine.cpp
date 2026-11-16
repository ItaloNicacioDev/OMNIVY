#include "DSP/Engine/OscillatorEngine.h"

#include "DSP/Oscillators/VAOscillator.h"
#include "DSP/Oscillators/FMOscillator.h"
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

} // namespace dsp
} // namespace omnivy
