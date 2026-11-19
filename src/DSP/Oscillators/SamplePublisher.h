#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "DSP/Oscillators/SampleData.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// SamplePublisher — publicação RCU (Read-Copy-Update) de SampleData.
//
// Modelo de threads:
//   - Thread de áudio: chama acquire() uma vez por processBlock. Wait-free,
//     sem alocação, sem mutex, sem destruição. Obtém um ponteiro const válido
//     para todo o bloco.
//   - Thread de mensagem: chama publish() para trocar o sample. Pode alocar;
//     nunca bloqueia a thread de áudio. É a única responsável por destruir
//     buffers antigos.
//   - shutdown(): chamado fora da thread de áudio (releaseResources /
//     destruição do plugin). Aguarda leitores e destrói buffers pendentes.
//
// Segurança (prova RCU):
//   1. Audio thread carrega m_current uma vez por bloco (acquire). Todas as
//      vozes usam esse mesmo ponteiro durante o bloco.
//   2. Message thread troca m_current via exchange (acq_rel) e aposenta o
//      buffer antigo com um tag de epoch (m_globalEpoch).
//   3. Audio thread anuncia m_audioSeenEpoch = m_globalEpoch no início de
//      cada bloco (release). Isso marca a fronteira de bloco.
//   4. Reclamation só destrói um buffer aposentado quando
//      m_audioSeenEpoch >= retireEpoch, ou seja, o áudio já passou por uma
//      fronteira de bloco após a aposentadoria.
//   5. Use-after-free impossível: se o áudio está mid-bloco usando o buffer
//      aposentado, audioSeenEpoch < retireEpoch e o buffer não é destruído.
// -----------------------------------------------------------------------------
class SamplePublisher
{
public:
    SamplePublisher() = default;
    ~SamplePublisher() { shutdown(); }

    SamplePublisher (const SamplePublisher&) = delete;
    SamplePublisher& operator= (const SamplePublisher&) = delete;

    // --- THREAD DE ÁUDIO ------------------------------------------------------
    // Chamada uma vez por processBlock, antes de qualquer voz renderizar.
    // Wait-free, sem alocação, sem bloqueio, sem destruição.
    const SampleData* acquire() noexcept
    {
        // Anuncia o epoch que o áudio está vendo: ponto de quietude (quiescent
        // state) = início do bloco. Isso permite que a message thread saiba
        // quando é seguro destruir buffers aposentados.
        m_audioSeenEpoch.store(
            m_globalEpoch.load (std::memory_order_acquire),
            std::memory_order_release);

        return m_current.load (std::memory_order_acquire);
    }

    // Consulta não-áudio (editor, presets, host parado). Não atualiza o epoch
    // de áudio; usar apenas fora do callback de áudio.
    const SampleData* current() const noexcept
    {
        return m_current.load (std::memory_order_acquire);
    }

    // --- THREAD DE MENSAGEM ---------------------------------------------------
    // Publica um novo sample. A propriedade do SampleData é transferida para
    // o publisher; o chamador não deve mais usá-lo após esta chamada.
    // Pode alocar; nunca bloqueia a thread de áudio.
    void publish (std::unique_ptr<SampleData> next) noexcept
    {
        SampleData* raw = next.get();
        SampleData* old = m_current.exchange (raw, std::memory_order_acq_rel);
        next.release(); // propriedade agora pertence a m_current

        if (old == nullptr)
            return;

        // Aposenta o buffer antigo com um tag de epoch.
        const uint64_t retireEpoch =
            m_globalEpoch.fetch_add (1, std::memory_order_acq_rel) + 1;
        m_retireList.emplace_back (old, retireEpoch);

        // Tenta reclamar buffers já seguros para destruir.
        reclaim();
    }

    // Shutdown: chamado fora da thread de áudio. Aguarda que não haja bloco
    // de áudio em andamento usando buffers aposentados, então destrói tudo.
    void shutdown() noexcept
    {
        // Força o áudio a anunciar o epoch mais recente (se ainda estiver
        // rodando). Depois disso, todos os buffers aposentados são seguros.
        m_audioSeenEpoch.store(
            m_globalEpoch.load (std::memory_order_acquire),
            std::memory_order_release);

        // Destroi todos os buffers na lista de aposentadoria.
        for (auto& entry : m_retireList)
            delete entry.first;
        m_retireList.clear();

        // Destroi o buffer atual, se houver.
        SampleData* current = m_current.exchange (nullptr, std::memory_order_acq_rel);
        delete current;
    }

private:
    // Reclamação: destrói buffers aposentados cujo retireEpoch <= audioSeenEpoch.
    // Chamada apenas pela thread de mensagem.
    void reclaim() noexcept
    {
        const uint64_t audioSeen =
            m_audioSeenEpoch.load (std::memory_order_acquire);

        auto it = m_retireList.begin();
        while (it != m_retireList.end())
        {
            if (audioSeen >= it->second)
            {
                delete it->first;
                it = m_retireList.erase (it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::atomic<const SampleData*> m_current { nullptr };
    std::atomic<uint64_t>          m_globalEpoch { 0 };
    // Escrito pela thread de áudio (release), lido pela thread de mensagem
    // (acquire). Marca a última fronteira de bloco vista pelo áudio.
    std::atomic<uint64_t>          m_audioSeenEpoch { 0 };

    // Lista de aposentadoria: (ponteiro, retireEpoch). Tocada apenas pela
    // thread de mensagem.
    std::vector<std::pair<const SampleData*, uint64_t>> m_retireList;
};

} // namespace dsp
} // namespace omnivy
