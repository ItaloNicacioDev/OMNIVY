#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

OmnivyLookAndFeel::OmnivyLookAndFeel()
{
}

void OmnivyLookAndFeel::drawPanelBackground (juce::Graphics& g,
                                             const juce::Rectangle<float>& area,
                                             bool highlighted)
{
    juce::ignoreUnused (highlighted);

    // Corpo do painel com gradiente muito sutil para dar profundidade.
    juce::ColourGradient grad (PanelDark.withAlpha (0.95f), area.getX(), area.getY(),
                               PanelLight.withAlpha (0.85f), area.getRight(), area.getBottom(),
                               false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (area, (float) PanelCornerRadius);

    // Borda.
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) PanelCornerRadius, (float) BorderWidth);
}

void OmnivyLookAndFeel::drawIconButtonBackground (juce::Graphics& g,
                                                  const juce::Rectangle<float>& area,
                                                  bool isMouseOver, bool isToggleOn,
                                                  bool isEnabled)
{
    auto base = isEnabled ? (isToggleOn ? PrimaryPurple : ControlSurface)
                          : ControlSurface.withAlpha (0.5f);

    if (isMouseOver && isEnabled)
        base = base.brighter (0.15f);

    g.setColour (base);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);

    if (isToggleOn && isEnabled)
    {
        // Anel de glow ao redor do botão ativado.
        g.setColour (GlowActive);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.5f);
    }
    else
    {
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, (float) BorderWidth);
    }
}

juce::Font OmnivyLookAndFeel::getTabButtonFont (juce::TabBarButton&, float /*height*/)
{
    return uiFont (FontSizeLabel, juce::Font::bold);
}

void OmnivyLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                       bool isMouseOver, bool isMouseDown)
{
    auto area = button.getBounds().toFloat();
    juce::ignoreUnused (isMouseDown);

    // Todas as abas usam a mesma aparência escura; destaque sutil no hover.
    if (isMouseOver)
    {
        g.setColour (PrimaryPurple.withAlpha (0.2f));
        g.fillRoundedRectangle (area, 6.0f);
    }
    else
    {
        g.setColour (PanelDark);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (Border);
        g.drawRoundedRectangle (area, 6.0f, (float) BorderWidth);
    }
    g.setColour (isMouseOver ? TextMain : TextSecondary);

    g.setFont (getTabButtonFont (button, area.getHeight()));
    g.drawText (button.getName(), (int) area.getX(), (int) area.getY(),
                (int) area.getWidth(), (int) area.getHeight(),
                juce::Justification::centred, true);
}

} // namespace gui
} // namespace omnivy
