#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "GUI/DesignSystem.h"
#include "GUI/KnobComponent.h"
#include "GUI/PanelComponent.h"
#include "GUI/WaveformComponent.h"
#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

// -----------------------------------------------------------------------------
// OscillatorRowComponent
//
// Uma linha de oscilador (OSC 1..6). Cada controle escreve no parâmetro real
// do APVTS (oscActive, oscWaveform, oscLevel, oscOctave, oscSemitone,
// oscEngine) e o estado visual reflete o parâmetro via attachments.
// -----------------------------------------------------------------------------
class OscillatorRowComponent : public juce::Component,
                               public juce::ComboBox::Listener
{
public:
    OscillatorRowComponent (juce::AudioProcessorValueTreeState& apvts,
                            int oscillatorIndex,   // 0..5
                            bool selectable = true);

    ~OscillatorRowComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    int getOscillatorIndex() const noexcept { return m_oscillatorIndex; }
    bool isSelected() const noexcept { return m_selected; }
    void setSelected (bool selected);

    // ComboBox::Listener — mantém a pré-visualização sincronizada com o tipo.
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

private:
    juce::AudioProcessorValueTreeState& m_apvts;
    int m_oscillatorIndex = 0;
    bool m_selected = false;

    // Faixa lateral colorida.
    juce::Colour m_stripeColour;

    // Controles (todos ligados a parâmetros reais via attachments).
    IconButtonComponent* m_powerButton = nullptr;
    juce::ComboBox* m_engineCombo = nullptr;
    juce::ComboBox* m_waveformCombo = nullptr;
    WaveformComponent* m_waveformPreview = nullptr;
    KnobComponent* m_levelKnob = nullptr;
    KnobComponent* m_octaveKnob = nullptr;
    KnobComponent* m_semitoneKnob = nullptr;
    juce::TextButton* m_chevronButton = nullptr;

    // Labels.
    juce::Label m_numberLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_engineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_waveformAttachment;

    void populateEngineCombo();
    void populateWaveformCombo();
    void updateSelectionVisuals();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorRowComponent)
};

} // namespace gui
} // namespace omnivy
