#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "DSP/Utilities/DSPUtilities.h"
#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// WaveformComponent
//
// Desenha a representação matemática de uma forma de onda (Sine, Saw, Square,
// Triangle, Pulse, Noise). O tipo de onda é um parâmetro real do sintetizador
// (oscWaveform), portanto a pré-visualização reflete o tipo selecionado — não
// é um desenho genérico nem dados fictícios.
// -----------------------------------------------------------------------------
class WaveformComponent : public juce::Component
{
public:
    WaveformComponent();

    ~WaveformComponent() override = default;

    void setWaveform (dsp::Waveform waveform);
    dsp::Waveform getWaveform() const noexcept { return m_waveform; }

    void setColour (juce::Colour colour);
    void setSizeHint (int width, int height);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    dsp::Waveform m_waveform = dsp::Waveform::Sine;
    juce::Colour m_waveColour = PrimaryPurple;
    int m_thumbW = 64;
    int m_thumbH = 40;

    // Gera um ciclo da forma de onda em pontos normalizados (0..1 x, -1..1 y).
    juce::Path buildWaveformPath (dsp::Waveform w, int numPoints) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformComponent)
};

} // namespace gui
} // namespace omnivy
