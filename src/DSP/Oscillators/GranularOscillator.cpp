#include "DSP/Oscillators/GranularOscillator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>
#include <algorithm>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Constantes
// -----------------------------------------------------------------------------
static constexpr float kMinGrainSize = 0.01f;   // 10 ms
static constexpr float kMaxGrainSize = 2.0f;    // 2 s
static constexpr float kMinGrainDensity = 0.0f; // 0 grãos/s
static constexpr float kMaxGrainDensity = 1000.0f; // 1.000 grãos/s
static constexpr float kMinPitch = -48.0f;
static constexpr float kMaxPitch = 48.0f;
static constexpr float kMinPitchRand = 0.0f;
static constexpr float kMaxPitchRand = 24.0f;

// -----------------------------------------------------------------------------
// Construtor
// -----------------------------------------------------------------------------
GranularOscillator::GranularOscillator()
{
    m_levelSmoother.setSmoothTime(0.002f); // 2 ms level smoothing

    // Inicializa a lista livre com todos os índices do pool.
    for (int i = 0; i < kMaxGrains; ++i)
        m_freeList[i] = i;
    m_freeCount = kMaxGrains;
    m_activeCount = 0;
}

// -----------------------------------------------------------------------------
// Ciclo de vida
// -----------------------------------------------------------------------------
void GranularOscillator::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_levelSmoother.setSampleRate(sampleRate);
}

void GranularOscillator::reset()
{
    // Desativa todos os grãos e os devolve à lista livre.
    for (int i = 0; i < m_activeCount; ++i)
    {
        int idx = m_activeList[i];
        m_grains[idx].active = false;
        m_grains[idx].activeListIndex = -1;
        m_freeList[m_freeCount++] = idx;
    }
    m_activeCount = 0;

    m_spawnAccum = 0.0;
    m_rngSeed = 12345u;
    m_levelSmoother.reset(m_level);
}

// -----------------------------------------------------------------------------
// Controles básicos (OscillatorBase)
// -----------------------------------------------------------------------------
void GranularOscillator::setFrequency(float frequencyHz)
{
    m_frequencyHz = frequencyHz;
}

float GranularOscillator::getFrequency() const
{
    return m_frequencyHz;
}

void GranularOscillator::setLevel(float level)
{
    m_level = std::clamp(level, 0.0f, 2.0f);
}

float GranularOscillator::getLevel() const
{
    return m_level;
}

void GranularOscillator::setPan(float pan)
{
    m_pan = std::clamp(pan, -1.0f, 1.0f);
}

float GranularOscillator::getPan() const
{
    return m_pan;
}

void GranularOscillator::setActive(bool active)
{
    m_active = active;
    if (!active)
        reset();
}

bool GranularOscillator::isActive() const
{
    return m_active;
}

void GranularOscillator::setOctave(int octave)
{
    m_octave = octave;
}

void GranularOscillator::setSemitone(int semitone)
{
    m_semitone = semitone;
}

void GranularOscillator::setDetuneCents(float cents)
{
    m_detuneCents = std::clamp(cents, -50.0f, 50.0f);
}

void GranularOscillator::setFineTuneCents(float cents)
{
    m_fineTuneCents = std::clamp(cents, -50.0f, 50.0f);
}

void GranularOscillator::setInitialPhase(float phase)
{
    m_initialPhase = wrapPhase(phase);
}

float GranularOscillator::getPhase() const
{
    return m_initialPhase;
}

// -----------------------------------------------------------------------------
// Parâmetros granulares
// -----------------------------------------------------------------------------
void GranularOscillator::setGrainSize(float seconds)
{
    m_grainSize = std::clamp(seconds, kMinGrainSize, kMaxGrainSize);
}

void GranularOscillator::setGrainDensity(float grainsPerSecond)
{
    m_grainDensity = std::clamp(grainsPerSecond, kMinGrainDensity, kMaxGrainDensity);
}

void GranularOscillator::setPosition(float position)
{
    m_position = std::clamp(position, 0.0f, 1.0f);
}

void GranularOscillator::setPositionSpray(float spray)
{
    m_positionSpray = std::clamp(spray, 0.0f, 1.0f);
}

void GranularOscillator::setPitch(float semitones)
{
    m_pitchSemitones = std::clamp(semitones, kMinPitch, kMaxPitch);
}

void GranularOscillator::setPitchRandomization(float semitones)
{
    m_pitchRandomization = std::clamp(semitones, kMinPitchRand, kMaxPitchRand);
}

void GranularOscillator::setGrainEnvelope(GrainEnvelope envelope)
{
    m_grainEnvelope = envelope;
}

