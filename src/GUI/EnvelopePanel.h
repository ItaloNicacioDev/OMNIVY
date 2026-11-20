#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"
#include "GUI/EnvelopeDisplay.h"
#include "GUI/KnobComponent.h"
#include "GUI/PanelComponent.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// EnvelopePanel
//
// Envelope de amplitude real (ADSR) com quatro knobs ligados aos parâmetros
// envAttack/envDecay/envSustain/envRelease e uma visualização da curva.
// -----------------------------------------------------------------------------
class EnvelopePanel : public PanelComponent,
                      public juce::AudioProcessorValueTreeState::Listener
{
public:
    EnvelopePanel (juce::AudioProcessorValueTreeState& apvts);

    ~EnvelopePanel() override = default;

    void resized() override;

    // juce::AudioProcessorValueTreeState::Listener
    void parameterValueChanged (const juce::String& parameterID, float newValue) override;

private:
    juce::AudioProcessorValueTreeState& m_apvts;

    KnobComponent* m_attackKnob = nullptr;
    KnobComponent* m_decayKnob = nullptr;
    KnobComponent* m_sustainKnob = nullptr;
    KnobComponent* m_releaseKnob = nullptr;
    EnvelopeDisplay* m_envelopeDisplay = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopePanel)
};

} // namespace gui
} // namespace omnivy
