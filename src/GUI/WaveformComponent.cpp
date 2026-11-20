#include "GUI/WaveformComponent.h"

#include <cmath>

namespace omnivy {
namespace gui {

WaveformComponent::WaveformComponent()
{
    setInterceptsMouseClicks (false, false);
    setSize (m_thumbW, m_thumbH);
}

void WaveformComponent::setWaveform (dsp::Waveform waveform)
{
    if (m_waveform != waveform)
    {
        m_waveform = waveform;
        repaint();
    }
}

void WaveformComponent::setColour (juce::Colour colour)
{
    m_waveColour = colour;
    repaint();
}

void WaveformComponent::setSizeHint (int width, int height)
{
    m_thumbW = width;
    m_thumbH = height;
    setSize (width, height);
}

juce::Path WaveformComponent::buildWaveformPath (dsp::Waveform w, int numPoints) const
{
    juce::Path path;
    if (numPoints < 2)
        return path;

    const float twoPi = 2.0f * (float) juce::MathConstants<double>::pi;
    const float noiseFreq = 0.0f;

    path.startNewSubPath (0.0f, 0.0f);

    for (int i = 0; i < numPoints; ++i)
    {
        float t = static_cast<float> (i) / static_cast<float> (numPoints - 1);
        float phase = t * twoPi;
        float y = 0.0f;

        switch (w)
        {
            case dsp::Waveform::Sine:
                y = std::sin (phase);
                break;
            case dsp::Waveform::Saw:
                y = (t * 2.0f) - 1.0f; // rampa de -1 a +1
                break;
            case dsp::Waveform::Square:
                y = (phase < juce::MathConstants<float>::pi) ? -1.0f : 1.0f;
                break;
            case dsp::Waveform::Triangle:
                y = 2.0f * std::fabsf (fmodf (phase + juce::MathConstants<float>::piOverTwo,
                                              twoPi) / juce::MathConstants<float>::pi - 1.0f) - 1.0f;
                break;
            case dsp::Waveform::Pulse:
                y = (phase < juce::MathConstants<float>::pi * 0.75f) ? -1.0f : 1.0f;
                break;
            case dsp::Waveform::Noise:
            {
                // Ruído pré-determinado por amostra (mesmo seed => mesma aparência).
                unsigned int seed = 12345u + static_cast<unsigned int> (i * 1103515245u);
                seed = (seed * 1103515245u + 12345u) & 0x7fffffffu;
                y = (static_cast<float> (seed) / static_cast<float> (0x7fffffffu)) * 2.0f - 1.0f;
                break;
            }
            default:
                y = std::sin (phase);
                break;
        }

        if (i == 0)
            path.startNewSubPath (t, y);
        else
            path.lineTo (t, y);
    }

    return path;
}

void WaveformComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    auto centre = area.getCentre();

    // Fundo escuro do controle.
    g.setColour (ControlSurface);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);
    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    // Área de desenho da forma de onda (centrada, com margem).
    float margin = 6.0f;
    juce::Rectangle<float> plotArea (area.getX() + margin, area.getY() + margin,
                                     area.getWidth() - margin * 2.0f,
                                     area.getHeight() - margin * 2.0f);

    int numPoints = (int) juce::jlimit (32, 256, (int) plotArea.getWidth() * 4);
    juce::Path path = buildWaveformPath (m_waveform, numPoints);

    // Mapeia o caminho do espaço normalizado (x: 0..1, y: -1..1) para plotArea.
    juce::AffineTransform transform = juce::AffineTransform::scale (plotArea.getWidth(),
                                                                    plotArea.getHeight() * 0.5f)
                                        .translated (plotArea.getX(),
                                                     plotArea.getCentreY());

    path.applyTransform (transform);

    // Linha da forma de onda com cor semântica + brilho suave.
    g.setColour (m_waveColour.withAlpha (0.35f));
    g.strokePath (path, juce::PathStrokeType (1.5f));
    g.setColour (m_waveColour);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    // Linha central (zero).
    g.setColour (Border.brighter (0.3f));
    g.drawLine (plotArea.getX(), plotArea.getCentreY(), plotArea.getRight(),
                plotArea.getCentreY(), 1.0f);
}

void WaveformComponent::resized()
{
    // Nada a fazer: o caminho é reconstruido em paint().
}

} // namespace gui
} // namespace omnivy
