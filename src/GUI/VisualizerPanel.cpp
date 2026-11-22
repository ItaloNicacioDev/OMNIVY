#include "GUI/VisualizerPanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

VisualizerPanel::VisualizerPanel (juce::AudioProcessorValueTreeState& apvts)
    : m_apvts (apvts)
{
    setInterceptsMouseClicks (true, true);

    m_visualizerTabs = new juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop);
    m_visualizerTabs->setTabBarDepth (36);

    m_spectrumView = new SpectrumView();
    m_scopeView = new ScopeView();
    m_view3D = new View3D();

    m_visualizerTabs->addTab ("Spectrum", PanelDark, m_spectrumView, true);
    m_visualizerTabs->addTab ("Scope", PanelDark, m_scopeView, false);
    m_visualizerTabs->addTab ("3D", PanelDark, m_view3D, false);

    addAndMakeVisible (*m_visualizerTabs);

    m_apvts.addParameterListener (params::oscWaveform (0), this);
    m_apvts.addParameterListener (params::EnvAttack, this);
    m_apvts.addParameterListener (params::EnvDecay, this);
    m_apvts.addParameterListener (params::EnvSustain, this);
    m_apvts.addParameterListener (params::EnvRelease, this);

    refresh();
}

VisualizerPanel::~VisualizerPanel()
{
    m_apvts.removeParameterListener (params::oscWaveform (0), this);
    m_apvts.removeParameterListener (params::EnvAttack, this);
    m_apvts.removeParameterListener (params::EnvDecay, this);
    m_apvts.removeParameterListener (params::EnvSustain, this);
    m_apvts.removeParameterListener (params::EnvRelease, this);
}

void VisualizerPanel::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    // refresh() lê parâmetros do APVTS e atualiza componentes de UI. Garante
    // execução na thread de mensagem para evitar data race com paint() quando
    // a automação do host notifica na thread de áudio.
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        refresh();
    else
        juce::MessageManager::getInstance()->callAsync ([this] { refresh(); });
}

void VisualizerPanel::refresh()
{
    // Atualiza visualizações baseadas em parâmetros reais do APVTS.
    m_spectrumView->repaint();
    m_scopeView->repaint();
    m_view3D->repaint();
}

void VisualizerPanel::paint (juce::Graphics& g)
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

void VisualizerPanel::resized()
{
    auto area = getLocalBounds().toFloat();
    float sx = getWidth() / 1672.0f;
    float sy = getHeight() / 941.0f;

    int tabBarDepth = (int) (36.0f * sy);
    m_visualizerTabs->setTabBarDepth (tabBarDepth);
    m_visualizerTabs->setBounds (area.toNearestInt());
}

} // namespace gui
} // namespace omnivy
