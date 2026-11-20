#include "GUI/EnvelopeDisplay.h"

#include <cmath>

namespace omnivy {
namespace gui {

EnvelopeDisplay::EnvelopeDisplay()
{
    setInterceptsMouseClicks (false);
}

void EnvelopeDisplay::setParameters (float attack, float decay, float sustain, float release)
{
    m_attack = attack;
    m_decay = decay;
    m_sustain = sustain;
    m_release = release;
    repaint();
}

void EnvelopeDisplay::setColour (juce::Colour colour)
{
    m_curveColour = colour;
    repaint();
}

float EnvelopeDisplay::timeToX (float seconds) const
{
    constexpr float maxT = 2.0f; // janela de exibição: 2 segundos
    float t = juce::jlimit (0.0f, maxT, seconds);
    return std::log (1.0f + t) / std::log (1.0f + maxT); // 0..1, escala logarítmica
}

float EnvelopeDisplay::xToTime (float x) const
{
    constexpr float maxT = 2.0f;
    return std::exp (x * std::log (1.0f + maxT)) - 1.0f;
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    juce::Rectangle<float> plot (area.getX() + 6.0f, area.getY() + 6.0f,
                                 area.getWidth() - 12.0f, area.getHeight() - 12.0f);

    // Eixos.
    g.setColour (Border.brighter (0.2f));
    g.drawLine (plot.getX(), plot.getBottom(), plot.getRight(), plot.getBottom(), 1.0f);
    g.drawLine (plot.getX(), plot.getBottom(), plot.getX(), plot.getY(), 1.0f);

    // Pontos da curva ADSR em coordenadas de tela (y para baixo).
    auto pointFor = [&] (float t, float level) -> juce::Point<float>
    {
        float x = plot.getX() + timeToX (t) * plot.getWidth();
        float y = plot.getBottom() - juce::jlimit (0.0f, 1.0f, level) * plot.getHeight();
        return { x, y };
    };

    float attackEnd = m_attack;
    float decayEnd = attackEnd + m_decay;
    float sustainEnd = decayEnd + 0.5f; // sustenta por 0.5s na exibição
    float releaseEnd = sustainEnd + m_release;

    juce::Path curve;
    curve.startNewSubPath (pointFor (0.0f, 0.0f));           // início em zero
    curve.lineTo (pointFor (attackEnd, 1.0f));               // ataque até 1.0
    curve.lineTo (pointFor (decayEnd, m_sustain));           // decaimento até sustain
    curve.lineTo (pointFor (sustainEnd, m_sustain));         // sustain horizontal
    curve.lineTo (pointFor (releaseEnd, 0.0f));              // release até zero

    g.setColour (m_curveColour.withAlpha (0.35f));
    g.strokePath (curve, juce::PathStrokeType (1.5f));
    g.setColour (m_curveColour);
    g.strokePath (curve, juce::PathStrokeType (2.2f));

    // Marcador do nível de sustain.
    g.setColour (TextMuted);
    g.setFont (uiFont (FontSizeMicro));
    g.drawText ("S", plot.getRight() - 14, plot.getBottom() - m_sustain * plot.getHeight() - 7,
                12, 12, juce::Justification::centred, true);
}

void EnvelopeDisplay::resized()
{
    // O caminho é reconstruido em paint().
}

} // namespace gui
} // namespace omnivy
