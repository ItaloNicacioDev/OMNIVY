#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// SampleData — buffer de áudio imutável (mono ou estéreo) com sample rate.
//
// Carregado exclusivamente fora da thread de áudio. Uma vez construído, seus
// dados nunca são modificados, o que permite publicação lock-free para a
// thread de áudio via SamplePublisher.
// -----------------------------------------------------------------------------
class SampleData
{
public:
    SampleData() = default;
    ~SampleData() = default;

    SampleData (const SampleData&) = delete;
    SampleData& operator= (const SampleData&) = delete;

    SampleData (SampleData&&) noexcept = default;
    SampleData& operator= (SampleData&&) noexcept = default;

    // Constrói um sample em silêncio com os canais e tamanho dados.
    void allocate (int numChannels, int numSamples, double sampleRate)
    {
        m_sampleRate = sampleRate;
        m_numSamples = numSamples;
        m_channels.clear();
        m_channels.reserve (static_cast<size_t> (numChannels));
        for (int c = 0; c < numChannels; ++c)
            m_channels.emplace_back (static_cast<size_t> (numSamples), 0.0f);
    }

    // Carrega um arquivo de áudio fora da thread de áudio (message thread).
    // Retorna false se o arquivo não puder ser lido.
    bool loadFromFile (const juce::File& file)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader (
            formatManager.createReaderFor (file));
        if (!reader)
            return false;

        const int numChannels = static_cast<int> (reader->numChannels);
        const int numSamples  = static_cast<int> (reader->lengthInSamples);
        m_sampleRate = reader->sampleRate;
        m_numSamples = numSamples;

        m_channels.clear();
        m_channels.reserve (static_cast<size_t> (numChannels));
        for (int c = 0; c < numChannels; ++c)
            m_channels.emplace_back (static_cast<size_t> (numSamples), 0.0f);

        juce::AudioBuffer<float> temp (numChannels, numSamples);
        reader->read (&temp, 0, numSamples, 0, true, true);

        for (int c = 0; c < numChannels; ++c)
        {
            const float* src = temp.getReadPointer (c);
            float* dst = m_channels[c].data();
            std::memcpy (dst, src, static_cast<size_t> (numSamples) * sizeof (float));
        }

        return true;
    }

    // Constrói um sample a partir de buffers de memória já existentes.
    // Os dados são copiados (SampleData é proprietário).
    void fromMemory (const float* left, const float* right,
                     int numSamples, double sampleRate,
                     bool isStereo)
    {
        m_sampleRate = sampleRate;
        m_numSamples = numSamples;

        m_channels.clear();
        m_channels.emplace_back (static_cast<size_t> (numSamples));
        if (left != nullptr)
            std::memcpy (m_channels[0].data(), left,
                         static_cast<size_t> (numSamples) * sizeof (float));
        else
            std::memset (m_channels[0].data(), 0,
                         static_cast<size_t> (numSamples) * sizeof (float));

        if (isStereo && right != nullptr)
        {
            m_channels.emplace_back (static_cast<size_t> (numSamples));
            std::memcpy (m_channels[1].data(), right,
                         static_cast<size_t> (numSamples) * sizeof (float));
        }
        else
        {
            // Fonte mono: canal direito espelha o esquerdo.
            m_channels.emplace_back (m_channels[0]);
        }
    }

    // ----- Consultas ----------------------------------------------------------
    int    getNumChannels() const noexcept { return static_cast<int> (m_channels.size()); }
    int    getNumSamples() const noexcept { return m_numSamples; }
    double getSampleRate() const noexcept { return m_sampleRate; }
    bool   isEmpty() const noexcept { return m_numSamples == 0 || m_channels.empty(); }
    bool   isStereo() const noexcept { return getNumChannels() >= 2; }

    const float* getChannelData (int channel) const noexcept
    {
        const int idx = std::clamp (channel, 0, static_cast<int> (m_channels.size()) - 1);
        return m_channels[idx].data();
    }

    float* getChannelData (int channel) noexcept
    {
        const int idx = std::clamp (channel, 0, static_cast<int> (m_channels.size()) - 1);
        return m_channels[idx].data();
    }

    // Preenche o buffer com um padrão sentinela (útil para testes de
    // integridade e detecção de uso-após-liberação).
    void fillSentinel (float value) noexcept
    {
        for (auto& ch : m_channels)
            std::fill (ch.begin(), ch.end(), value);
    }

private:
    std::vector<std::vector<float>> m_channels;
    int m_numSamples = 0;
    double m_sampleRate = 44100.0;
};

} // namespace dsp
} // namespace omnivy
