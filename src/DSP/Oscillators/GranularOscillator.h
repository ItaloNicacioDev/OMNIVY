#pragma once

#include "DSP/Oscillators/OscillatorBase.h"
#include "DSP/Oscillators/SampleData.h"
#include "DSP/Utilities/Smoothing.h"

#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// GranularOscillator — mecanismo de síntese granular genuíno.
//
// Cada instância mantém seu próprio pool de grãos, estado de reprodução e
// parâmetros, garantindo isolamento completo entre os seis slots e entre
// vozes. Todo o processamento é real-time safe: zero alocações, zero mutexes,
// zero operações de disco dentro de processSample().
// -----------------------------------------------------------------------------
class GranularOscillator : public OscillatorBase
{
public:
    static constexpr int kMaxGrains = 4096; // suporte a até ~2048 grãos
                                               // simultâneos (1.000 grãos/s
                                               // × 2,0 s) com margem.

    enum class GrainEnvelope : int
    {
        None = 0,   // sem envelope (ganho constante)
        Linear,     // rampa linear 1 -> 0
        Triangle,   // rampa 0 -> 1 -> 0
        Hanning,    // janela de Hann 0 -> 1 -> 0
        Count
    };

    enum class PlaybackDirection : int
    {
        Forward = 0,
        Reverse = 1
    };

    GranularOscillator();
    ~GranularOscillator() override = default;

    // ---- OscillatorBase ------------------------------------------------------
    void setSampleRate (double sampleRate) override;
    void reset() override;

    void setFrequency (float frequencyHz) override;
    float getFrequency() const override;

    void setWaveform (Waveform) override { /* granular ignora waveform */ }
    Waveform getWaveform() const override { return Waveform::Sine; }

    void setLevel (float level) override;
    float getLevel() const override;
    void setPan (float pan) override;
    float getPan() const override;
    void setActive (bool active) override;
    bool isActive() const override;

    void setOctave (int octave) override;
    void setSemitone (int semitone) override;
    void setDetuneCents (float cents) override;
    void setFineTuneCents (float cents) override;

    void setInitialPhase (float phase) override;
    float getPhase() const override;

    // ---- Parâmetros granulares ----------------------------------------------
    void setGrainSize (float seconds);           // 0.01 .. 2.0 s
    float getGrainSize() const noexcept { return m_grainSize; }

    void setGrainDensity (float grainsPerSecond); // 0 .. 1000
    float getGrainDensity() const noexcept { return m_grainDensity; }

    void setPosition (float position);           // 0 .. 1 (normalizado)
    float getPosition() const noexcept { return m_position; }

    void setPositionSpray (float spray);         // 0 .. 1
    float getPositionSpray() const noexcept { return m_positionSpray; }

    void setPitch (float semitones);             // -48 .. +48
    float getPitch() const noexcept { return m_pitchSemitones; }

    void setPitchRandomization (float semitones); // 0 .. 24
    float getPitchRandomization() const noexcept { return m_pitchRandomization; }

    void setGrainEnvelope (GrainEnvelope envelope);
    GrainEnvelope getGrainEnvelope() const noexcept { return m_grainEnvelope; }

    void setStereoSpread (float spread);         // 0 .. 1
    float getStereoSpread() const noexcept { return m_stereoSpread; }

    void setPlaybackDirection (PlaybackDirection direction);
    PlaybackDirection getPlaybackDirection() const noexcept { return m_playbackDirection; }

    // Define o sample a ser lido. Chamado pela thread de áudio com o ponteiro
    // obtido via SamplePublisher::acquire() no início do processBlock.
    void setSample (const SampleData* sample) noexcept { m_sample = sample; }

    // Retorna o número de grãos ativos no momento (útil para testes).
    int getActiveGrainCount() const noexcept { return m_activeCount; }

    StereoFrame processSample() override;

private:
    struct Grain
    {
        bool             active = false;
        int              activeListIndex = -1;
        double           position = 0.0;      // índice de amostra no buffer
        double           increment = 0.0;     // buffer-samples por output-sample
        float            gain = 0.0f;         // ganho já com envelope aplicado
        float            pan = 0.0f;          // -1 .. 1, definido no nascimento
        float            envPhase = 0.0f;     // 0 .. 1 sobre a vida do grão
        float            envInc = 0.0f;       // incremento do envelope por amostra
        int              samplesLeft = 0;
    };

    // Spawn de um novo grão. Retorna false se o pool estiver cheio.
    bool spawnGrain();

    // Atualiza o ganho por grão com base no número esperado de grãos ativos.
    void updateGrainGainScale() noexcept;

    // Aplica o envelope a um ganho base, dado a fase do envelope (0..1).
    float applyEnvelope (float envPhase) const noexcept;

    // Lê uma amostra do buffer com interpolação linear.
    float readBuffer (int channel, double position) const noexcept;

    float effectivePitchRatio() const noexcept;

    float m_frequencyHz = 440.0f;

    // ---- Parâmetros granulares ----------------------------------------------
    std::atomic<float> m_grainSize { 0.1f };            // segundos
    std::atomic<float> m_grainDensity { 20.0f };        // grãos por segundo
    float m_position = 0.5f;             // 0 .. 1
    float m_positionSpray = 0.0f;        // 0 .. 1
    float m_pitchSemitones = 0.0f;       // -48 .. +48
    float m_pitchRandomization = 0.0f;   // 0 .. 24
    GrainEnvelope m_grainEnvelope = GrainEnvelope::Triangle;
    float m_stereoSpread = 0.5f;         // 0 .. 1
    PlaybackDirection m_playbackDirection = PlaybackDirection::Forward;

    // Ganho escalado por densidade esperada — acessado por thread de áudio
    // (spawnGrain) e thread de mensagem (updateGrainGainScale). Atômico para
    // evitar data race.
    std::atomic<float> m_grainGainScale { 1.0f };

    // ---- Cache de pitch ratio -----------------------------------------------
    // effectivePitchRatio() chama 3× std::pow(). Os inputs mudam raramente
    // (apenas em setters de pitch), então cacheamos o resultado e invalidamos
    // quando qualquer parâmetro de pitch é alterado.
    float m_pitchRatioCache = 1.0f;
    bool m_pitchRatioDirty = true;

    void updatePitchRatioCache() noexcept;

    // ---- Pool de grãos (pré-alocado) ----------------------------------------
    Grain m_grains[kMaxGrains];
    std::array<int, kMaxGrains> m_activeList;
    int m_activeCount = 0;
    std::array<int, kMaxGrains> m_freeList;
    int m_freeCount = 0;

    // ---- Estado de spawn ----------------------------------------------------
    double m_spawnAccum = 0.0;           // acumulador fracionário de spawn
    uint32_t m_rngSeed = 12345u;         // LCG com semente fixa (determinístico)

    // Gera um número aleatório em [0, 1) usando LCG com semente fixa.
    float randomFloat();

    // ---- Ponteiro do sample (não proprietário) ------------------------------
    const SampleData* m_sample = nullptr;

    // ---- Smoothing ----------------------------------------------------------
    LinearSmoother m_levelSmoother;

    float m_level = 0.7f;
    float m_pan = 0.0f;
    bool  m_active = true;

    int m_octave = 0;
    int m_semitone = 0;
    float m_detuneCents = 0.0f;
    float m_fineTuneCents = 0.0f;
    float m_initialPhase = 0.0f;

    double m_sampleRate = 44100.0;
};

} // namespace dsp
} // namespace omnivy
