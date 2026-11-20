#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/OscillatorRow.h"
#include "GUI/PanelComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// OscillatorPanel
//
// Exibe os seis slots de oscilador. Cada linha controla um parâmetro real
// (active, engine, waveform, level, octave, semitone). Selecionar uma linha
// destaca-a visualmente (estado da UI apenas).
// -----------------------------------------------------------------------------
class OscillatorPanel : public PanelComponent
{
public:
    OscillatorPanel (juce::AudioProcessorValueTreeState& apvts);

    ~OscillatorPanel() override = default;

    void resized() override;

    int getSelectedOscillator() const noexcept { return m_selectedOscillator; }

private:
    OscillatorRowComponent* m_rows[params::NumOscillators] = { nullptr };
    int m_selectedOscillator = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};

} // namespace gui
} // namespace omnivy
