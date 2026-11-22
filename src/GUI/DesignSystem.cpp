#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// OmnivyLookAndFeel
// -----------------------------------------------------------------------------
OmnivyLookAndFeel::OmnivyLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, WindowBgTop);
}

// -----------------------------------------------------------------------------
// drawPanelBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawPanelBackground (juce::Graphics& g,
                                             const juce::Rectangle<float>& area,
                                             bool /*highlighted*/)
{
    auto r = area.toNearestInt();

    // Fundo gradiente vertical sutil: PanelTop → PanelBottom
    juce::ColourGradient bgGrad (PanelTop, r.getX(), r.getY(),
                                 PanelBottom, r.getX(), r.getBottom(),
                                 false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (r.toFloat(), (float) PanelCornerRadius);

    // Borda 1px
    g.setColour (PanelBorder);
    g.drawRoundedRectangle (r.toFloat(), (float) PanelCornerRadius, (float) BorderWidth);

    // Sombra interna roxa muito sutil
    g.setColour (GlowActive.withAlpha (0.12f));
    g.drawRoundedRectangle (r.reduced (1).toFloat(), (float) PanelCornerRadius - 1.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// drawIconButtonBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawIconButtonBackground (juce::Graphics& g,
                                                  const juce::Rectangle<float>& area,
                                                  bool isMouseOver, bool isToggleOn, bool isEnabled)
{
    auto r = area.toNearestInt();

    if (!isEnabled)
    {
        g.setColour (PanelDark);
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (TextDisabled.withAlpha (0.5f));
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, (float) BorderWidth);
        return;
    }

    if (isToggleOn || isMouseOver)
    {
        juce::ColourGradient grad (PrimaryPurpleDark, r.getX(), r.getY(),
                                   PrimaryPurple,     r.getX(), r.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (GlowActive);
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, 1.5f);
    }
    else
    {
        g.setColour (FieldBg);
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (FieldBorder);
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, (float) BorderWidth);

        if (isMouseOver)
        {
            g.setColour (PrimaryPurple.withAlpha (0.3f));
            g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, 1.0f);
        }
    }
}

// -----------------------------------------------------------------------------
// drawTextButtonBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawTextButtonBackground (juce::Graphics& g,
                                                  const juce::Rectangle<float>& area,
                                                  bool isMouseOver, bool isToggleOn, bool isEnabled)
{
    auto r = area.toNearestInt();

    if (!isEnabled)
    {
        g.setColour (PanelDark);
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (TextDisabled.withAlpha (0.5f));
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, (float) BorderWidth);
        return;
    }

    if (isToggleOn)
    {
        juce::ColourGradient grad (PrimaryPurpleDark, r.getX(), r.getY(),
                                   PrimaryPurple,     r.getX(), r.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (GlowActive);
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, 1.5f);
    }
    else if (isMouseOver)
    {
        g.setColour (juce::Colour (0x1E1B4Bu)); // fundo hover
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (PrimaryPurpleLight);
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0x161634u)); // fundo padrão
        g.fillRoundedRectangle (r.toFloat(), (float) ControlCornerRadius);
        g.setColour (FieldBorder);
        g.drawRoundedRectangle (r.toFloat(), (float) ControlCornerRadius, (float) BorderWidth);
    }
}

// -----------------------------------------------------------------------------
// drawTabButton (abas estilo paralelogramo/chanfrado)
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                       bool isMouseOver, bool isMouseDown)
{
    auto r = button.getLocalBounds().toFloat();
    bool isActive = button.getToggleState();

    // Forma de paralelogramo (cantos inclinados)
    float shear = 6.0f;
    juce::Path shape;
    shape.startNewSubPath (r.getX() + shear * 0.5f, r.getY());
    shape.lineTo (r.getRight() + shear * 0.5f, r.getY());
    shape.lineTo (r.getRight() - shear * 0.5f, r.getBottom());
    shape.lineTo (r.getX() - shear * 0.5f, r.getBottom());
    shape.closeSubPath();

    if (isActive)
    {
        juce::ColourGradient grad (PrimaryPurpleDark, r.getX(), r.getY(),
                                   PrimaryPurple,     r.getX(), r.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillPath (shape);
        g.setColour (juce::Colours::white);
    }
    else if (isMouseOver)
    {
        g.setColour (PrimaryPurple.withAlpha (0.18f));
        g.fillPath (shape);
        g.setColour (TextSecondary);
    }
    else
    {
        g.setColour (PanelDark);
        g.fillPath (shape);
        g.setColour (FieldBorder);
        g.strokePath (shape, juce::PathStrokeType ((float) BorderWidth));
        g.setColour (TextSecondary);
    }

    g.setFont (getTabButtonFont (button, r.getHeight()));
    auto text = button.getButtonText();
    g.drawFittedText (text, r.toNearestInt(), juce::Justification::centred, 1);
}

// -----------------------------------------------------------------------------
// getTabButtonFont
// -----------------------------------------------------------------------------
juce::Font OmnivyLookAndFeel::getTabButtonFont (juce::TabBarButton& button, float /*height*/)
{
    juce::ignoreUnused (button);
    return uiFont (FontSizeLabel, juce::Font::bold);
}

} // namespace gui
} // namespace omnivy
