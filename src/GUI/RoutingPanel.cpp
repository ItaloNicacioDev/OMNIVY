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

    m_sourceCombo = new juce::ComboBox ("Source");
    populateNodeCombo (*m_sourceCombo);
    addAndMakeVisible (*m_sourceCombo);

    m_destCombo = new juce::ComboBox ("Destination");
    populateNodeCombo (*m_destCombo);
    addAndMakeVisible (*m_destCombo);

    m_typeCombo = new juce::ComboBox ("Type");
    m_typeCombo->addItem ("Audio", 1);
    m_typeCombo->addItem ("FM", 2);
    m_typeCombo->addItem ("PM", 3);
    m_typeCombo->addItem ("Ring", 4);
    m_typeCombo->addItem ("Hard Sync", 5);
    m_typeCombo->setSelectedId (1);
    addAndMakeVisible (*m_typeCombo);

    m_gainSlider = new juce::Slider ("Gain");
    m_gainSlider->setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    m_gainSlider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    m_gainSlider->setRange (0.0, 1.0, 0.01);
    m_gainSlider->setValue (1.0);
    m_gainSlider->setTooltip ("Gain of the new routing connection.");
    addAndMakeVisible (*m_gainSlider);

    m_addButton = new juce::TextButton ("Add");
    m_addButton->onClick = [this] { addConnection(); };
    addAndMakeVisible (*m_addButton);

    m_clearButton = new juce::TextButton ("Clear");
    m_clearButton->onClick = [this] { clearConnections(); };
    addAndMakeVisible (*m_clearButton);

    m_statusLabel.setText ("No connections", juce::dontSendNotification);
    m_statusLabel.setFont (uiFont (FontSizeMicro));
    m_statusLabel.setColour (juce::Label::textColourId, TextMuted);
    m_statusLabel.setEnabled (false);
    addAndMakeVisible (m_statusLabel);

    m_titleLabel.setText ("Routing Graph", juce::dontSendNotification);
    m_titleLabel.setFont (uiFont (FontSizeLabel, juce::Font::bold));
    m_titleLabel.setColour (juce::Label::textColourId, TextMain);
    m_titleLabel.setJustificationType (juce::Justification::left);
    m_titleLabel.setEnabled (false);
    addAndMakeVisible (m_titleLabel);

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

    // Fundo do grafo (PanelTop → PanelBottom).
    juce::ColourGradient bgGrad (PanelTop, area.getX(), area.getY(),
                                 PanelBottom, area.getX(), area.getBottom(),
                                 false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (PanelBorder);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    auto& graph = processor.getRoutingGraph();

    int numNodes = omnivy::params::NumOscillators;
    float centerY = area.getCentreY();

    // --- Posições das fontes (lado esquerdo) ---------------------------------
    float sourceStartX = area.getX() + 20.0f * getWidth() / 1672.0f;
    float sourceStep = (area.getWidth() * 0.35f - 40.0f * getWidth() / 1672.0f) / (float) (numNodes - 1);
    juce::Point<float> sourcePos[omnivy::params::NumOscillators];
    for (int i = 0; i < numNodes; ++i)
        sourcePos[i] = { sourceStartX + i * sourceStep, centerY };

    // --- Posições dos nós de processamento (centro-direita) ------------------
    float mixX = area.getRight() - 140.0f * getWidth() / 1672.0f;
    float filterX = area.getRight() - 80.0f * getWidth() / 1672.0f;
    float fxX = area.getRight() - 20.0f * getWidth() / 1672.0f;
    juce::Point<float> mixPos { mixX, centerY };
    juce::Point<float> filterPos { filterX, centerY };
    juce::Point<float> fxPos { fxX, centerY };

    // --- Desenha conexões das fontes para o Mix ------------------------------
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

            // Conexão da fonte para o nó Mix (curva Bézier).
            auto c = typeColour (conn->getType());
            juce::Path curve;
            curve.startNewSubPath (sourcePos[src]);
            curve.cubicTo (sourcePos[src].getX() + 60.0f, centerY - 30.0f,
                           mixPos.getX() - 60.0f, centerY + 30.0f,
                           mixPos.getX(), mixPos.getY());

            g.setColour (c.withAlpha (0.5f));
            g.strokePath (curve, juce::PathStrokeType (2.0f));
        }
    }

    // --- Conexões da cadeia: Mix → Filter → FX -------------------------------
    juce::Path chainCurve;
    chainCurve.startNewSubPath (mixPos.getX() + 22.0f, mixPos.getY());
    chainCurve.lineTo (filterPos.getX() - 22.0f, filterPos.getY());
    g.setColour (PrimaryPurpleLight);
    g.strokePath (chainCurve, juce::PathStrokeType (2.0f));

    juce::Path chainCurve2;
    chainCurve2.startNewSubPath (filterPos.getX() + 22.0f, filterPos.getY());
    chainCurve2.lineTo (fxPos.getX() - 22.0f, fxPos.getY());
    g.setColour (PrimaryPurpleLight);
    g.strokePath (chainCurve2, juce::PathStrokeType (2.0f));

    // --- Desenha nós de fonte (pílulas com ponto de cor) ----------------------
    for (int i = 0; i < numNodes; ++i)
    {
        auto c = oscillatorColour (i);
        float pillW = 56.0f * getWidth() / 1672.0f;
        float pillH = 20.0f * getHeight() / 941.0f;
        juce::Rectangle<float> pill (sourcePos[i].getX() - pillW * 0.5f,
                                     sourcePos[i].getY() - pillH * 0.5f,
                                     pillW, pillH);

        // Fundo da pílula.
        g.setColour (ControlSurface);
        g.fillRoundedRectangle (pill, pillH * 0.5f);
        g.setColour (c);
        g.drawRoundedRectangle (pill, pillH * 0.5f, 1.5f);

        // Ponto de cor à esquerda da pílula.
        g.setColour (c);
        g.fillEllipse (pill.getX() - 6.0f, pill.getCentreY() - 4.0f, 8.0f, 8.0f);

        // Texto do oscilador.
        g.setColour (TextMain);
        g.setFont (uiFont (11.0f, juce::Font::bold));
        g.drawText ("OSC " + juce::String (i + 1),
                    (int) pill.getX(), (int) (pill.getY() - pillH * 0.5f + 2),
                    (int) pill.getWidth(), (int) pillH,
                    juce::Justification::centred, true);
    }

    // --- Nó Mix (roxo, maior) --------------------------------------------------
    float mixRadius = 22.0f * getWidth() / 1672.0f;
    juce::Rectangle<float> mixRect (mixPos.getX() - mixRadius, mixPos.getY() - mixRadius,
                                    mixRadius * 2.0f, mixRadius * 2.0f);
    juce::ColourGradient mixGrad (PrimaryPurpleDark, mixRect.getX(), mixRect.getY(),
                                  PrimaryPurple, mixRect.getRight(), mixRect.getBottom(),
                                  false);
    g.setGradientFill (mixGrad);
    g.fillRoundedRectangle (mixRect, 8.0f);
    g.setColour (PrimaryPurpleLight);
    g.drawRoundedRectangle (mixRect, 8.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.setFont (uiFont (12.0f, juce::Font::bold));
    g.drawText ("Mix", (int) mixRect.getX(), (int) mixRect.getY(),
                (int) mixRect.getWidth(), (int) mixRect.getHeight(),
                juce::Justification::centred, true);

    // --- Nó Filter (azul escuro) -----------------------------------------------
    float filterRadius = 18.0f * getWidth() / 1672.0f;
    juce::Rectangle<float> filterRect (filterPos.getX() - filterRadius, filterPos.getY() - filterRadius,
                                       filterRadius * 2.0f, filterRadius * 2.0f);
    juce::ColourGradient filterGrad (PrimaryPurpleDark, filterRect.getX(), filterRect.getY(),
                                     PrimaryPurple, filterRect.getRight(), filterRect.getBottom(),
                                     false);
    g.setGradientFill (filterGrad);
    g.fillRoundedRectangle (filterRect, 6.0f);
    g.setColour (PrimaryPurpleLight);
    g.drawRoundedRectangle (filterRect, 6.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.setFont (uiFont (11.0f, juce::Font::bold));
    g.drawText ("Filter", (int) filterRect.getX(), (int) filterRect.getY(),
                (int) filterRect.getWidth(), (int) filterRect.getHeight(),
                juce::Justification::centred, true);

    // --- Nó FX (azul) ----------------------------------------------------------
    float fxRadius = 18.0f * getWidth() / 1672.0f;
    juce::Rectangle<float> fxRect (fxPos.getX() - fxRadius, fxPos.getY() - fxRadius,
                                   fxRadius * 2.0f, fxRadius * 2.0f);
    juce::ColourGradient fxGrad (PrimaryPurpleDark, fxRect.getX(), fxRect.getY(),
                                 PrimaryPurple, fxRect.getRight(), fxRect.getBottom(),
                                 false);
    g.setGradientFill (fxGrad);
    g.fillRoundedRectangle (fxRect, 6.0f);
    g.setColour (PrimaryPurpleLight);
    g.drawRoundedRectangle (fxRect, 6.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.setFont (uiFont (11.0f, juce::Font::bold));
    g.drawText ("FX", (int) fxRect.getX(), (int) fxRect.getY(),
                (int) fxRect.getWidth(), (int) fxRect.getHeight(),
                juce::Justification::centred, true);

    // --- Setas direcionais na cadeia -------------------------------------------
    // Seta Mix → Filter.
    g.setColour (PrimaryPurpleLight);
    g.fillEllipse (filterPos.getX() - filterRadius - 4.0f, filterPos.getY() - 3.0f, 6.0f, 6.0f);
    // Seta Filter → FX.
    g.fillEllipse (fxPos.getX() - fxRadius - 4.0f, fxPos.getY() - 3.0f, 6.0f, 6.0f);

    // --- Labels dos nós abaixo -------------------------------------------------
    g.setColour (TextSecondary);
    g.setFont (uiFont (FontSizeMicro));
    g.drawText ("Mix", (int) (mixPos.getX() - 20), (int) (mixPos.getY() + mixRadius + 4), 40, 14,
                juce::Justification::centred, true);
    g.drawText ("Filter", (int) (filterPos.getX() - 25), (int) (filterPos.getY() + filterRadius + 4), 50, 14,
                juce::Justification::centred, true);
    g.drawText ("FX", (int) (fxPos.getX() - 10), (int) (fxPos.getY() + fxRadius + 4), 20, 14,
                juce::Justification::centred, true);
}

void RoutingPanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    juce::ColourGradient bgGrad (PanelTop, area.getX(), area.getY(),
                                 PanelBottom, area.getX(), area.getBottom(),
                                 false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (PanelBorder);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
}

void RoutingPanel::resized()
{
    auto area = getLocalBounds().toFloat();
    float sx = getWidth() / 1672.0f;
    float sy = getHeight() / 941.0f;

    area = area.reduced (12.0f * sx, 0);
    const int gap = (int) (8.0f * sx);
    int labelW = (int) (60.0f * sx);

    // Título à esquerda do cabeçalho.
    m_titleLabel.setBounds (area.removeFromLeft ((int) (120.0f * sx)).toNearestInt());
    area.removeFromLeft (gap);

    // Botão Clear no canto superior direito do cabeçalho.
    m_clearButton->setBounds (area.removeFromRight ((int) (60.0f * sx)).toNearestInt());
    area.removeFromRight (gap);

    area.removeFromTop ((int) ((44.0f - 24.0f) * sy)); // restante do cabeçalho

    // Grafo ocupa a maior parte (esquerda e centro).
    m_graphCanvas.setBounds (area.removeFromLeft (area.getWidth() * 0.72f).toNearestInt());
    area.removeFromLeft ((int) (14.0f * sx));

    m_sourceCombo->setBounds (area.removeFromLeft (labelW).toNearestInt());
    m_sourceCombo->setEditableText (false);
    area.removeFromLeft (gap);
    m_destCombo->setBounds (area.removeFromLeft (labelW).toNearestInt());
    area.removeFromLeft (gap);
    m_typeCombo->setBounds (area.removeFromLeft (labelW).toNearestInt());
    area.removeFromLeft (gap);

    m_gainSlider->setBounds (area.removeFromLeft ((int) (90.0f * sx)).toNearestInt());
    area.removeFromLeft (gap);

    m_addButton->setBounds (area.removeFromLeft ((int) (70.0f * sx)).toNearestInt());

    m_statusLabel.setBounds (area.removeFromBottom ((int) (20.0f * sy)).toNearestInt());
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
