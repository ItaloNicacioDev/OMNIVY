#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace omnivy {
namespace gui {

// ============================================================================
// OMNIVY — Design System (Dark Neon / Cyber-Synth)
// Todas as cores, dimensões e fontes estão centralizadas aqui.
// ============================================================================

// -----------------------------------------------------------------------------
// Superfícies da janela (fundo com gradiente diagonal)
// -----------------------------------------------------------------------------
inline const juce::Colour WindowBgTop    = juce::Colour (0x0A0A1Eu);  // #0A0A1E
inline const juce::Colour WindowBgMid    = juce::Colour (0x0D0D26u);  // #0D0D26
inline const juce::Colour WindowBgBottom = juce::Colour (0x0A0A1Cu);  // #0A0A1C
inline const juce::Colour WindowBorder   = juce::Colour (0x2A2A55u);  // #2A2A55
inline const juce::Colour Border         = juce::Colour (0x2A2A55u);  // #2A2A55 (alias para compatibilidade)

// -----------------------------------------------------------------------------
// Superfícies dos painéis
// -----------------------------------------------------------------------------
inline const juce::Colour PanelTop     = juce::Colour (0x14142Fu);  // #14142F
inline const juce::Colour PanelBottom  = juce::Colour (0x101028u);  // #101028
inline const juce::Colour PanelLight   = juce::Colour (0x15152Fu);  // #15152F (alias para compatibilidade)
inline const juce::Colour PanelBorder  = juce::Colour (0x2B2B58u);  // #2B2B58
inline const juce::Colour PanelBorderAlpha = juce::Colour (0x2B2B58u).withAlpha (0.9f);
inline const juce::Colour FieldBg      = juce::Colour (0x0E0E24u);  // #0E0E24 (dropdowns/displays)
inline const juce::Colour FieldBorder  = juce::Colour (0x34346Au);  // #34346A

// -----------------------------------------------------------------------------
// Identidade primária (roxos)
// -----------------------------------------------------------------------------
inline const juce::Colour PrimaryPurple     = juce::Colour (0x8B5CF6u);  // #8B5CF6
inline const juce::Colour PrimaryPurpleDark = juce::Colour (0x6D28D9u);  // #6D28D9
inline const juce::Colour PrimaryPurpleLight = juce::Colour (0xA78BFau); // #A78BFA
inline const juce::Colour GlowActive        = juce::Colour (0x8B5CF6u).withAlpha (0.55f);

// Gradiente primário (6D28D9 → 8B5CF6)
inline juce::ColourGradient primaryGradient()
{
    return juce::ColourGradient (PrimaryPurpleDark, 0.0f, 0.0f,
                                 PrimaryPurple,     1.0f, 0.0f,
                                 false);
}

// -----------------------------------------------------------------------------
// Cores semânticas dos osciladores (usadas na barra lateral, ponto do routing,
// matriz de modulação)
// -----------------------------------------------------------------------------
inline const juce::Colour Osc1Colour = juce::Colour (0x3B82F6u); // azul
inline const juce::Colour Osc2Colour = juce::Colour (0xA855F7u); // roxo (ref: Osc2)
inline const juce::Colour Osc3Colour = juce::Colour (0x22D3EEu); // ciano/verde-água
inline const juce::Colour Osc4Colour = juce::Colour (0x22C55Eu); // verde
inline const juce::Colour Osc5Colour = juce::Colour (0xF59E0Bu); // laranja
inline const juce::Colour Osc6Colour = juce::Colour (0xEC4899u); // rosa

inline const juce::Colour LFO1Colour  = Osc1Colour;
inline const juce::Colour LFO2Colour  = Osc4Colour;
inline const juce::Colour Env1Colour  = juce::Colour (0xFACC15u); // amarelo
inline const juce::Colour Env2Colour  = juce::Colour (0x60A5FAu); // azul claro
inline const juce::Colour MacroColour = Osc6Colour;

// Aliases para compatibilidade com código existente
inline const juce::Colour BackgroundMain = WindowBgTop;
inline const juce::Colour PanelDark      = PanelTop;
inline const juce::Colour ControlSurface = FieldBg;

// Retorna a cor semântica de um oscilador (index 0..5 → OSC 1..6).
inline juce::Colour oscillatorColour (int index)
{
    switch (index)
    {
        case 0: return Osc1Colour;
        case 1: return Osc2Colour;
        case 2: return Osc3Colour;
        case 3: return Osc4Colour;
        case 4: return Osc5Colour;
        case 5: return Osc6Colour;
        default: return PrimaryPurple;
    }
}

// -----------------------------------------------------------------------------
// Texto
// -----------------------------------------------------------------------------
inline const juce::Colour TextMain      = juce::Colour (0xF2F2FAu);  // #F2F2FA
inline const juce::Colour TextSecondary = juce::Colour (0x9A9AC0u);  // #9A9AC0
inline const juce::Colour TextDisabled  = juce::Colour (0x5A5A85u);  // #5A5A85
inline const juce::Colour TextMuted     = juce::Colour (0x6E7099u);

// -----------------------------------------------------------------------------
// VU meter (verde → amarelo → vermelho)
// -----------------------------------------------------------------------------
inline const juce::Colour VuLow  = juce::Colour (0x22C55Eu);  // #22C55E
inline const juce::Colour VuMid  = juce::Colour (0xEAB308u);  // #EAB308
inline const juce::Colour VuHigh = juce::Colour (0xEF4444u);  // #EF4444

// -----------------------------------------------------------------------------
// Tipografia
// -----------------------------------------------------------------------------
inline constexpr float FontSizePanelTitle = 15.0f;
inline constexpr float FontSizeLabel      = 12.0f;
inline constexpr float FontSizeMicro      = 10.0f;
inline constexpr float FontSizeValue      = 12.0f;
inline constexpr float FontSizePreset     = 16.0f;
inline constexpr float FontSizeLogo       = 48.0f;
inline constexpr float FontSizeSubtitle   = 17.0f;

inline const juce::Colour LabelColor   = juce::Colour (0x9A9AC0u);  // labels 12px
inline const juce::Colour ValueColor   = juce::Colour (0xE6E6F5u);  // valores numéricos

// Embute a fonte Inter se disponível; caso contrário usa Segoe UI/Roboto/Helvetica.
inline juce::Font uiFont (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
{
    return juce::Font ("Inter", size, style);
}

// -----------------------------------------------------------------------------
// Formas / dimensões
// -----------------------------------------------------------------------------
inline constexpr int PanelCornerRadius   = 10;
inline constexpr int ControlCornerRadius = 6;
inline constexpr int WindowCornerRadius  = 14;
inline constexpr int BorderWidth         = 1;

// Tamanhos do knob (px)
inline constexpr int KnobSizeSmall = 36;
inline constexpr int KnobSizeMedium = 52;
inline constexpr int KnobSizeLarge = 84;

// Arco do knob: ~270 graus (inicia em 135°, termina em 405° → 135+270)
inline constexpr float KnobArcStartDegrees = 135.0f;
inline constexpr float KnobArcSweepDegrees = 270.0f;
inline constexpr float KnobArcThickness    = 4.0f;

// Layout da janela principal (cinco faixas, base 1672 x 941)
inline constexpr int TopBarHeight      = 95;   // 0–95
inline constexpr int TabBarHeight      = 50;   // 97–147
inline constexpr int BottomBandHeight  = 135;  // 755–890
inline constexpr int FooterHeight      = 41;   // 900–941

// -----------------------------------------------------------------------------
// Look and Feel personalizada
// -----------------------------------------------------------------------------
class OmnivyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OmnivyLookAndFeel();

    // --- Desenho de fundo / painéis -----------------------------------------
    void drawPanelBackground (juce::Graphics& g, const juce::Rectangle<float>& area,
                              bool highlighted = false);

    // --- Botões ícone --------------------------------------------------------
    void drawIconButtonBackground (juce::Graphics& g, const juce::Rectangle<float>& area,
                                   bool isMouseOver, bool isToggleOn, bool isEnabled);

    // --- Botões de texto -----------------------------------------------------
    void drawTextButtonBackground (juce::Graphics& g, const juce::Rectangle<float>& area,
                                   bool isMouseOver, bool isToggleOn, bool isEnabled);

    // --- Abas (TabbedButtonBar::LookAndFeelMethods) --------------------------
    void drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                        bool isMouseOver, bool isMouseDown) override;
    juce::Font getTabButtonFont (juce::TabBarButton& button, float height) override;

    // --- Slider --------------------------------------------------------------
    void drawLinearSliderBackground (juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     const juce::Slider::SliderStyle, juce::Slider& slider) override;
    void drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                const juce::Slider::SliderStyle, juce::Slider& slider) override;

    // --- ComboBox ------------------------------------------------------------
    void drawComboBoxBackground (juce::Graphics& g, int width, int height,
                                 bool isMouseDown, int buttonX, int buttonY, int buttonW,
                                 int buttonH, juce::ComboBox& box);
    void drawPopupMenuBackground (juce::Graphics& g, int width, int height);
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSelected, int i, int totalItems,
                            const juce::String& text, const juce::String& shortcut,
                            bool isDisabled, bool isChecked, bool possibleSubmenu);

    // --- MIDI Keyboard -------------------------------------------------------
    void drawKeyboardBackground (juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawWhiteKey (juce::Graphics& g, const juce::Rectangle<int>& area,
                       float keyWidth, float keyHeight, float keyY, int keyIndex);
    void drawBlackKey (juce::Graphics& g, const juce::Rectangle<int>& area,
                       float keyWidth, float keyHeight, float keyY, int keyIndex);
};

} // namespace gui
} // namespace omnivy
