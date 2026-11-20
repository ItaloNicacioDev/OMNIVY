#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// PanelComponent
//
// Painel modular com borda sutil, título opcional e glow quando destacado.
// O conteúdo é adicionado via addAndMakeVisible ao próprio panel (ele é um
// Container de layout simples; subclasses definem o resized()).
// -----------------------------------------------------------------------------
class PanelComponent : public juce::Component
{
public:
    PanelComponent (const juce::String& title = {},
                    bool highlighted = false);

    ~PanelComponent() override = default;

    void paint (juce::Graphics&) override;

    void setHighlighted (bool highlighted);
    bool isHighlighted() const noexcept { return m_highlighted; }

    juce::Label& titleLabel() { return m_titleLabel; }

protected:
    juce::Colour m_borderColour = Border;
    bool m_highlighted = false;

private:
    juce::Label m_titleLabel;
    bool m_titleEmpty = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelComponent)
};

// -----------------------------------------------------------------------------
// IconButtonComponent
//
// Botão quadrado com ícone (texto/símbolo), estados ligado/desligado, hover
// e desabilitado. Usado para power, favorito, salvar, etc.
// -----------------------------------------------------------------------------
class IconButtonComponent : public juce::Button
{
public:
    IconButtonComponent (const juce::String& icon,
                         const juce::String& tooltip = {},
                         juce::Colour iconColour = TextMain);

    ~IconButtonComponent() override = default;

    void setToggleState (bool shouldBeOn, juce::NotificationType notification);
    bool getToggleState() const noexcept { return m_toggleState; }

    void setIconColour (juce::Colour colour);
    juce::Colour getIconColour() const noexcept { return m_iconColour; }

    void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

private:
    juce::String m_icon;
    juce::Colour m_iconColour;
    bool m_toggleState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButtonComponent)
};

} // namespace gui
} // namespace omnivy
