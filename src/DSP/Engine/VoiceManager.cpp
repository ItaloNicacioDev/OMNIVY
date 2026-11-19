#include "DSP/Engine/VoiceManager.h"

#include <limits>
#include <memory>

namespace omnivy {

VoiceManager::VoiceManager (dsp::WavetableData* wavetableData)
    : m_wavetableData (wavetableData)
{
    // Pré-aloca o pool completo de voices no construtor (thread de mensagem,
    // criação do plugin). A thread de áudio nunca aloca nem desaloca voices.
    m_voices.reserve (static_cast<size_t> (kMaxVoices));
    for (int i = 0; i < kMaxVoices; ++i)
        m_voices.emplace_back (std::make_unique<SynthVoice> (m_wavetableData));
}

void VoiceManager::setSampleRate (double sampleRate)
{
    m_sampleRate = sampleRate;
    for (auto& voice : m_voices)
        voice->setSampleRate (sampleRate);
}

void VoiceManager::setPolyphony (int polyphony)
{
    // Apenas ajusta o contador ativo. Nenhuma alocação, nenhum resize, nenhum
    // make_unique — o pool já está pré-alocado no construtor.
    m_polyphony = std::clamp (polyphony, 1, kMaxVoices);
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
        if (voice && voice->isPlayingNote (midiNote))
            voice->noteOff();
    }
}

void VoiceManager::noteOffAll()
{
    for (auto& voice : m_voices)
        if (voice) voice->noteOff();
}

void VoiceManager::reset()
{
    noteOffAll();
    for (auto& voice : m_voices)
        if (voice) voice->reset();
    m_noteOnCounter = 0;
}

void VoiceManager::setPitchBendSemitones (float semitones)
{
    m_pitchBendSemitones = semitones;
    for (auto& voice : m_voices)
        if (voice) voice->setPitchBendSemitones (semitones);
}

void VoiceManager::updateParameters (const SynthParameters& params)
{
    setPolyphony (params.getPolyphony());
    for (auto& voice : m_voices)
        if (voice) voice->updateParameters (params);
}

dsp::StereoFrame VoiceManager::renderNextSample()
{
    dsp::StereoFrame sum { 0.0f, 0.0f };

    for (auto& voice : m_voices)
    {
        if (voice && voice->isActive())
        {
            const auto frame = voice->render();
            sum.left += frame.left;
            sum.right += frame.right;
        }
    }

    return sum;
}

int VoiceManager::getNumActiveVoices() const noexcept
{
    int count = 0;
    for (const auto& voice : m_voices)
        if (voice && voice->isActive()) ++count;
    return count;
}

bool VoiceManager::isVoicePlayingNote (int midiNote) const noexcept
{
    for (const auto& voice : m_voices)
        if (voice && voice->isPlayingNote (midiNote)) return true;
    return false;
}

SynthVoice* VoiceManager::findVoiceForNote (int midiNote) noexcept
{
    for (auto& voice : m_voices)
        if (voice && voice->isPlayingNote (midiNote) && voice->isActive())
            return voice.get();
    return nullptr;
}

SynthVoice* VoiceManager::findFreeVoice() noexcept
{
    for (auto& voice : m_voices)
        if (voice && voice->isIdle())
            return voice.get();
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
        if (!voice || !voice->isActive())
            continue;

        if (bestActive == nullptr || voice->getNoteOnOrder() < bestActive->getNoteOnOrder())
            bestActive = voice.get();

        if (voice->isInRelease())
        {
            if (bestRelease == nullptr || voice->getNoteOnOrder() < bestRelease->getNoteOnOrder())
                bestRelease = voice.get();
        }
    }

    return bestRelease != nullptr ? bestRelease : bestActive;
}

} // namespace omnivy
