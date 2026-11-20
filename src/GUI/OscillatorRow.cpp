#include "GUI/OscillatorRow.h"

#include "DSP/Utilities/DSPUtilities.h"

namespace omnivy {
namespace gui {

OscillatorRowComponent::OscillatorRowComponent (juce::AudioProcessorValueTreeState& apvts,
                                                int oscillatorIndex,
                                                bool selectable)
    : m_apvts (apvts), m_oscillatorIndex (oscillatorIndex),
      m_stripeColour (oscillatorColour (oscillatorIndex))
{
    setInterceptsMouseClicks (true, true);

    // Número do oscilador (esquerda).
    m_numberLabel.setText (juce::String (oscillatorIndex + 1), juce::dontSendNotification);
    m_numberLabel.setFont (uiFont (15.0f, juce::Font::bold));
    m_numberLabel.setColour (juce::Label::textColourId, m_stripeColour.brighter (0.3f));
    m_numberLabel.setJustificationType (juce::Justification::centred);
    m_numberLabel.setEnabled (false);
    addAndMakeVisible (m_numberLabel);

    // Botão Power — parâmetro real oscActive.
    m_powerButton = new IconButtonComponent ("⏻", "OSC " + juce::String (oscillatorIndex + 1) + " ligar/desligar",
                                             m_stripeColour);
    m_powerButton->setToggleState (false, juce::dontSendNotification);
    m_powerButton->onClick = [this] { repaint(); };
    addAndMakeVisible (*m_powerButton);

    // Tipo de engine — parâmetro real oscEngine.
    m_engineCombo = new juce::ComboBox ("OSC " + juce::String (oscillatorIndex + 1) + " Engine");
    m_engineCombo->addListener (this);
    populateEngineCombo();
    addAndMakeVisible (*m_engineCombo);

    // Tipo de onda — parâmetro real oscWaveform.
    m_waveformCombo = new juce::ComboBox ("OSC " + juce::String (oscillatorIndex + 1) + " Waveform");
    m_waveformCombo->addListener (this);
    populateWaveformCombo();
    addAndMakeVisible (*m_waveformCombo);

    // Pré-visualização da forma de onda.
    m_waveformPreview = new WaveformComponent();
    m_waveformPreview->setColour (m_stripeColour);
    m_waveformPreview->setSizeHint (64, 40);
    addAndMakeVisible (*m_waveformPreview);

    // Level — parâmetro real oscLevel.
    m_levelKnob = new KnobComponent (apvts, params::oscLevel (oscillatorIndex),
                                     "Level", "", KnobComponent::Size::Small, m_stripeColour);
    addAndMakeVisible (*m_levelKnob);

    // Octave — parâmetro real oscOctave.
    m_octaveKnob = new KnobComponent (apvts, params::oscOctave (oscillatorIndex),
                                      "Oct", "", KnobComponent::Size::Small, TextSecondary);
    addAndMakeVisible (*m_octaveKnob);

    // Semitone — parâmetro real oscSemitone.
    m_semitoneKnob = new KnobComponent (apvts, params::oscSemitone (oscillatorIndex),
                                        "Semi", "", KnobComponent::Size::Small, TextSecondary);
    addAndMakeVisible (*m_semitoneKnob);

    // Chevron de seleção da linha (estado apenas da UI).
    m_chevronButton = new juce::TextButton ("›");
    m_chevronButton->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    m_chevronButton->setColour (juce::TextButton::textColourOffId, TextMuted);
    m_chevronButton->setColour (juce::TextButton::textColourOnId, PrimaryPurple);
    m_chevronButton->onClick = [this] { setSelected (!m_selected); };
    addAndMakeVisible (*m_chevronButton);

    // Attachments aos parâmetros reais do APVTS (não-knobs; os knobs criam
    // seu próprio SliderAttachment internamente no construtor).
    m_powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (m_apvts, params::oscActive (oscillatorIndex), *m_powerButton);

    m_engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (m_apvts, params::oscEngine (oscillatorIndex), *m_engineCombo);

    m_waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (m_apvts, params::oscWaveform (oscillatorIndex), *m_waveformCombo);
}

} // namespace gui
} // namespace omnivy