void GranularOscillator::setStereoSpread(float spread)
{
    m_stereoSpread = std::clamp(spread, 0.0f, 1.0f);
}

void GranularOscillator::setPlaybackDirection(PlaybackDirection direction)
{
    m_playbackDirection = direction;
}

// -----------------------------------------------------------------------------
// DSP internos
// -----------------------------------------------------------------------------
float GranularOscillator::effectivePitchRatio() const noexcept
{
    const float ratio = centsToRatio(m_detuneCents + m_fineTuneCents)
                      * static_cast<float>(std::pow(2.0, m_semitone / 12.0))
                      * static_cast<float>(std::pow(2.0, m_octave))
                      * static_cast<float>(std::pow(2.0, m_pitchSemitones / 12.0));
    return ratio;
}

float GranularOscillator::applyEnvelope(float envPhase) const noexcept
{
    switch (m_grainEnvelope)
    {
        case GrainEnvelope::None:
            return 1.0f;
        case GrainEnvelope::Linear:
            return 1.0f - envPhase;
        case GrainEnvelope::Triangle:
            return envPhase < 0.5f ? 2.0f * envPhase : 2.0f * (1.0f - envPhase);
        case GrainEnvelope::Hanning:
            return 0.5f - 0.5f * static_cast<float>(std::cos(TwoPi * envPhase));
        default:
            return 1.0f;
    }
}

float GranularOscillator::readBuffer(int channel, double position) const noexcept
{
    if (m_sample == nullptr || m_sample->isEmpty())
        return 0.0f;

    const float* data = m_sample->getChannelData(channel);
    const int numSamples = m_sample->getNumSamples();

    if (numSamples <= 1)
        return 0.0f;

    // Fora dos bounds: silêncio (o grão será desativado no passo seguinte).
    if (position < 0.0 || position >= static_cast<double>(numSamples - 1))
    {
        if (position >= static_cast<double>(numSamples - 1))
            return data[numSamples - 1];
        return 0.0f;
    }

    const int idx = static_cast<int>(position);
    const double frac = position - static_cast<double>(idx);
    return static_cast<float>(
        data[idx] * (1.0 - frac) + data[idx + 1] * frac);
}

float GranularOscillator::randomFloat()
{
    m_rngSeed = m_rngSeed * 1664525u + 1013904223u;
    return static_cast<float>(m_rngSeed) * (1.0f / 4294967296.0f);
}

bool GranularOscillator::spawnGrain()
{
    if (m_freeCount == 0)
        return false; // pool cheio — spawn silenciosamente ignorado

    const int idx = m_freeList[--m_freeCount];
    Grain& grain = m_grains[idx];

    // ---- Posição de leitura -------------------------------------------------
    float pos = m_position;
    if (m_positionSpray > 0.0f)
    {
        const float r = randomFloat() * 2.0f - 1.0f;
        pos += m_positionSpray * r;
    }
    pos = std::clamp(pos, 0.0f, 1.0f);

    const int numSamples = m_sample->getNumSamples();
    grain.position = pos * static_cast<double>(numSamples > 1 ? numSamples - 1 : 0);

    // ---- Pitch --------------------------------------------------------------
    float ratio = effectivePitchRatio();
    if (m_pitchRandomization > 0.0f)
    {
        const float r = randomFloat() * 2.0f - 1.0f;
        const float randSemi = m_pitchRandomization * r;
        ratio *= centsToRatio(randSemi);
    }

    const double bufSampleRate = m_sample->getSampleRate();
    const double rateRatio = m_sampleRate / bufSampleRate;
    grain.increment = ratio * rateRatio;

    if (m_playbackDirection == PlaybackDirection::Reverse)
        grain.increment = -grain.increment;

    // ---- Duração ------------------------------------------------------------
    int samplesLeft = static_cast<int>(m_grainSize * m_sampleRate);
    if (samplesLeft < 1)
        samplesLeft = 1;
    grain.samplesLeft = samplesLeft;

    // ---- Ganho por grão -----------------------------------------------------
    // Conservador para evitar clipping com muitos grãos sobrepostos.
    grain.gain = 0.3f;

    // ---- Pan (com espalhamento estéreo) -------------------------------------
    float pan = m_pan;
    if (m_stereoSpread > 0.0f)
    {
        const float r = randomFloat() * 2.0f - 1.0f;
        pan += m_stereoSpread * r;
    }
    grain.pan = std::clamp(pan, -1.0f, 1.0f);

    // ---- Envelope -----------------------------------------------------------
    grain.envPhase = 0.0f;
    grain.envInc = samplesLeft > 0 ? 1.0f / static_cast<float>(samplesLeft) : 1.0f;

    // ---- Ativa --------------------------------------------------------------
    grain.active = true;
    grain.activeListIndex = m_activeCount;
    m_activeList[m_activeCount] = idx;
    ++m_activeCount;

    return true;
}

