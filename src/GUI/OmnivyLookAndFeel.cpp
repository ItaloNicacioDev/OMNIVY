#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

OmnivyLookAndFeel::OmnivyLookAndFeel()
{
    setDefaultSansSerifTypeface (uiFont (FontSizeLabel).getTypeface());
}

void OmnivyLookAndFeel::drawPanelBackground (juce::Graphics& g,
                                             const juce::Rectangle<float>& area,
                                             bool highlighted)
{
    // Sombra externa sutil (glow de borda) quando destacado.
    if (highlighted)
    {
        juce::DropShadow (Colours::black.withAlpha (0.6f), 8, { 0, 4 })
            .drawForRectangle (g, area);
    }

    // Corpo do painel com gradiente muito sutil para dar profundidade.
    juce::ColourGradient grad (PanelDark.withAlpha (0.95f), area.getTopLeft(),
                               PanelLight.withAlpha (0.85f), area.getBottomRight());
    grad.isRadial = false;
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

void OmnivyLookAndFeel::drawLabel (juce::Graphics& g, int x, int y, int width, int height,
                                   const juce::String& text, const juce::Font& font,
                                   juce::Justification justification, juce::Colour colour)
{
    g.setColour (colour);
    g.setFont (font);
    g.drawText (text, x, y, width, height, justification, false);
}

juce::Font OmnivyLookAndFeel::getTabButtonFont (juce::TabBarButton&, int /*tabDepth*/)
{
    return uiFont (FontSizeLabel, juce::Font::bold);
}

int OmnivyLookAndFeel::getTabButtonBestWidth (juce::TabBarButton& button, int /*tabDepth*/)
{
    auto textWidth = button.getFont().getStringWidth (button.getName()) + 24;
    return jlimit (80, 200, (int) textWidth);
}

void OmnivyLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                       bool isMouseOver, bool isTabFront)
{
    auto area = button.getBounds().toFloat();

    if (isTabFront)
    {
        // Aba ativa: gradiente roxo, texto branco, glow discreto.
        juce::ColourGradient grad (PrimaryPurpleDark, area.getTopLeft(),
                                   PrimaryPurple, area.getBottomRight());
        g.setGradientFill (grad);
        g.fillRoundedRectangle (area, 6.0f);

        g.setColour (GlowActive.withAlpha (0.35f));
        g.drawRoundedRectangle (area, 6.0f, 1.5f);

        g.setColour (TextMain);
    }
    else
    {
        // Aba inativa: painel escuro, texto secundário.
        g.setColour (PanelDark);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (Border);
        g.drawRoundedRectangle (area, 6.0f, (float) BorderWidth);
        g.setColour (isMouseOver ? TextSecondary : TextMuted);
    }

    g.setFont (getTabButtonFont (button, 0));
    g.drawText (button.getName(), (int) area.getX(), (int) area.getY(),
                (int) area.getWidth(), (int) area.getHeight(),
                juce::Justification::centred, true);
}

} // namespace gui
} // namespace omnivy
