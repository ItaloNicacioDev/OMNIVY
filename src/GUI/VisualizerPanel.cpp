#include "GUI/VisualizerPanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

VisualizerPanel::VisualizerPanel (juce::AudioProcessorValueTreeState& apvts)
    : m_apvts (apvts)
{
    setInterceptsMouseClicks (true);

    m_waveformPreview = new WaveformComponent();
    m_waveformPreview->setColour (Osc1Colour);
    m_waveformPreview->setSizeHint (260, 140);
    addAndMakeVisible (*m_waveformPreview);

    m_envelopeDisplay = new EnvelopeDisplay();
    m_envelopeDisplay->setColour (Env1Colour);
    addAndMakeVisible (*m_envelopeDisplay);

    m_scopeNote.setText ("Escopo e espectro de áudio em tempo real dependem de uma "
                         "captura de buffer de saída ainda não implementada no DSP.",
                         juce::dontSendNotification);
    m_scopeNote.setFont (uiFont (FontSizeMicro));
    m_scopeNote.setColour (juce::Label::textColourId, TextMuted);
    m_scopeNote.setJustificationType (juce::Justification::centred);
    m_scopeNote.setEnabled (false);
    addAndMakeVisible (m_scopeNote);

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

void VisualizerPanel::parameterValueChanged (const juce::String& /*parameterID*/, float /*newValue*/)
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
    if (auto* p = m_apvts.getParameter (params::oscWaveform (0)))
        m_waveformPreview->setWaveform (static_cast<dsp::Waveform> (
            static_cast<int> (*p)));

    m_envelopeDisplay->setParameters (
        *m_apvts.getRawParameterValue (params::EnvAttack),
        *m_apvts.getRawParameterValue (params::EnvDecay),
        *m_apvts.getRawParameterValue (params::EnvSustain),
        *m_apvts.getRawParameterValue (params::EnvRelease));
}

void VisualizerPanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
}

void VisualizerPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (44);

    int half = area.getWidth() / 2;
    m_waveformPreview->setBounds (area.removeFromLeft (half));
    area.removeFromLeft (16);
    m_envelopeDisplay->setBounds (area.removeFromLeft (half));
    area.removeFromTop (16);

    m_scopeNote.setBounds (area.removeFromBottom (30));
}

} // namespace gui
} // namespace omnivy
