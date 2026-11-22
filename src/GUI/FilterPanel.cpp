#include "GUI/FilterPanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

FilterPanel::FilterPanel (juce::AudioProcessorValueTreeState& apvts)
    : PanelComponent ("Filter & FX", false), m_apvts (apvts)
{
    setInterceptsMouseClicks (true, true);

    m_enabledButton = new IconButtonComponent ("⏻", "Filter enable", PrimaryPurple);
    m_enabledButton->setToggleState (false, juce::dontSendNotification);
    m_enabledButton->onClick = [this] { repaint(); };
    addAndMakeVisible (*m_enabledButton);

    m_typeCombo = new juce::ComboBox ("Filter Type");
    m_typeCombo->addItem ("24 dB Low Pass", 1);
    m_typeCombo->addItem ("24 dB High Pass", 2);
    m_typeCombo->addItem ("24 dB Band Pass", 3);
    m_typeCombo->setSelectedId (1);
    addAndMakeVisible (*m_typeCombo);

    m_cutoffKnob = new KnobComponent (apvts, params::FilterCutoff, "Cutoff", "Hz",
                                      KnobComponent::Size::Medium, Osc2Colour);
    addAndMakeVisible (*m_cutoffKnob);

    m_resonanceKnob = new KnobComponent (apvts, params::FilterResonance, "Resonance", "",
                                         KnobComponent::Size::Medium, Osc2Colour);
    addAndMakeVisible (*m_resonanceKnob);

    m_keyTrackKnob = new KnobComponent (apvts, params::FilterKeyTrack, "Key Track", "",
                                        KnobComponent::Size::Small, TextSecondary);
    addAndMakeVisible (*m_keyTrackKnob);

    // Segmented control FX.
    m_fxDriveButton = new juce::TextButton ("Drive");
    m_fxDriveButton->setClickingTogglesState (true);
    m_fxDriveButton->setToggleState (true, juce::dontSendNotification);
    m_fxDriveButton->setTooltip ("Drive effect");
    addAndMakeVisible (*m_fxDriveButton);

    m_fxDelayButton = new juce::TextButton ("Delay");
    m_fxDelayButton->setClickingTogglesState (true);
    m_fxDelayButton->setToggleState (false, juce::dontSendNotification);
    m_fxDelayButton->setTooltip ("Delay effect");
    addAndMakeVisible (*m_fxDelayButton);

    m_fxReverbButton = new juce::TextButton ("Reverb");
    m_fxReverbButton->setClickingTogglesState (true);
    m_fxReverbButton->setToggleState (false, juce::dontSendNotification);
    m_fxReverbButton->setTooltip ("Reverb effect");
    addAndMakeVisible (*m_fxReverbButton);

    // Knobs extras abaixo do gráfico.
    m_fxDriveKnob = new KnobComponent (apvts, params::FilterCutoff, "Drive", "",
                                      KnobComponent::Size::Small, PrimaryPurple);
    addAndMakeVisible (*m_fxDriveKnob);

    m_fxDelayKnob = new KnobComponent (apvts, params::FilterResonance, "Delay", "",
                                      KnobComponent::Size::Small, PrimaryPurpleLight);
    addAndMakeVisible (*m_fxDelayKnob);

    m_enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (m_apvts, params::FilterEnabled, *m_enabledButton);

    m_typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (m_apvts, params::FilterType, *m_typeCombo);
}

void FilterPanel::paint (juce::Graphics& g)
{
    PanelComponent::paint (g);

    // Título do painel.
    g.setColour (TextMain);
    g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
    g.drawText ("Filter & FX", (int) getLocalBounds().getX(),
                (int) getLocalBounds().getY() + 8,
                (int) getLocalBounds().getWidth(), 20,
                juce::Justification::left, true);

    // Gráfico de resposta do filtro.
    auto area = getLocalBounds().toFloat().reduced (10);
    area.removeFromTop (60); // espaço para título + controles superiores

    float cutoff = 0.0f, resonance = 0.0f;
    if (auto* p = m_apvts.getParameter (params::FilterCutoff))
        cutoff = *m_apvts.getRawParameterValue (params::FilterCutoff);
    if (auto* p = m_apvts.getParameter (params::FilterResonance))
        resonance = *m_apvts.getRawParameterValue (params::FilterResonance);

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
    g.drawText ("Response from cutoff/resonance parameters.",
                (int) plot.getX(), (int) plot.getBottom() + 2,
                (int) plot.getWidth(), 14, juce::Justification::centred, true);
}

void FilterPanel::resized()
{
    auto area = getLocalBounds().toFloat();
    float sx = getWidth() / 1672.0f;
    float sy = getHeight() / 941.0f;

    area = area.reduced (10.0f * sx, 0);
    const int gap = (int) (10.0f * sx);

    area.removeFromTop ((int) (44.0f * sy)); // espaço do título do PanelComponent

    int btnW = (int) (32.0f * sx);
    m_enabledButton->setBounds (area.removeFromLeft (btnW).toNearestInt());
    area.removeFromLeft (gap);
    int comboW = (int) (120.0f * sx);
    m_typeCombo->setBounds (area.removeFromLeft (comboW).toNearestInt());
    area.removeFromLeft (gap);

    int knobW = (int) (110.0f * sx);
    m_cutoffKnob->setBounds (area.removeFromLeft (knobW).toNearestInt());
    area.removeFromLeft (gap);
    m_resonanceKnob->setBounds (area.removeFromLeft (knobW).toNearestInt());
    area.removeFromLeft (gap);
    m_keyTrackKnob->setBounds (area.removeFromLeft ((int) (56.0f * sx)).toNearestInt());

    area.removeFromTop (gap);
    int fxBtnW = (int) (70.0f * sx);
    m_fxDriveButton->setBounds (area.removeFromLeft (fxBtnW).toNearestInt());
    area.removeFromLeft ((int) (gap * 0.5f));
    m_fxDelayButton->setBounds (area.removeFromLeft (fxBtnW).toNearestInt());
    area.removeFromLeft ((int) (gap * 0.5f));
    m_fxReverbButton->setBounds (area.removeFromLeft (fxBtnW).toNearestInt());

    area.removeFromTop (gap);
    m_fxDriveKnob->setBounds (area.removeFromLeft (knobW).toNearestInt());
    area.removeFromLeft (gap);
    m_fxDelayKnob->setBounds (area.removeFromLeft (knobW).toNearestInt());
}

} // namespace gui
} // namespace omnivy
