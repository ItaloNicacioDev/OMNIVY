#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "GUI/DesignSystem.h"
#include "PluginProcessor.h"

// Forward declarations dos painéis de conteúdo (definidos em PluginEditor.cpp).
namespace omnivy {
namespace gui {
class OscillatorPanel;
class WavetablePanel;
class RoutingPanel;
class FilterPanel;
class EnvelopePanel;
class VisualizerPanel;
class TopBarComponent;
class MasterBandComponent;
} // namespace gui
} // namespace omnivy

// -----------------------------------------------------------------------------
// OmnivyAudioProcessorEditor
//
// Interface principal: cinco faixas (top bar, tab bar, painéis superiores,
// painéis inferiores, teclado + master) com identidade Dark Neon / Cyber-Synth.
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

    OmnivyLookAndFeel m_lookAndFeel;

    juce::TabbedComponent m_tabbed { juce::TabbedButtonBar::TabsAtTop };

    omnivy::gui::TopBarComponent* m_topBar = nullptr;
    omnivy::gui::MasterBandComponent* m_masterBand = nullptr;

    // Índices das abas.
    static constexpr int TabOscillators = 0;
    static constexpr int TabWavetable = 1;
    static constexpr int TabRouting = 2;
    static constexpr int TabFilter = 3;
    static constexpr int TabEnvelope = 4;
    static constexpr int TabVisualizer = 5;

    void buildTabs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyAudioProcessorEditor)
};
