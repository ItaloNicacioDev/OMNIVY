#include "GUI/RoutingPanel.h"

#include "Core/PluginProcessor.h"

#include "Parameters/ParameterIDs.h"

#include "DSP/Routing/Connection.h"
#include "DSP/Routing/RoutingGraph.h"

#include <cmath>

namespace omnivy {
namespace gui {

namespace
{
juce::Colour typeColour (dsp::ConnectionType type)
{
    switch (type)
    {
        case dsp::ConnectionType::Audio:             return Osc1Colour;
        case dsp::ConnectionType::FrequencyModulation: return Osc2Colour;
        case dsp::ConnectionType::PhaseModulation:   return Osc3Colour;
        case dsp::ConnectionType::RingModulation:    return Osc4Colour;
        case dsp::ConnectionType::HardSync:          return PrimaryPurple;
        default:                                     return TextMuted;
    }
}

}

RoutingPanel::RoutingPanel (OmnivyAudioProcessor& processor)
    : m_processor (processor), m_graphCanvas (processor)
{
    setInterceptsMouseClicks (true, true);
    addAndMakeVisible (m_graphCanvas);

    m_sourceCombo = new juce::ComboBox ("Origem");
    populateNodeCombo (*m_sourceCombo);
    addAndMakeVisible (*m_sourceCombo);

    m_destCombo = new juce::ComboBox ("Destino");
    populateNodeCombo (*m_destCombo);
    addAndMakeVisible (*m_destCombo);

    m_typeCombo = new juce::ComboBox ("Tipo");
    m_typeCombo->addItem ("Áudio", 1);
    m_typeCombo->addItem ("FM", 2);
    m_typeCombo->addItem ("PM", 3);
    m_typeCombo->addItem ("Ring", 4);
    m_typeCombo->addItem ("Hard Sync", 5);
    m_typeCombo->setSelectedId (1);
    addAndMakeVisible (*m_typeCombo);

    m_gainSlider = new juce::Slider ("Ganho");
    m_gainSlider->setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    m_gainSlider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    m_gainSlider->setRange (0.0, 1.0, 0.01);
    m_gainSlider->setValue (1.0);
    m_gainSlider->setTooltip ("Ganho da nova conexão de roteamento.");
    addAndMakeVisible (*m_gainSlider);

    m_addButton = new juce::TextButton ("Adicionar");
    m_addButton->onClick = [this] { addConnection(); };
    addAndMakeVisible (*m_addButton);

    m_clearButton = new juce::TextButton ("Limpar");
    m_clearButton->onClick = [this] { clearConnections(); };
    addAndMakeVisible (*m_clearButton);

    m_statusLabel.setText ("Nenhuma conexão", juce::dontSendNotification);
    m_statusLabel.setFont (uiFont (FontSizeMicro));
    m_statusLabel.setColour (juce::Label::textColourId, TextMuted);
    m_statusLabel.setEnabled (false);
    addAndMakeVisible (m_statusLabel);

    startTimerHz (15);
}

RoutingPanel::~RoutingPanel()
{
}

void RoutingPanel::timerCallback()
{
    refreshStatus();
}

void RoutingPanel::populateNodeCombo (juce::ComboBox& combo)
{
    for (int i = 0; i < omnivy::params::NumOscillators; ++i)
        combo.addItem ("OSC " + juce::String (i + 1), i + 1);
    combo.setSelectedId (1);
}

void RoutingPanel::GraphCanvas::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (AreaInner);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    auto& graph = processor.getRoutingGraph();

    int numNodes = omnivy::params::NumOscillators;
    float nodeRadius = 22.0f;
    float centerY = area.getCentreY();
    float startX = area.getX() + 60.0f;
    float step = (area.getWidth() - 120.0f) / (float) (numNodes - 1);

    // Posições dos nós.
    juce::Point<float> nodePos[omnivy::params::NumOscillators];
    for (int i = 0; i < numNodes; ++i)
        nodePos[i] = { startX + i * step, centerY };

    // Desenha conexões (curvas) — trás da frente dos nós.
    for (int src = 0; src < numNodes; ++src)
    {
        auto outgoing = graph.getOutgoingConnections (src);
        for (const auto& conn : outgoing)
        {
            if (!conn || !conn->isEnabled())
                continue;
            int dst = conn->getDestination();
            if (dst < 0 || dst >= numNodes)
                continue;

            auto c = typeColour (conn->getType());
            juce::Path curve;
            curve.startNewSubPath (nodePos[src]);
            // Ponto de controle acima da linha para formar um arco.
            juce::Point<float> cp ((nodePos[src].getX() + nodePos[dst].getX()) * 0.5f,
                                   centerY - 60.0f);
            curve.quadraticTo (cp, nodePos[dst]);

            g.setColour (c.withAlpha (0.5f));
            g.strokePath (curve, juce::PathStrokeType (2.0f));

            // Seta no destino.
            g.setColour (c);
            g.fillEllipse (nodePos[dst].getX() - 3.0f, nodePos[dst].getY() - 3.0f, 6.0f, 6.0f);
        }
    }

