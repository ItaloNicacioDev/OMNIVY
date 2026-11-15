#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class OmnivyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OmnivyAudioProcessorEditor (OmnivyAudioProcessor&);
    ~OmnivyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OmnivyAudioProcessor& audioProcessor;

    juce::Slider testSlider;
    juce::Label testLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyAudioProcessorEditor)
};
