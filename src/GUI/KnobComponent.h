#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// LookAndFeel que desenha nada para o slider interno do KnobComponent.
// O slider é usado apenas como suporte de valor e conduto do SliderAttachment;
// todo o desenho é feito pelo KnobComponent.
// -----------------------------------------------------------------------------
class TransparentSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        juce::ignoreUnused (x, y, width, height, sliderPos, rotaryStartAngle, rotaryEndAngle);
    }
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        juce::ignoreUnused (x, y, width, height, sliderPos, minSliderPos, maxSliderPos);
    }
};

// -----------------------------------------------------------------------------
// KnobComponent
//
// Botão circular com trilho e arco de valor em gradiente roxo/azul.
// Conecta-se ao parâmetro via AudioProcessorValueTreeState::SliderAttachment.
// Três tamanhos: small (36 px), medium (52 px), large (72 px).
//
// O arco percorre ~270 graus. Double-click reseta para o valor padrão do parâmetro.
// -----------------------------------------------------------------------------
class KnobComponent : public juce::Component
{
public:
    enum class Size
    {
        Small,
        Medium,
        Large
    };

    KnobComponent (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& parameterID,
                   const juce::String& label,
                   const juce::String& unit = {},
                   Size knobSize = Size::Medium,
                   juce::Colour accent = PrimaryPurple);

    ~KnobComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Retorna o valor atual do parâmetro (útil para leituras).
    float getValue() const noexcept;

private:
    // Slider funcional (invisível) que conduz o SliderAttachment e recebe
    // interações de mouse (arrasto vertical / rotativo).
    TransparentSliderLookAndFeel m_transparentLA;
    juce::Slider m_slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_attachment;

    Size m_size;
    juce::Colour m_accent;
    juce::String m_label;
    juce::String m_unit;
    float m_defaultValue = 0.0f;

    int knobDiameter() const noexcept;

    float angleForValue (float value) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobComponent)
};

} // namespace gui
} // namespace omnivy
