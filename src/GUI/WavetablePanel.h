#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/WaveformComponent.h"
#include "GUI/PanelComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// WavetablePanel
//
// Visualiza a wavetable realmente usada pelo motor (atualmente uma tabela de
// seno gerada em AudioEngine::prepare). A seleção reflete a tabela efetiva.
// Importar/editar a wavetable ainda não existe no DSP, portanto esses comandos
// são apresentados como indisponíveis (sem simulação).
// -----------------------------------------------------------------------------
class WavetablePanel : public juce::Component
{
public:
    WavetablePanel (juce::AudioProcessorValueTreeState& /*apvts*/);

    ~WavetablePanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::ComboBox* m_wavetableSelector = nullptr;
    WaveformComponent* m_waveformPreview = nullptr;
    juce::Label m_infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetablePanel)
};

} // namespace gui
} // namespace omnivy
