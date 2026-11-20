#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"
#include "GUI/EnvelopeDisplay.h"
#include "GUI/WaveformComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// VisualizerPanel
//
// Visualizações com dados reais do sintetizador: a forma de onda do oscilador
// 1 (parâmetro oscWaveform) e a curva do envelope de amplitude (parâmetros
// env*). Escopo/espetro de áudio em tempo real exigiriam uma captura de buffer
// de saída que ainda não existe no DSP, portanto não são simulados.
// -----------------------------------------------------------------------------
class VisualizerPanel : public juce::Component,
                        public juce::AudioProcessorValueTreeState::Listener
{
public:
    VisualizerPanel (juce::AudioProcessorValueTreeState& apvts);

    ~VisualizerPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged (const juce::String& /*parameterID*/, float /*newValue*/) override;

private:
    juce::AudioProcessorValueTreeState& m_apvts;

    WaveformComponent* m_waveformPreview = nullptr;
    EnvelopeDisplay* m_envelopeDisplay = nullptr;
    juce::Label m_scopeNote;

    void refresh();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizerPanel)
};

} // namespace gui
} // namespace omnivy
