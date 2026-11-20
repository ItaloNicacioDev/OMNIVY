#include "GUI/FilterPanel.h"

namespace omnivy {
namespace gui {

FilterPanel::FilterPanel (juce::AudioProcessorValueTreeState& apvts)
    : PanelComponent ("Filtro & FX", false), m_apvts (apvts)
{
    setInterceptsMouseClicks (true);

    m_enabledButton = new IconButtonComponent ("⏻", "Filtro ligar/desligar", PrimaryPurple);
    m_enabledButton->setToggleState (false, juce::dontSendNotification);
    m_enabledButton->onClick = [this] { repaint(); };
    addAndMakeVisible (m_enabledButton);

    m_typeCombo = new juce::ComboBox ("Tipo de filtro");
    m_typeCombo->addItem ("Low Pass", 1);
    m_typeCombo->addItem ("High Pass", 2);
    m_typeCombo->addItem ("Band Pass", 3);
    addAndMakeVisible (m_typeCombo);

    m_cutoffKnob = new KnobComponent (apvts, params::FilterCutoff, "Cutoff", "Hz",
                                      KnobComponent::Size::Medium, Osc2Colour);
    addAndMakeVisible (m_cutoffKnob);

    m_resonanceKnob = new KnobComponent (apvts, params::FilterResonance, "Resonance", "",
                                         KnobComponent::Size::Medium, Osc2Colour);
    addAndMakeVisible (m_resonanceKnob);

    m_keyTrackKnob = new KnobComponent (apvts, params::FilterKeyTrack, "Key Track", "",
                                        KnobComponent::Size::Small, TextSecondary);
    addAndMakeVisible (m_keyTrackKnob);

    m_enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (m_apvts, params::FilterEnabled, *m_enabledButton);

    m_typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (m_apvts, params::FilterType, *m_typeCombo);
}

void FilterPanel::paint (juce::Graphics& g)
{
    PanelComponent::paint (g);

    // Esboço da resposta do filtro baseado nos parâmetros reais (corte e
    // ressonância). Não é uma medição de áudio em tempo real, mas uma
    // representação dos valores configurados.
    auto area = getLocalBounds().toFloat().reduced (10);
    area.removeFromTop (46);

    float cutoff = 0.0f, resonance = 0.0f;
    if (auto* p = m_apvts.getParameter (params::FilterCutoff))
        cutoff = static_cast<float> (*p);
    if (auto* p = m_apvts.getParameter (params::FilterResonance))
        resonance = static_cast<float> (*p);

    juce::Rectangle<float> plot (area.getX(), area.getY(), area.getWidth(), area.getHeight() - 18);
    g.setColour (Border.brighter (0.2f));
    g.drawRoundedRectangle (plot, 4.0f, 1.0f);

    // Desenha uma curva simples de resposta (low-pass esboçado) quando o
    // filtro está ligado.
    if (m_enabledButton->getToggleState())
    {
        juce::Path curve;
        float cx = plot.getCentreX();
        float cy = plot.getBottom();
        // Normaliza corte (20..20000 Hz) para posição horizontal aproximada.
        float cutoffPos = juce::jmap (juce::jlimit (20.0f, 20000.0f, cutoff), 20.0f, 20000.0f,
                                      0.0f, 1.0f);
        float x = plot.getX() + cutoffPos * plot.getWidth();
        float depth = juce::jlimit (0.0f, 1.0f, resonance) * plot.getHeight() * 0.6f;

        curve.startNewSubPath (plot.getX(), cy);
        curve.lineTo (x - plot.getWidth() * 0.25f, cy);
        curve.cubicTo (x - plot.getWidth() * 0.1f, cy,
                       x - plot.getWidth() * 0.1f, cy - depth,
                       x, cy - depth);
        curve.cubicTo (x + plot.getWidth() * 0.1f, cy - depth,
                       x + plot.getWidth() * 0.1f, cy,
                       plot.getRight(), cy);
        g.setColour (Osc2Colour);
        g.strokePath (curve, juce::PathStrokeType (2.0f));
    }

    g.setColour (TextMuted);
    g.setFont (uiFont (FontSizeMicro));
    g.drawText ("Resposta esboçada a partir dos parâmetros de corte/ressonância.",
                (int) plot.getX(), (int) plot.getBottom() + 2,
                (int) plot.getWidth(), 14, juce::Justification::centred, true);
}

void FilterPanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (44);

    m_enabledButton->setBounds (area.removeFromLeft (32));
    area.removeFromLeft (8);
    m_typeCombo->setBounds (area.removeFromLeft (110));
    area.removeFromLeft (16);

    int knobW = 110;
    m_cutoffKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (12);
    m_resonanceKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (12);
    m_keyTrackKnob->setBounds (area.removeFromLeft (56));
}

} // namespace gui
} // namespace omnivy
