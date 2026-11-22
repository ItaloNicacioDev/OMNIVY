#include "GUI/WavetablePanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

WavetablePanel::WavetablePanel (juce::AudioProcessorValueTreeState& /*apvts*/)
{
    setInterceptsMouseClicks (true, true);

    // --- Cabeçalho: dropdown + 4 icon buttons ---------------------------------
    m_wavetableSelector = new juce::ComboBox ("Wavetable");
    m_wavetableSelector->addItem ("Custom 1", 1);
    m_wavetableSelector->setSelectedId (1);
    m_wavetableSelector->setTooltip ("A wavetable é gerada pelo DSP (seno). "
                                     "Importar/editar ainda não implementado.");
    addAndMakeVisible (*m_wavetableSelector);

    m_wavetableAddButton = new juce::TextButton ("+");
    m_wavetableAddButton->setTooltip ("Adicionar wavetable");
    m_wavetableAddButton->setEnabled (false);
    addAndMakeVisible (*m_wavetableAddButton);

    m_wavetableRefreshButton = new juce::TextButton ("↻");
    m_wavetableRefreshButton->setTooltip ("Atualizar wavetable");
    m_wavetableRefreshButton->setEnabled (false);
    addAndMakeVisible (*m_wavetableRefreshButton);

    m_wavetableCopyButton = new juce::TextButton ("📋");
    m_wavetableCopyButton->setTooltip ("Copiar wavetable");
    m_wavetableCopyButton->setEnabled (false);
    addAndMakeVisible (*m_wavetableCopyButton);

    m_wavetableDeleteButton = new juce::TextButton ("🗑");
    m_wavetableDeleteButton->setTooltip ("Excluir wavetable");
    m_wavetableDeleteButton->setEnabled (false);
    addAndMakeVisible (*m_wavetableDeleteButton);

    // --- Surface 3D / Wireframe (área central) --------------------------------
    m_waveformPreview = new WaveformComponent();
    m_waveformPreview->setWaveform (dsp::Waveform::Sine);
    m_waveformPreview->setColour (PrimaryPurple);
    m_waveformPreview->setSizeHint (320, 220);
    addAndMakeVisible (*m_waveformPreview);

    // --- Knobs Morph / Phase / Warp (coluna direita) --------------------------
    m_morphKnob = new juce::Slider ("Morph");
    m_morphKnob->setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    m_morphKnob->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    m_morphKnob->setRange (0.0, 1.0, 0.01);
    m_morphKnob->setValue (0.5);
    m_morphKnob->setColour (juce::Slider::thumbColourId, PrimaryPurple);
    addAndMakeVisible (*m_morphKnob);

    m_phaseKnob = new juce::Slider ("Phase");
    m_phaseKnob->setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    m_phaseKnob->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    m_phaseKnob->setRange (0.0, 1.0, 0.01);
    m_phaseKnob->setValue (0.5);
    m_phaseKnob->setColour (juce::Slider::thumbColourId, PrimaryPurpleLight);
    addAndMakeVisible (*m_phaseKnob);

    m_warpKnob = new juce::Slider ("Warp");
    m_warpKnob->setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    m_warpKnob->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    m_warpKnob->setRange (0.0, 1.0, 0.01);
    m_warpKnob->setValue (0.5);
    m_warpKnob->setColour (juce::Slider::thumbColourId, PrimaryPurpleDark);
    addAndMakeVisible (*m_warpKnob);

    // --- Nota informativa -------------------------------------------------------
    m_infoLabel.setText ("Wavetable compartilhada gerada pelo DSP (seno, 2048 amostras). "
                         "Importar, atualizar, copiar e excluir dependem de implementação no DSP.",
                         juce::dontSendNotification);
    m_infoLabel.setFont (uiFont (FontSizeMicro));
    m_infoLabel.setColour (juce::Label::textColourId, TextMuted);
    m_infoLabel.setJustificationType (juce::Justification::centred);
    m_infoLabel.setEnabled (false);
    addAndMakeVisible (m_infoLabel);
}

void WavetablePanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Fundo do painel com gradiente sutil.
    juce::ColourGradient bgGrad (PanelDark, area.getX(), area.getY(),
                                 BackgroundMain, area.getRight(), area.getBottom(),
                                 false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (area, (float) ControlCornerRadius);

    g.setColour (Border);
    g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

    // Desenha malha 3D (wireframe da wavetable) com profundidade e perspectiva.
    auto wireArea = area.reduced (20).toFloat();
    wireArea.removeFromTop (44); // espaço do cabeçalho
    wireArea.removeFromBottom (30); // espaço do footer

    int cols = 16;
    int rows = 8;
    float cellW = wireArea.getWidth() / (float) (cols - 1);
    float cellH = wireArea.getHeight() / (float) (rows - 1);

    g.setColour (Border.withAlpha (0.25f));

    // Linhas verticais da malha (perspectiva 3D).
    for (int c = 0; c < cols; ++c)
    {
        float x = wireArea.getX() + c * cellW;
        float topY = wireArea.getY();
        float bottomY = wireArea.getBottom();
        // Perspectiva: curvar levemente as linhas verticais.
        float curve = std::sin (juce::MathConstants<float>::pi * c / (float) (cols - 1)) * 8.0f;
        g.drawLine (x - curve * 0.3f, topY, x + curve * 0.3f, bottomY, 1.0f);
    }

    // Linhas horizontais com onda senoidal (wireframe da wavetable).
    for (int r = 0; r < rows; ++r)
    {
        float y = wireArea.getY() + r * cellH;
        juce::Path wavePath;
        wavePath.startNewSubPath (wireArea.getX(), y);
        for (int c = 0; c < cols; ++c)
        {
            float x = wireArea.getX() + c * cellW;
            float waveY = y + std::sin (juce::MathConstants<float>::pi * 2.0f * c / (float) (cols - 1) + r * 0.3f) * wireArea.getHeight() * 0.12f;
            wavePath.lineTo (x, waveY);
        }
        // Gradiente de cor ao longo das linhas.
        float alpha = 0.3f + 0.4f * (r / (float) (rows - 1));
        g.setColour (PrimaryPurple.withAlpha (alpha));
        g.strokePath (wavePath, juce::PathStrokeType (1.5f));
    }

    // Nós de controle na curva principal (linha do meio).
    int mainRow = rows / 2;
    g.setColour (PrimaryPurpleLight);
    for (int n = 0; n <= 4; ++n)
    {
        float nx = wireArea.getX() + (n / 4.0f) * wireArea.getWidth();
        float ny = wireArea.getCentreY() + std::sin (n * juce::MathConstants<float>::pi / 2.0f) * wireArea.getHeight() * 0.12f;
        g.fillEllipse (nx - 5.0f, ny - 5.0f, 10.0f, 10.0f);
        g.setColour (Border);
        g.drawEllipse (nx - 5.0f, ny - 5.0f, 10.0f, 10.0f, 1.5f);
        g.setColour (PrimaryPurpleLight);
    }

    // Destacar a linha central com uma curva mais grossa.
    juce::Path mainCurve;
    mainCurve.startNewSubPath (wireArea.getX(), wireArea.getCentreY());
    for (int c = 0; c < cols; ++c)
    {
        float x = wireArea.getX() + c * cellW;
        float waveY = wireArea.getCentreY() + std::sin (juce::MathConstants<float>::pi * 2.0f * c / (float) (cols - 1)) * wireArea.getHeight() * 0.12f;
        mainCurve.lineTo (x, waveY);
    }
    g.setColour (PrimaryPurple);
    g.strokePath (mainCurve, juce::PathStrokeType (2.5f));
}

void WavetablePanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    const int gap = 10;

    // Cabeçalho: dropdown + 4 icon buttons.
    int headerH = 36;
    auto headerArea = area.removeFromTop (headerH);

    int selectorW = 150;
    m_wavetableSelector->setBounds (headerArea.removeFromLeft (selectorW));
    headerArea.removeFromLeft (gap);

    int btnW = 32;
    m_wavetableAddButton->setBounds (headerArea.removeFromLeft (btnW));
    headerArea.removeFromLeft (gap / 2);
    m_wavetableRefreshButton->setBounds (headerArea.removeFromLeft (btnW));
    headerArea.removeFromLeft (gap / 2);
    m_wavetableCopyButton->setBounds (headerArea.removeFromLeft (btnW));
    headerArea.removeFromLeft (gap / 2);
    m_wavetableDeleteButton->setBounds (headerArea.removeFromLeft (btnW));

    // Área central (wireframe 3D) — ocupa o meio.
    m_waveformPreview->setBounds (area.removeFromLeft (area.getWidth() * 0.62f));
    area.removeFromLeft (gap);

    // Coluna direita: 3 knobs Morph / Phase / Warp.
    int knobW = 72;
    m_morphKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_phaseKnob->setBounds (area.removeFromLeft (knobW));
    area.removeFromLeft (gap);
    m_warpKnob->setBounds (area.removeFromLeft (knobW));

    // Nota informativa no rodapé.
    m_infoLabel.setBounds (area.removeFromBottom (22));
}

} // namespace gui
} // namespace omnivy
