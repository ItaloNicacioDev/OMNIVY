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
// Superfícies
// -----------------------------------------------------------------------------
inline constexpr juce::Colour BackgroundMain    = juce::Colour (0x0A0A1Eu);  // #0A0A1E
inline constexpr juce::Colour PanelDark         = juce::Colour (0x12122Bu);  // #12122B
inline constexpr juce::Colour PanelLight        = juce::Colour (0x15152Fu);  // #15152F
inline constexpr juce::Colour AreaInner         = juce::Colour (0x0C0C22u);  // #0C0C22
inline constexpr juce::Colour ControlSurface    = juce::Colour (0x1A1A3Au);  // #1A1A3A
inline constexpr juce::Colour Border            = juce::Colour (0x2A2A55u);  // #2A2A55

// -----------------------------------------------------------------------------
// Identidade primária (roxos)
// -----------------------------------------------------------------------------
inline constexpr juce::Colour PrimaryPurple     = juce::Colour (0x8B5CF6u);  // #8B5CF6
inline constexpr juce::Colour PrimaryPurpleDark = juce::Colour (0x6D28D9u);  // #6D28D9
inline constexpr juce::Colour PrimaryPurpleLight = juce::Colour (0xA78BFau); // #A78BFA
inline constexpr juce::Colour GlowActive        = juce::Colour (0x8B5CF6u).withAlpha (0.55f);

// Gradiente primário (6D28D9 → 8B5CF6)
inline juce::ColourGradient primaryGradient()
{
    return juce::ColourGradient (PrimaryPurpleDark, 0.0f, 0.0f,
                                 PrimaryPurple,     1.0f, 0.0f);
}

// -----------------------------------------------------------------------------
// Cores semânticas (por fonte de modulação / oscilador)
// -----------------------------------------------------------------------------
inline constexpr juce::Colour Osc1Colour = juce::Colour (0x3B82F6u); // azul
inline constexpr juce::Colour Osc2Colour = juce::Colour (0x22D3EEu); // ciano
inline constexpr juce::Colour Osc3Colour = juce::Colour (0x2DD4BFu); // turquesa
inline constexpr juce::Colour Osc4Colour = juce::Colour (0x22C55Eu); // verde
inline constexpr juce::Colour Osc5Colour = juce::Colour (0xF59E0Bu); // âmbar
inline constexpr juce::Colour Osc6Colour = juce::Colour (0xEC4899u); // rosa

inline constexpr juce::Colour LFO1Colour  = Osc1Colour;
inline constexpr juce::Colour LFO2Colour  = Osc4Colour;
inline constexpr juce::Colour Env1Colour  = juce::Colour (0xFACC15u); // amarelo
inline constexpr juce::Colour Env2Colour  = juce::Colour (0x60A5FAu); // azul claro
inline constexpr juce::Colour MacroColour = Osc6Colour;

// Retorna a cor semántica de um oscilador (index 0..5 → OSC 1..6).
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
// Tipografia
// -----------------------------------------------------------------------------
inline constexpr float FontSizePanelTitle = 16.0f;
inline constexpr float FontSizeLabel      = 13.0f;
inline constexpr float FontSizeMicro      = 10.0f;
inline constexpr float FontSizeValue      = 12.0f;

inline constexpr juce::Colour TextMain      = juce::Colour (0xF2F3FFu);
inline constexpr juce::Colour TextSecondary = juce::Colour (0xA5A8C8u);
inline constexpr juce::Colour TextMuted     = juce::Colour (0x6E7099u);

// Tenta usar Inter se disponível; caso contrário usa a fonte padrão da JUCE.
inline juce::Font uiFont (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
{
    return juce::Font ("Inter", size, style);
}

// -----------------------------------------------------------------------------
// Formas / dimensões
// -----------------------------------------------------------------------------
inline constexpr int PanelCornerRadius   = 10;
inline constexpr int ControlCornerRadius = 6;
inline constexpr int BorderWidth         = 1;

// Tamanhos do knob (px)
inline constexpr int KnobSizeSmall = 36;
inline constexpr int KnobSizeMedium = 52;
inline constexpr int KnobSizeLarge = 72;

// Arco do knob: ~270 graus (inicia em 135°, termina em 405° → 135+270)
inline constexpr float KnobArcStartDegrees = 135.0f;
inline constexpr float KnobArcSweepDegrees = 270.0f;

// Layout da janela principal (cinco faixas)
inline constexpr int TopBarHeight      = 72;
inline constexpr int TabBarHeight      = 52;
inline constexpr int BottomBandHeight  = 150; // teclado + master

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

    // --- Labels --------------------------------------------------------------
    void drawLabel (juce::Graphics& g, int x, int y, int width, int height,
                    const juce::String& text, const juce::Font& font,
                    juce::Justification justification, juce::Colour colour) override;

    // --- Abas ----------------------------------------------------------------
    juce::Font getTabButtonFont (juce::TabBarButton&, int tabDepth) override;
    int getTabButtonBestWidth (juce::TabBarButton&, int tabDepth) override;
    void drawTabButton (juce::TabBarButton&, juce::Graphics&,
                        bool isMouseOver, bool isTabFront) override;

private:
    juce::Font m_font;
};

} // namespace gui
} // namespace omnivy
