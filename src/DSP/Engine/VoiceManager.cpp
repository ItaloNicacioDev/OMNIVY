#include "DSP/Engine/VoiceManager.h"

#include <limits>

namespace omnivy {

VoiceManager::VoiceManager (dsp::WavetableData* wavetableData)
    : m_wavetableData (wavetableData)
{
}

void VoiceManager::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    for (auto& voice : m_voices)
        voice.setSampleRate (sampleRate);
}

void VoiceManager::setPolyphony (int polyphony)
{
    polyphony = std::clamp (polyphony, 1, 64);

    const int current = static_cast<int> (m_voices.size());
    if (polyphony == current)
    {
        m_polyphony = polyphony;
        return;
    }

    if (polyphony > current)
    {
        m_voices.reserve (static_cast<size_t> (polyphony));
        for (int i = current; i < polyphony; ++i)
        {
            m_voices.emplace_back (m_wavetableData);
            m_voices.back().setSampleRate (m_sampleRate);
        }
    }
    else
    {
        // Release voices being removed, then destroy the excess (no copy needed).
        for (int i = polyphony; i < current; ++i)
            m_voices[i].noteOff();
        m_voices.resize (static_cast<size_t> (polyphony));
    }

    m_polyphony = polyphony;
}

void VoiceManager::noteOn (int midiNote, float velocity)
{
    // Re-trigger an existing voice playing the same note (mono per-note).
    if (SynthVoice* existing = findVoiceForNote (midiNote))
    {
        existing->noteOn (midiNote, velocity, 0.0);
        return;
    }

    // Prefer a free (idle) voice.
    if (SynthVoice* freeVoice = findFreeVoice())
    {
        freeVoice->noteOn (midiNote, velocity, 0.0);
        return;
    }

    // No free voice: steal intelligently.
    if (SynthVoice* victim = selectStealVictim())
    {
        victim->noteOn (midiNote, velocity, 0.0);
        return;
    }

    // All voices busy and none stealable: drop the note (better than a stuck note).
}

void VoiceManager::noteOff (int midiNote)
{
    for (auto& voice : m_voices)
    {
        if (voice.isPlayingNote (midiNote))
            voice.noteOff();
    }
}

void VoiceManager::noteOffAll()
{
    for (auto& voice : m_voices)
        voice.noteOff();
}

void VoiceManager::reset()
{
    noteOffAll();
    for (auto& voice : m_voices)
        voice.reset();
    m_noteOnCounter = 0;
}

void VoiceManager::setPitchBendSemitones (float semitones)
{
    m_pitchBendSemitones = semitones;
    for (auto& voice : m_voices)
        voice.setPitchBendSemitones (semitones);
}

void VoiceManager::updateParameters (const SynthParameters& params)
{
    setPolyphony (params.getPolyphony());
    for (auto& voice : m_voices)
        voice.updateParameters (params);
}

dsp::StereoFrame VoiceManager::renderNextSample()
{
    dsp::StereoFrame sum { 0.0f, 0.0f };
    int active = 0;

    for (auto& voice : m_voices)
    {
        if (voice.isActive() || voice.getNote() >= 0)
        {
            const auto frame = voice.render();
            sum.left += frame.left;
            sum.right += frame.right;
            if (voice.isActive()) ++active;
        }
    }

    (void) active;
    return sum;
}

int VoiceManager::getNumActiveVoices() const noexcept
{
    int count = 0;
    for (const auto& voice : m_voices)
        if (voice.isActive()) ++count;
    return count;
}

bool VoiceManager::isVoicePlayingNote (int midiNote) const noexcept
{
    for (const auto& voice : m_voices)
        if (voice.isPlayingNote (midiNote)) return true;
    return false;
}

SynthVoice* VoiceManager::findVoiceForNote (int midiNote) noexcept
{
    for (auto& voice : m_voices)
        if (voice.isPlayingNote (midiNote) && voice.isActive())
            return &voice;
    return nullptr;
}

SynthVoice* VoiceManager::findFreeVoice() noexcept
{
    SynthVoice* oldestIdle = nullptr;
    int oldestIdleTime = std::numeric_limits<int>::max();

    for (auto& voice : m_voices)
    {
        if (voice.isIdle())
        {
            return &voice; // any idle voice is usable
        }
    }
    return nullptr;
}

SynthVoice* VoiceManager::selectStealVictim() noexcept
{
    // Deterministic, intelligent stealing:
    //  1. Prefer a voice already in release (envelope fading out) — stealing it
    //     is inaudible. Pick the oldest such voice.
    //  2. Otherwise steal the oldest active voice (lowest note-on order).

    SynthVoice* bestRelease = nullptr;
    SynthVoice* bestActive = nullptr;

    for (auto& voice : m_voices)
    {
        if (!voice.isActive())
            continue;

        if (bestActive == nullptr || voice.getNoteOnOrder() < bestActive->getNoteOnOrder())
            bestActive = &voice;

        if (voice.isInRelease())
        {
            if (bestRelease == nullptr || voice.getNoteOnOrder() < bestRelease->getNoteOnOrder())
                bestRelease = &voice;
        }
    }

    return bestRelease != nullptr ? bestRelease : bestActive;
}

} // namespace omnivy
