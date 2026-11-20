#include "GUI/WavetablePanel.h"

namespace omnivy {
namespace gui {

WavetablePanel::WavetablePanel (juce::AudioProcessorValueTreeState& /*apvts*/)
{
    setInterceptsMouseClicks (true);

    m_wavetableSelector = new juce::ComboBox ("Wavetable");
    m_wavetableSelector->addItem ("Sine (built-in)", 1);
    m_wavetableSelector->setSelectedId (1);
    m_wavetableSelector->setEnabled (false); // apenas uma tabela existe no DSP
    m_wavetableSelector->setTooltip ("A wavetable é gerada pelo DSP (seno). "
                                     "Importar/editar ainda não implementado.");
    addAndMakeVisible (*m_wavetableSelector);

    m_waveformPreview = new WaveformComponent();
    m_waveformPreview->setWaveform (dsp::Waveform::Sine);
    m_waveformPreview->setColour (PrimaryPurple);
    m_waveformPreview->setSizeHint (200, 120);
    addAndMakeVisible (*m_waveformPreview);

    m_infoLabel.setText ("A wavetable compartilhada é uma tabela de seno (2048 amostras)\n"
                         "gerada em AudioEngine::prepare().\n"
                         "Importar, atualizar, copiar e excluir dependem de implementação no DSP.",
                         juce::dontSendNotification);
    m_infoLabel.setFont (uiFont (FontSizeMicro));
    m_infoLabel.setColour (juce::Label::textColourId, TextMuted);
    m_infoLabel.setJustificationType (juce::Justification::centred);
    m_infoLabel.setEnabled (false);
    addAndMakeVisible (m_infoLabel);
}

void WavetablePanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
}

void WavetablePanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (44); // título do pai (PanelComponent não é ancestral aqui;
                             // desenhamos fundo em paint, então deixamos espaço)

    int selectorW = 200;
    m_wavetableSelector->setBounds (area.removeFromLeft (selectorW));
    area.removeFromLeft (20);

    m_waveformPreview->setBounds (area.removeFromLeft (220));
    area.removeFromLeft (20);

    m_infoLabel.setBounds (area.removeFromLeft (juce::jlimit (100, area.getWidth(), 300)));
}

} // namespace gui
} // namespace omnivy
