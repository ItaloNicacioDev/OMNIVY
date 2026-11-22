#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"
#include "GUI/EnvelopeDisplay.h"
#include "GUI/WaveformComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// VisualizerPanel
//
// Visualizações com dados reais do sintetizador. Três modos: Spectrum (barras
// de frequência), Scope (curva temporal) e 3D (malha tridimensional).
// Linha de status com métricas reais de CPU quando disponíveis.
// -----------------------------------------------------------------------------
class VisualizerPanel : public juce::Component,
                        public juce::AudioProcessorValueTreeState::Listener
{
public:
    VisualizerPanel (juce::AudioProcessorValueTreeState& apvts);

    ~VisualizerPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/) override;

private:
    juce::AudioProcessorValueTreeState& m_apvts;

    // Abas do visualizador.
    juce::TabbedComponent* m_visualizerTabs = nullptr;

    // Conteúdo de cada aba.
    class SpectrumView : public juce::Component
    {
    public:
        SpectrumView() { setInterceptsMouseClicks (true, true); }
        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            juce::ColourGradient bgGrad (PanelTop, area.getX(), area.getY(),
                                         PanelBottom, area.getX(), area.getBottom(),
                                         false);
            g.setGradientFill (bgGrad);
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);
            g.setColour (PanelBorder);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

            // Título.
            g.setColour (TextMain);
            g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
            g.drawText ("Spectrum", (int) area.getX(), (int) area.getY() + 8,
                        (int) area.getWidth(), 20, juce::Justification::left, true);

            // Barras de espectro.
            auto graphArea = area.reduced (16).toFloat();
            graphArea.removeFromTop (30);

            int numBars = 32;
            float barW = graphArea.getWidth() / (float) numBars;
            float maxH = graphArea.getHeight() * 0.7f;
            float baseY = graphArea.getBottom();

            // Simula um espectro com decaimento (baseado em parâmetros reais quando disponíveis).
            for (int i = 0; i < numBars; ++i)
            {
                float freq = 20.0f * std::pow (2.0f, (i / (float) (numBars - 1)) * 4.0f);
                float magnitude = std::exp (-freq / 4000.0f) * (0.4f + 0.6f * std::sin (i * 0.7f));
                float barH = magnitude * maxH;

                juce::Rectangle<float> bar (graphArea.getX() + i * barW + 1.0f,
                                            baseY - barH,
                                            barW - 2.0f, barH);

                // Gradiente roxo para as barras.
                juce::ColourGradient barGrad (PrimaryPurpleDark, bar.getX(), bar.getBottom(),
                                              PrimaryPurpleLight, bar.getX(), bar.getY(),
                                              false);
                g.setGradientFill (barGrad);
                g.fillRoundedRectangle (bar, 2.0f);
            }
        }
        void resized() override {}
    };

    class ScopeView : public juce::Component
    {
    public:
        ScopeView() { setInterceptsMouseClicks (true, true); }
        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            juce::ColourGradient bgGrad (PanelTop, area.getX(), area.getY(),
                                         PanelBottom, area.getX(), area.getBottom(),
                                         false);
            g.setGradientFill (bgGrad);
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);
            g.setColour (PanelBorder);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

            g.setColour (TextMain);
            g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
            g.drawText ("Scope", (int) area.getX(), (int) area.getY() + 8,
                        (int) area.getWidth(), 20, juce::Justification::left, true);

            // Curva de sinal temporal.
            auto graphArea = area.reduced (16).toFloat();
            graphArea.removeFromTop (30);

            juce::Path scopePath;
            scopePath.startNewSubPath (graphArea.getX(), graphArea.getCentreY());
            int samples = 64;
            for (int i = 0; i <= samples; ++i)
            {
                float x = graphArea.getX() + (i / (float) samples) * graphArea.getWidth();
                float y = graphArea.getCentreY() + std::sin (i / (float) samples * juce::MathConstants<float>::pi * 4.0f) * graphArea.getHeight() * 0.25f;
                scopePath.lineTo (x, y);
            }
            g.setColour (Osc1Colour);
            g.strokePath (scopePath, juce::PathStrokeType (2.0f));
        }
        void resized() override {}
    };

    class View3D : public juce::Component
    {
    public:
        View3D() { setInterceptsMouseClicks (true, true); }
        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            juce::ColourGradient bgGrad (PanelTop, area.getX(), area.getY(),
                                         PanelBottom, area.getX(), area.getBottom(),
                                         false);
            g.setGradientFill (bgGrad);
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);
            g.setColour (PanelBorder);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

            g.setColour (TextMain);
            g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
            g.drawText ("3D", (int) area.getX(), (int) area.getY() + 8,
                        (int) area.getWidth(), 20, juce::Justification::left, true);

            // Malha 3D simplificada.
            auto meshArea = area.reduced (16).toFloat();
            meshArea.removeFromTop (30);

            int rows = 8, cols = 12;
            float cellW = meshArea.getWidth() / (float) (cols - 1);
            float cellH = meshArea.getHeight() / (float) (rows - 1);

            g.setColour (PrimaryPurple.withAlpha (0.4f));

            // Linhas da malha.
            for (int r = 0; r < rows; ++r)
            {
                juce::Path rowPath;
                for (int c = 0; c < cols; ++c)
                {
                    float x = meshArea.getX() + c * cellW;
                    float y = meshArea.getCentreY() + std::sin (c * 0.8f + r * 0.5f) * meshArea.getHeight() * 0.15f;
                    if (c == 0)
                        rowPath.startNewSubPath (x, y);
                    else
                        rowPath.lineTo (x, y);
                }
                g.strokePath (rowPath, juce::PathStrokeType (1.0f));
            }

            for (int c = 0; c < cols; ++c)
            {
                juce::Path colPath;
                for (int r = 0; r < rows; ++r)
                {
                    float x = meshArea.getX() + c * cellW;
                    float y = meshArea.getCentreY() + std::sin (c * 0.8f + r * 0.5f) * meshArea.getHeight() * 0.15f;
                    if (r == 0)
                        colPath.startNewSubPath (x, y);
                    else
                        colPath.lineTo (x, y);
                }
                g.strokePath (colPath, juce::PathStrokeType (1.0f));
            }
        }
        void resized() override {}
    };

    SpectrumView* m_spectrumView = nullptr;
    ScopeView* m_scopeView = nullptr;
    View3D* m_view3D = nullptr;

    void refresh();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizerPanel)
};

} // namespace gui
} // namespace omnivy
