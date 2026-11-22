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

// -----------------------------------------------------------------------------
// drawLinearSliderBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawLinearSliderBackground (juce::Graphics& g, int x, int y, int width, int height,
                                                    float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
                                                    const juce::Slider::SliderStyle, juce::Slider& /*slider*/)
{
    auto r = juce::Rectangle<int> (x, y, width, height).toFloat();
    float trackY = r.getCentreY();
    float trackH = 4.0f;

    g.setColour (FieldBorder);
    g.fillRoundedRectangle (r.getX(), trackY - trackH * 0.5f, r.getWidth(), trackH, trackH * 0.5f);
}

// -----------------------------------------------------------------------------
// drawLinearSliderThumb
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height,
                                               float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
                                               const juce::Slider::SliderStyle, juce::Slider& /*slider*/)
{
    auto r = juce::Rectangle<int> (x, y, width, height).toFloat();
    float thumbSize = 14.0f;

    g.setColour (PrimaryPurple);
    g.fillEllipse (r.getCentreX() - thumbSize * 0.5f, r.getCentreY() - thumbSize * 0.5f,
                   thumbSize, thumbSize);
    g.setColour (juce::Colours::white);
    g.drawEllipse (r.getCentreX() - thumbSize * 0.5f, r.getCentreY() - thumbSize * 0.5f,
                   thumbSize, thumbSize, 1.5f);
}

// -----------------------------------------------------------------------------
// drawComboBoxBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawComboBoxBackground (juce::Graphics& g, int width, int height,
                                                bool isMouseDown, int /*buttonX*/, int /*buttonY*/,
                                                int /*buttonW*/, int /*buttonH*/, juce::ComboBox& /*box*/)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat();

    if (isMouseDown)
        g.setColour (PrimaryPurpleDark.withAlpha (0.3f));
    else
        g.setColour (FieldBg);

    g.fillRoundedRectangle (r, (float) ControlCornerRadius);

    g.setColour (FieldBorder);
    g.drawRoundedRectangle (r, (float) ControlCornerRadius, (float) BorderWidth);
}

// -----------------------------------------------------------------------------
// drawPopupMenuBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat();
    g.setColour (PanelDark);
    g.fillRoundedRectangle (r, (float) ControlCornerRadius);
    g.setColour (FieldBorder);
    g.drawRoundedRectangle (r, (float) ControlCornerRadius, (float) BorderWidth);
}

// -----------------------------------------------------------------------------
// drawPopupMenuItem
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSelected, int /*i*/, int /*totalItems*/,
                                           const juce::String& text, const juce::String& /*shortcut*/,
                                           bool isDisabled, bool /*isChecked*/, bool /*possibleSubmenu*/)
{
    auto r = area.toFloat();

    if (isSelected)
    {
        juce::ColourGradient grad (PrimaryPurpleDark, r.getX(), r.getY(),
                                   PrimaryPurple,     r.getX(), r.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, (float) ControlCornerRadius);
        g.setColour (juce::Colours::white);
    }
    else
    {
        g.setColour (FieldBg);
        g.fillRoundedRectangle (r, (float) ControlCornerRadius);
        g.setColour (isDisabled ? TextDisabled : TextMain);
    }

    g.setFont (uiFont (FontSizeLabel));
    g.drawFittedText (text, r.toNearestInt().reduced (10, 0), juce::Justification::centredLeft, 1);
}

// -----------------------------------------------------------------------------
// drawKeyboardBackground
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawKeyboardBackground (juce::Graphics& g, const juce::Rectangle<int>& area)
{
    g.setColour (juce::Colour (0x0A0A1Eu));
    g.fillRoundedRectangle (area.toFloat(), 8.0f);
    g.setColour (FieldBorder);
    g.drawRoundedRectangle (area.toFloat(), 8.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// drawWhiteKey
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawWhiteKey (juce::Graphics& g, const juce::Rectangle<int>& area,
                                      float /*keyWidth*/, float /*keyHeight*/, float /*keyY*/, int /*keyIndex*/)
{
    g.setColour (juce::Colour (0xF2F0FFu));
    g.fillRoundedRectangle (area.toFloat().reduced (1.0f, 0.0f), 4.0f);
    g.setColour (juce::Colour (0x2A2A55u).withAlpha (0.3f));
    g.drawRoundedRectangle (area.toFloat().reduced (1.0f, 0.0f), 4.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// drawBlackKey
// -----------------------------------------------------------------------------
void OmnivyLookAndFeel::drawBlackKey (juce::Graphics& g, const juce::Rectangle<int>& area,
                                      float /*keyWidth*/, float /*keyHeight*/, float /*keyY*/, int /*keyIndex*/)
{
    g.setColour (juce::Colour (0x0E0E26u));
    g.fillRoundedRectangle (area.toFloat(), 3.0f);
    g.setColour (juce::Colour (0x2A2A55u));
    g.drawRoundedRectangle (area.toFloat(), 3.0f, 1.0f);
}

} // namespace gui
} // namespace omnivy
