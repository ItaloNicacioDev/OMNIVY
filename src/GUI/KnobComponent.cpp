#include "GUI/KnobComponent.h"

#include <cmath>

namespace omnivy {
namespace gui {

namespace
{
constexpr float Pi = (float) juce::MathConstants<double>::pi;

float startAngleRad() { return 3.0f * Pi / 4.0f; }      // 135° (canto superior-esquerdo)
float sweepAngleRad() { return 3.0f * Pi / 2.0f; }      // 270°
float endAngleRad()   { return startAngleRad() + sweepAngleRad(); }  // 405° = 45°
}

KnobComponent::KnobComponent (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& parameterID,
                              const juce::String& label,
                              const juce::String& unit,
                              Size knobSize,
                              juce::Colour accent)
    : m_size (knobSize), m_accent (accent), m_label (label), m_unit (unit)
{
    setWantsKeyboardFocus (false);
    setInterceptsMouseClicks (true);

    // Slider funcional, invisível. Conduz o SliderAttachment e recebe o arrasto do mouse.
    m_slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    m_slider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    m_slider.setLookAndFeel (&m_transparentLA);
    m_slider.setDoubleClickReturnValue (true, 0.0f);
    m_slider.onValueChange = [this] { repaint(); };

    // Lê o intervalo e o valor padrão diretamente do parâmetro do APVTS, para que
    // o knob nunca apresente um intervalo fictício.
    if (auto* param = apvts.getParameter (parameterID))
    {
        auto normalisable = param->getNormalisableRange();
        m_slider.setRange (normalisable.start, normalisable.end, normalisable.interval);
        m_defaultValue = param->getDefaultValue();
        m_slider.setValue (m_defaultValue, juce::dontSendNotification);
        m_slider.setDoubleClickReturnValue (true, m_defaultValue);
    }
    else
    {
        m_slider.setRange (0.0, 1.0, 0.01);
        m_defaultValue = 0.0f;
    }

    m_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, parameterID, m_slider);

    addAndMakeVisible (m_slider);
}

KnobComponent::~KnobComponent() = default;

int KnobComponent::knobDiameter() const noexcept
{
    switch (m_size)
    {
        case Size::Small:   return KnobSizeSmall;
        case Size::Medium:  return KnobSizeMedium;
        case Size::Large:   return KnobSizeLarge;
    }
    return KnobSizeMedium;
}

float KnobComponent::angleForValue (float value) const noexcept
{
    auto minV = (float) m_slider.getRange().getStart();
    auto maxV = (float) m_slider.getRange().getEnd();
    auto span = maxV - minV;
    if (span <= 0.0f)
        return startAngleRad();
    auto prop = juce::jlimit (0.0f, 1.0f, (value - minV) / span);
    return startAngleRad() + prop * sweepAngleRad();
}

float KnobComponent::getValue() const noexcept
{
    return static_cast<float> (m_slider.getValue());
}

void KnobComponent::resized()
{
    m_slider.setBounds (getLocalBounds());
}

void KnobComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();
    int d = knobDiameter();
    float radius = d * 0.5f;

    // Área do círculo do knob (centralizada, deixando espaço para rótulo/valor).
    float knobRadius = radius * 0.72f;
    juce::Rectangle<float> knobRect (centre.getX() - knobRadius, centre.getY() - knobRadius,
                                     knobRadius * 2.0f, knobRadius * 2.0f);
    auto kc = knobRect.getCentre();

    // Valor atual e proporção no arco.
    float value = getValue();
    float minV = (float) m_slider.getRange().getStart();
    float maxV = (float) m_slider.getRange().getEnd();
    float span = maxV - minV;
    float prop = span > 0.0f ? juce::jlimit (0.0f, 1.0f, (value - minV) / span) : 0.0f;

    // ----- Rótulo (acima) -----
    if (m_size != Size::Small && m_label.isNotEmpty())
    {
        g.setColour (TextSecondary);
        g.setFont (uiFont (FontSizeMicro));
        g.drawText (m_label, (int) knobRect.getX(), 2, (int) knobRect.getWidth(), 14,
                    juce::Justification::centred, true);
    }

    // ----- Corpo circular escuro -----
    g.setColour (ControlSurface);
    g.fillEllipse (knobRect);
    g.setColour (Border);
    g.drawEllipse (knobRect, 1.0f);

    // ----- Trilho do arco (círculo completo de 270°, discreto) -----
    {
        juce::Path track;
        track.addArc (knobRect.getX(), knobRect.getY(), knobRect.getWidth(), knobRect.getHeight(),
                      startAngleRad(), endAngleRad());
        g.setColour (Border.brighter (0.2f));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ----- Arco de valor (gradiente roxo/azul aproximado com brilho) -----
    if (prop > 0.005f)
    {
        float endAngle = startAngleRad() + prop * sweepAngleRad();

        // Brilho externo.
        juce::Path glowPath;
        glowPath.addArc (knobRect.getX(), knobRect.getY(), knobRect.getWidth(), knobRect.getHeight(),
                         startAngleRad(), endAngle);
        g.setColour (m_accent.withAlpha (0.35f));
        g.strokePath (glowPath, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arco sólido primário.
        juce::Path valuePath;
        valuePath.addArc (knobRect.getX(), knobRect.getY(), knobRect.getWidth(), knobRect.getHeight(),
                          startAngleRad(), endAngle);
        g.setColour (m_accent);
        g.strokePath (valuePath, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ----- Indicador branco (linha do centro até o arco) -----
    {
        float angle = angleForValue (value);
        float indicatorLen = knobRadius * 0.78f;
        float endX = kc.getX() + std::cos (angle) * indicatorLen;
        float endY = kc.getY() + std::sin (angle) * indicatorLen;
        g.setColour (juce::Colours::white);
        g.drawLine (kc.getX(), kc.getY(), endX, endY, 2.2f);

        // Ponto central.
        g.setColour (m_accent);
        g.fillEllipse (kc.getX() - 3.0f, kc.getY() - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white);
        g.drawEllipse (kc.getX() - 3.0f, kc.getY() - 3.0f, 6.0f, 6.0f, 1.0f);
    }

    // ----- Valor numérico (abaixo) -----
    if (m_size != Size::Small)
    {
        juce::String text;
        if (m_slider.getRange().getInterval() >= 1.0f)
            text = juce::String ((int) value);
        else
            text = juce::String (value, 2);
        if (m_unit.isNotEmpty())
            text += " " + m_unit;

        g.setColour (TextMain);
        g.setFont (uiFont (FontSizeValue, juce::Font::bold));
        g.drawText (text, (int) knobRect.getX(), (int) (bounds.getBottom() - 14),
                    (int) knobRect.getWidth(), 14, juce::Justification::centred, true);
    }
}

} // namespace gui
} // namespace omnivy
