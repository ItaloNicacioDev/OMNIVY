#include "GUI/OscillatorPanel.h"

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& apvts)
    : PanelComponent ("Osciladores", false)
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
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (44); // espaço para o título do painel (desenhado por PanelComponent)

    int rowHeight = 56;
    int gap = 8;
    int totalH = omnivy::params::NumOscillators * rowHeight + (omnivy::params::NumOscillators - 1) * gap;

    // Centraliza verticalmente se houver sobra.
    int y = area.getY();
    if (area.getHeight() > totalH)
        y += (area.getHeight() - totalH) / 2;

    for (int i = 0; i < omnivy::params::NumOscillators; ++i)
    {
        m_rows[i]->setBounds (area.getX(), y, area.getWidth(), rowHeight);
        y += rowHeight + gap;
    }
}

} // namespace gui
} // namespace omnivy
