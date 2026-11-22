#include "GUI/OscillatorPanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts)
    : PanelComponent ("Oscillators", false)
{
    for (int i = 0; i < omnivy::params::NumOscillators; ++i)
    {
        m_rows[i] = new OscillatorRowComponent (apvts, i, true);
        addAndMakeVisible (*m_rows[i]);
    }
    m_rows[0]->setSelected (true);
    m_selectedOscillator = 0;
}

void OscillatorPanel::resized()
{
    auto area = getLocalBounds().toFloat();
    float sx = getWidth() / 1672.0f;
    float sy = getHeight() / 941.0f;

    area = area.reduced (10.0f * sx, 0);
    area.removeFromTop ((int) (44.0f * sy)); // espaço para o título do painel

    int rowHeight = (int) (56.0f * sy);
    int gap = (int) (8.0f * sy);
    int totalH = omnivy::params::NumOscillators * rowHeight + (omnivy::params::NumOscillators - 1) * gap;

    // Centraliza verticalmente se houver sobra.
    int y = (int) area.getY();
    if (area.getHeight() > totalH)
        y += (int) ((area.getHeight() - totalH) / 2.0f);

    for (int i = 0; i < omnivy::params::NumOscillators; ++i)
    {
        m_rows[i]->setBounds ((int) area.getX(), y, (int) area.getWidth(), rowHeight);
        y += rowHeight + gap;
    }
}

} // namespace gui
} // namespace omnivy
