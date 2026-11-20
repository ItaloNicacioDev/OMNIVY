#include "GUI/PanelComponent.h"

namespace omnivy {
namespace gui {

PanelComponent::PanelComponent (const juce::String& title, bool highlighted)
    : m_highlighted (highlighted)
{
    m_titleLabel.setText (title, juce::dontSendNotification);
    m_titleLabel.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
    m_titleLabel.setColour (juce::Label::textColourId, TextMain);
    m_titleLabel.setJustificationType (juce::Justification::left);
    m_titleLabel.setEditable (false);
    m_titleLabel.setEnabled (false); // não captura cliques
    if (title.isNotEmpty())
    {
        m_titleEmpty = false;
        addAndMakeVisible (m_titleLabel);
    }
}

void PanelComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    OmnivyLookAndFeel laf;
    laf.drawPanelBackground (g, area, m_highlighted);

    if (m_titleEmpty)
        return;

    // Título desenhado acima da borda interna do painel.
    g.setColour (TextMain);
    g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
    g.drawText (m_titleLabel.getText(), 12, 8, getWidth() - 24, 22,
                juce::Justification::left, true);

    // Linha de separação sutil abaixo do título.
    g.setColour (Border);
    g.fillRect (12, 32, getWidth() - 24, 1);
}

void PanelComponent::setHighlighted (bool highlighted)
{
    if (m_highlighted != highlighted)
    {
        m_highlighted = highlighted;
        repaint();
    }
}

// -----------------------------------------------------------------------------
// IconButtonComponent
// -----------------------------------------------------------------------------
IconButtonComponent::IconButtonComponent (const juce::String& icon,
                                          const juce::String& tooltip,
                                          juce::Colour iconColour)
    : juce::Button (icon), m_icon (icon), m_iconColour (iconColour)
{
    setTooltip (tooltip);
    setClickingTogglesState (true);
    setToggleState (false, juce::dontSendNotification);
    setWantsKeyboardFocus (false);
}

void IconButtonComponent::setToggleState (bool shouldBeOn, juce::NotificationType notification)
{
    if (m_toggleState != shouldBeOn)
    {
        m_toggleState = shouldBeOn;
        repaint();
        if (notification != juce::dontSendNotification)
            listeners.call ([this] (Listener& l) { l.buttonStateChanged (this); });
    }
}

void IconButtonComponent::setIconColour (juce::Colour colour)
{
    m_iconColour = colour;
    repaint();
}

void IconButtonComponent::paintButton (juce::Graphics& g,
                                       bool isMouseOverButton, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat();
    OmnivyLookAndFeel laf;
    laf.drawIconButtonBackground (g, area, isMouseOverButton, m_toggleState, isEnabled());

    // Ícone centralizado.
    g.setColour (isEnabled() ? m_iconColour : m_iconColour.withAlpha (0.3f));
    g.setFont (uiFont (18.0f));
    g.drawText (m_icon, (int) area.getX(), (int) area.getY(), (int) area.getWidth(),
                (int) area.getHeight(), juce::Justification::centred, true);
}

} // namespace gui
} // namespace omnivy
