#include "DSP/Engine/OscillatorEngine.h"

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
    if (m_oscillator)
        m_oscillator->setExternalAudioInput (sample);
}

void OscillatorEngine::setFrequencyModInput (float mod) noexcept
{
    if (m_oscillator)
        m_oscillator->setFrequencyModInput (mod);
}

void OscillatorEngine::setPhaseModInput (float mod) noexcept
{
    if (m_oscillator)
        m_oscillator->setPhaseModInput (mod);
}

void OscillatorEngine::setRingModInput (float mod) noexcept
{
    if (m_oscillator)
        m_oscillator->setRingModInput (mod);
}

void OscillatorEngine::setHardSyncTrigger (bool trigger) noexcept
{
    if (m_oscillator)
        m_oscillator->setHardSyncTrigger (trigger);
}

StereoFrame OscillatorEngine::processSample()
{
    if (!m_oscillator)
        return StereoFrame { 0.0f, 0.0f };
    m_lastOutput = m_oscillator->processSample();
    return m_lastOutput;
}

void OscillatorEngine::resetModInputs() noexcept
{
    if (m_oscillator)
    {
        m_oscillator->setExternalAudioInput (0.0f);
        m_oscillator->setFrequencyModInput (0.0f);
        m_oscillator->setPhaseModInput (0.0f);
        m_oscillator->setRingModInput (0.0f);
        m_oscillator->setHardSyncTrigger (false);
    }
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

} // namespace dsp
} // namespace omnivy
