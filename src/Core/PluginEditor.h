#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"
#include "GUI/KnobComponent.h"
#include "GUI/PanelComponent.h"
#include "GUI/OscillatorPanel.h"
#include "GUI/WavetablePanel.h"
#include "GUI/RoutingPanel.h"
#include "GUI/FilterPanel.h"
#include "GUI/EnvelopePanel.h"
#include "GUI/VisualizerPanel.h"

namespace omnivy {
namespace gui {

class TopBarComponent;
class TabBarComponent;
class ModulationMatrixPanel;
class ArpeggiatorPanel;
class BottomBandComponent;

// -----------------------------------------------------------------------------
// OmnivyAudioProcessorEditor
//
// Interface principal em cinco faixas (top bar, tab bar, painéis superiores,
// painéis inferiores, teclado + master + footer) com identidade Dark Neon /
// Cyber-Synth. Proporção próxima de 16:9 (1648 × 920 de referência).
//
// Todos os controles gravam em parâmetros reais do AudioProcessorValueTreeState
// e nenhum controle é deixado sem conexão com o DSP.
// -----------------------------------------------------------------------------
class OmnivyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OmnivyAudioProcessorEditor (OmnivyAudioProcessor&);
    ~OmnivyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OmnivyAudioProcessor& audioProcessor;
    omnivy::gui::OmnivyLookAndFeel m_lookAndFeel;

    // Faixa 1 — Top Bar
    TopBarComponent* m_topBar = nullptr;

    // Faixa 2 — Tab Bar (seis abas)
    TabBarComponent* m_tabBar = nullptr;

    // Faixa 3 — Painéis superiores (quatro colunas)
    OscillatorPanel*   m_oscPanel    = nullptr;
    WavetablePanel*    m_wavetablePanel = nullptr;
    RoutingPanel*      m_routingPanel = nullptr;
    FilterPanel*       m_filterPanel = nullptr;

    // Faixa 4 — Painéis inferiores (quatro colunas)
    ModulationMatrixPanel* m_modMatrixPanel = nullptr;
    juce::TabbedComponent* m_envTabbed = nullptr;   // Envelopes / LFOs / Sequencer
    VisualizerPanel*     m_visualizerPanel = nullptr;
    ArpeggiatorPanel*    m_arpPanel = nullptr;

    // Faixa 5 — Teclado + Master + Footer
    BottomBandComponent* m_bottomBand = nullptr;

    int m_activeTab = 0;

    void tabSelected (int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyAudioProcessorEditor)
};

} // namespace gui
} // namespace omnivy