    // Desenha nós.
    for (int i = 0; i < numNodes; ++i)
    {
        bool hasAny = graph.getNumConnections() > 0
            && (graph.getOutgoingConnections (i).size() > 0
                || graph.getIncomingConnections (i).size() > 0);
        g.setColour (oscillatorColour (i));
        if (hasAny)
            g.setColour (oscillatorColour (i).brighter (0.3f));
        g.fillEllipse (nodePos[i].getX() - nodeRadius, nodePos[i].getY() - nodeRadius,
                       nodeRadius * 2.0f, nodeRadius * 2.0f);
        g.setColour (Border);
        g.drawEllipse (nodePos[i].getX() - nodeRadius, nodePos[i].getY() - nodeRadius,
                       nodeRadius * 2.0f, nodeRadius * 2.0f, 1.5f);

        g.setColour (TextMain);
        g.setFont (uiFont (12.0f, juce::Font::bold));
        g.drawText (juce::String (i + 1),
                    (int) (nodePos[i].getX() - 10), (int) (nodePos[i].getY() - 10),
                    20, 20, juce::Justification::centred, true);
    }

    // Labels dos nós abaixo.
    g.setColour (TextSecondary);
    g.setFont (uiFont (FontSizeMicro));
    for (int i = 0; i < numNodes; ++i)
        g.drawText ("OSC " + juce::String (i + 1),
                    (int) (nodePos[i].getX() - 30), (int) (centerY + nodeRadius + 6),
                    60, 14, juce::Justification::centred, true);
}

void RoutingPanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
}

void RoutingPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (44);

    // Grafo ocupa a maior parte.
    m_graphCanvas.setBounds (area.removeFromLeft (area.getWidth() * 0.62f));
    area.removeFromLeft (14);

    int h = 26;
    int gap = 8;
    int labelW = 60;

    m_sourceCombo->setBounds (area.removeFromLeft (labelW));
    m_sourceCombo->setEditableText (false);
    area.removeFromLeft (gap);
    m_destCombo->setBounds (area.removeFromLeft (labelW));
    area.removeFromLeft (gap);
    m_typeCombo->setBounds (area.removeFromLeft (labelW));
    area.removeFromLeft (gap);

    m_gainSlider->setBounds (area.removeFromLeft (90));
    area.removeFromLeft (gap);

    m_addButton->setBounds (area.removeFromLeft (70));
    area.removeFromLeft (gap);
    m_clearButton->setBounds (area.removeFromLeft (70));

    m_statusLabel.setBounds (area.removeFromBottom (20));
}

void RoutingPanel::addConnection()
{
    int src = m_sourceCombo->getSelectedItemIndex();
    int dst = m_destCombo->getSelectedItemIndex();
    if (src == dst)
    {
        refreshStatus();
        return;
    }

    int typeId = m_typeCombo->getSelectedId();
    float gain = static_cast<float> (m_gainSlider->getValue());

    auto& graph = m_processor.getRoutingGraph();
    bool ok = false;

    switch (static_cast<dsp::ConnectionType> (typeId))
    {
        case dsp::ConnectionType::Audio:
            ok = graph.addConnection (std::make_shared<dsp::AudioConnection> (src, dst, gain));
            break;
        case dsp::ConnectionType::FrequencyModulation:
            ok = graph.addConnection (std::make_shared<dsp::FrequencyModulationConnection> (src, dst, gain));
            break;
        case dsp::ConnectionType::PhaseModulation:
            ok = graph.addConnection (std::make_shared<dsp::PhaseModulationConnection> (src, dst, gain));
            break;
        case dsp::ConnectionType::RingModulation:
            ok = graph.addConnection (std::make_shared<dsp::RingModulationConnection> (src, dst, gain));
            break;
        case dsp::ConnectionType::HardSync:
            ok = graph.addConnection (std::make_shared<dsp::HardSyncConnection> (src, dst, gain));
            break;
        default:
            break;
    }

    if (ok)
        graph.publishConfig();

    m_graphCanvas.repaint();
    refreshStatus();
}

void RoutingPanel::clearConnections()
{
    auto& graph = m_processor.getRoutingGraph();
    graph.clearAllConnections();
    graph.publishConfig();
    m_graphCanvas.repaint();
    refreshStatus();
}

void RoutingPanel::refreshStatus()
{
    int n = m_processor.getRoutingGraph().getNumConnections();
    m_statusLabel.setText (n == 0 ? "Nenhuma conexão"
                                  : juce::String (n) + " conexão" + (n > 1 ? "ões" : ""),
                           juce::dontSendNotification);
}

} // namespace gui
} // namespace omnivy
