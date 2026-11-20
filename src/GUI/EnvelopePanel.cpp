#include "GUI/EnvelopePanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

EnvelopePanel::EnvelopePanel (juce::AudioProcessorValueTreeState& apvts)
    : PanelComponent ("Envelope", false), m_apvts (apvts)
{
    setInterceptsMouseClicks (true, true);

    m_attackKnob = new KnobComponent (apvts, params::EnvAttack, "Attack", "s",
                                      KnobComponent::Size::Medium, Env1Colour);
    addAndMakeVisible (*m_attackKnob);

    m_decayKnob = new KnobComponent (apvts, params::EnvDecay, "Decay", "s",
                                     KnobComponent::Size::Medium, Env1Colour);
    addAndMakeVisible (*m_decayKnob);

    m_sustainKnob = new KnobComponent (apvts, params::EnvSustain, "Sustain", "",
                                       KnobComponent::Size::Medium, Env1Colour);
    addAndMakeVisible (*m_sustainKnob);

    m_releaseKnob = new KnobComponent (apvts, params::EnvRelease, "Release", "s",
                                       KnobComponent::Size::Medium, Env1Colour);
    addAndMakeVisible (*m_releaseKnob);

    m_envelopeDisplay = new EnvelopeDisplay();
    m_envelopeDisplay->setColour (Env1Colour);
    addAndMakeVisible (*m_envelopeDisplay);

    // Escuta o APVTS para manter o gráfico sincronizado com mudanças de MIDI,
    // automação ou presets.
    m_apvts.addParameterListener (params::EnvAttack, this);
    m_apvts.addParameterListener (params::EnvDecay, this);
    m_apvts.addParameterListener (params::EnvSustain, this);
    m_apvts.addParameterListener (params::EnvRelease, this);

    refreshDisplay();
}

EnvelopePanel::~EnvelopePanel()
{
    m_apvts.removeParameterListener (params::EnvAttack, this);
    m_apvts.removeParameterListener (params::EnvDecay, this);
    m_apvts.removeParameterListener (params::EnvSustain, this);
    m_apvts.removeParameterListener (params::EnvRelease, this);
}

void EnvelopePanel::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    // refreshDisplay() lê valores atômicos do APVTS e chama repaint(). repaint()
    // é thread-safe, mas a leitura/gravação dos membros de UI deve ocorrer na
    // thread de mensagem. Se a automação do host notificar na thread de áudio,
    // adiamos a atualização para evitar data race com paint().
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        refreshDisplay();
    else
        juce::MessageManager::getInstance()->callAsync ([this] { refreshDisplay(); });
}

void EnvelopePanel::refreshDisplay()
{
    if (m_envelopeDisplay == nullptr)
        return;
    m_envelopeDisplay->setParameters (
        *m_apvts.getRawParameterValue (params::EnvAttack),
        *m_apvts.getRawParameterValue (params::EnvDecay),
        *m_apvts.getRawParameterValue (params::EnvSustain),
        *m_apvts.getRawParameterValue (params::EnvRelease));
}

void EnvelopePanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (44);

    int knobW = 92;
    int gap = 12;
    m_attackKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_decayKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_sustainKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_releaseKnob->setBounds (area.removeFromLeft (knobW));

    m_envelopeDisplay->setBounds (area);
}

} // namespace gui
} // namespace omnivy
