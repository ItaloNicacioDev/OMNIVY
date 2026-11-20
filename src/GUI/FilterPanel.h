#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/KnobComponent.h"
#include "GUI/PanelComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// FilterPanel
//
// Filtro real (enabled, type, cutoff, resonance, keyTrack) com gráfico de
// resposta esboçado a partir dos parâmetros reais (não dados fictícios).
// -----------------------------------------------------------------------------
class FilterPanel : public PanelComponent
{
public:
    FilterPanel (juce::AudioProcessorValueTreeState& apvts);

    ~FilterPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& m_apvts;

    IconButtonComponent* m_enabledButton = nullptr;
    juce::ComboBox* m_typeCombo = nullptr;
    KnobComponent* m_cutoffKnob = nullptr;
    KnobComponent* m_resonanceKnob = nullptr;
    KnobComponent* m_keyTrackKnob = nullptr;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_enabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_typeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};

} // namespace gui
} // namespace omnivy
