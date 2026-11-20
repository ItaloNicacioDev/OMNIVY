#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/KnobComponent.h"
#include "GUI/PanelComponent.h"
#include "DSP/Routing/Connection.h"

class OmnivyAudioProcessor;

namespace omnivy {
namespace dsp {
class RoutingGraph;
} // namespace dsp

namespace gui {

// -----------------------------------------------------------------------------
// RoutingPanel
//
// Grafo de roteamento real entre os seis osciladores (nós 0..5). As conexões
// (Áudio, FM, PM, RingMod, HardSync) são manipuladas no RoutingGraph existente
// e publicadas para a thread de áudio via publishConfig(). Não é um grafo
// decorativo: representa roteamentos efetivos do sintetizador.
// -----------------------------------------------------------------------------
class RoutingPanel : public juce::Component
{
public:
    RoutingPanel (OmnivyAudioProcessor& processor);

    ~RoutingPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OmnivyAudioProcessor& m_processor;

    // Área de desenho do grafo.
    class GraphCanvas : public juce::Component
    {
    public:
        explicit GraphCanvas (OmnivyAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics&) override;
    private:
        OmnivyAudioProcessor& processor;
    };

    GraphCanvas m_graphCanvas { m_processor };

    juce::ComboBox* m_sourceCombo = nullptr;
    juce::ComboBox* m_destCombo = nullptr;
    juce::ComboBox* m_typeCombo = nullptr;
    juce::Slider* m_gainSlider = nullptr;
    juce::TextButton* m_addButton = nullptr;
    juce::TextButton* m_clearButton = nullptr;
    juce::Label m_statusLabel;

    void populateNodeCombo (juce::ComboBox&);
    void addConnection();
    void clearConnections();
    void refreshStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoutingPanel)
};

} // namespace gui
} // namespace omnivy