// -----------------------------------------------------------------------------
// Processamento de uma amostra estéreo
// -----------------------------------------------------------------------------
StereoFrame GranularOscillator::processSample()
{
    if (!m_active || m_sample == nullptr || m_sample->isEmpty())
        return StereoFrame{0.0f, 0.0f};

    const int numSamples = m_sample->getNumSamples();
    const double bufSampleRate = m_sample->getSampleRate();
    const double rateRatio = m_sampleRate / bufSampleRate;
    juce::ignoreUnused(rateRatio); // já incorporado no increment do grão

    // ---- 1. Spawn de novos grãos -------------------------------------------
    m_spawnAccum += m_grainDensity / m_sampleRate;
    int grainsToSpawn = static_cast<int>(m_spawnAccum);
    m_spawnAccum -= static_cast<double>(grainsToSpawn);

    for (int g = 0; g < grainsToSpawn; ++g)
    {
        if (!spawnGrain())
            break;
    }

    // ---- 2. Processa grãos ativos ------------------------------------------
    float mixL = 0.0f;
    float mixR = 0.0f;

    int i = 0;
    while (i < m_activeCount)
    {
        const int idx = m_activeList[i];
        Grain& grain = m_grains[idx];

        // Leitura do buffer (interpolação linear)
        const float sL = readBuffer(0, grain.position);
        const float sR = m_sample->isStereo()
            ? readBuffer(1, grain.position)
            : sL;

        // Envelope
        const float env = applyEnvelope(grain.envPhase);
        const float g = grain.gain * env;

        // Ganhos de pan (constant power)
        const auto pg = applyPan(1.0f, grain.pan);
        const float panL = pg.left;
        const float panR = pg.right;

        float outL = 0.0f;
        float outR = 0.0f;

        if (m_sample->isStereo())
        {
            // Fonte estéreo: pan como balanço entre canais.
            outL = sL * g * panL;
            outR = sR * g * panR;

            // Stereo Spread: adiciona largura extra a partir do sinal diferença.
            if (m_stereoSpread > 0.0f)
            {
                const float mid = (outL + outR) * 0.5f;
                const float side = (outL - outR) * 0.5f;
                const float width = 1.0f + m_stereoSpread;
                outL = mid + side * width;
                outR = mid - side * width;
            }
        }
        else
        {
            // Fonte mono: pan do sinal mono.
            outL = sL * g * panL;
            outR = sL * g * panR;

            // Stereo Spread: largura artificial para fonte mono.
            if (m_stereoSpread > 0.0f)
            {
                const float mid = (outL + outR) * 0.5f;
                const float side = (outL - outR) * 0.5f;
                const float width = 1.0f + m_stereoSpread;
                outL = mid + side * width;
                outR = mid - side * width;
            }
        }

        mixL += outL;
        mixR += outR;

        // ---- Avança o grão --------------------------------------------------
        grain.position += grain.increment;
        grain.envPhase += grain.envInc;
        --grain.samplesLeft;

        // ---- Verifica se o grão morreu -------------------------------------
        bool dead = false;
        if (grain.samplesLeft <= 0)
        {
            dead = true;
        }
        else if (m_playbackDirection == PlaybackDirection::Forward)
        {
            if (grain.position >= static_cast<double>(numSamples - 1))
                dead = true;
        }
        else // Reverse
        {
            if (grain.position < 0.0)
                dead = true;
        }

        if (dead)
        {
            // Remove da lista ativa (swap com o último) — O(1).
            const int lastIdx = m_activeList[m_activeCount - 1];
            m_activeList[i] = lastIdx;
            m_grains[lastIdx].activeListIndex = i;
            m_grains[idx].active = false;
            m_grains[idx].activeListIndex = -1;
            m_freeList[m_freeCount++] = idx;
            --m_activeCount;
            // Não incrementa i: o novo elemento na posição i deve ser processado.
        }
        else
        {
            ++i;
        }
    }

    // ---- 3. Nível de saída (suavizado) -------------------------------------
    const float smoothedLevel = m_levelSmoother.getNextValue();
    mixL *= smoothedLevel;
    mixR *= smoothedLevel;

    // ---- 4. Proteção contra NaN/Inf ----------------------------------------
    if (!std::isfinite(mixL))
        mixL = 0.0f;
    if (!std::isfinite(mixR))
        mixR = 0.0f;

    return StereoFrame{mixL, mixR};
}

} // namespace dsp
} // namespace omnivy
