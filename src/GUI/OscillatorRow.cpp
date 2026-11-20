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

OscillatorRowComponent::~OscillatorRowComponent()
{
}

void OscillatorRowComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    // Faixa lateral colorida.\n    g.setColour (m_stripeColour);
    g.fillRoundedRectangle (area.removeFromLeft (6.0f), (float) ControlCornerRadius);

    if (m_selected)
    {
        g.setColour (PrimaryPurple.withAlpha (0.3f));
        g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    }
}

void OscillatorRowComponent::resized()
{
    auto area = getLocalBounds().reduced (4);
    area.removeFromLeft (6); // faixa lateral

    int labelW = 24;
    int btnW = 28;
    int comboW = 110;
    int previewW = 72;
    int knobW = 46;
    int gap = 6;
    int chevronW = 24;

    m_numberLabel.setBounds (area.removeFromLeft (labelW));
    area.removeFromLeft (gap);

    m_powerButton->setBounds (area.removeFromLeft (btnW));
    area.removeFromLeft (gap);

    m_engineCombo->setBounds (area.removeFromLeft (comboW));
    area.removeFromLeft (gap);

    m_waveformCombo->setBounds (area.removeFromLeft (comboW));
    area.removeFromLeft (gap);

    m_waveformPreview->setBounds (area.removeFromLeft (previewW));
    area.removeFromLeft (gap);

    m_levelKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_octaveKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_semitoneKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);

    m_chevronButton->setBounds (area.removeFromLeft (chevronW));
}

void OscillatorRowComponent::setSelected (bool selected)
{
    m_selected = selected;
    updateSelectionVisuals();
    repaint();
}

void OscillatorRowComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == m_waveformCombo)
        m_waveformPreview->repaint();
}

void OscillatorRowComponent::populateEngineCombo()
{
    m_engineCombo->addItem ("VA", 1);
    m_engineCombo->addItem ("Additive", 2);
    m_engineCombo->addItem ("FM", 3);
    m_engineCombo->addItem ("Granular", 4);
    m_engineCombo->addItem ("Noise", 5);
    m_engineCombo->setSelectedId (1);
}

void OscillatorRowComponent::populateWaveformCombo()
{
    m_waveformCombo->addItem ("Sine", 1);
    m_waveformCombo->addItem ("Triangle", 2);
    m_waveformCombo->addItem ("Saw", 3);
    m_waveformCombo->addItem ("Square", 4);
    m_waveformCombo->addItem ("Noise", 5);
    m_waveformCombo->setSelectedId (1);
}

void OscillatorRowComponent::updateSelectionVisuals()
{
    // Estado visual apenas da UI; não afeta o DSP.
}

} // namespace gui
} // namespace omnivy
