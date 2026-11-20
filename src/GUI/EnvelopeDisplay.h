#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// EnvelopeDisplay
//
// Desenha a curva ADSR (ataque, decaimento, sustain, release) a partir dos
// valores reais dos parâmetros de envelope. É uma visualização dos valores
// configurados, não uma medição de áudio.
// -----------------------------------------------------------------------------
class EnvelopeDisplay : public juce::Component
{
public:
    EnvelopeDisplay();

    ~EnvelopeDisplay() override = default;

    void setParameters (float attack, float decay, float sustain, float release);
    void setColour (juce::Colour colour);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    float m_attack = 0.01f;    // segundos
    float m_decay = 0.1f;
    float m_sustain = 0.7f;    // 0..1
    float m_release = 0.2f;    // segundos
    juce::Colour m_curveColour = Env1Colour;

    // Converte tempo (segundos) para posição X normalizada (0..1) usando escala
    // logarítmica para curtos ataques ficarem legíveis.
    float timeToX (float seconds) const;
    float xToTime (float x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplay)
};

} // namespace gui
} // namespace omnivy
