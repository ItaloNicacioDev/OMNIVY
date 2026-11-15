#include "PluginEditor.h"

OmnivyAudioProcessorEditor::OmnivyAudioProcessorEditor (OmnivyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 400);

    testSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    testSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
    testSlider.setRange (0.0, 1.0, 0.01);
    testSlider.setValue (audioProcessor.testParameter);
    testSlider.onValueChange = [this]()
    {
        audioProcessor.testParameter = static_cast<float> (testSlider.getValue());
    };
    addAndMakeVisible (testSlider);

    testLabel.setText ("OMNIVY Test Parameter", juce::dontSendNotification);
    testLabel.setJustificationType (juce::Justification::centred);
    testLabel.setFont (juce::Font (24.0f, juce::Font::bold));
    addAndMakeVisible (testLabel);
}

OmnivyAudioProcessorEditor::~OmnivyAudioProcessorEditor()
{
}

void OmnivyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (32.0f, juce::Font::bold));
    g.drawText ("OMNIVY", getLocalBounds(), juce::Justification::centred, true);

    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::Font (16.0f));
    g.drawText ("Beyond Synthesis — Phase 1 Build",
                getLocalBounds().withTrimmedTop (60),
                juce::Justification::centred, true);

    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 2);
}

void OmnivyAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (40);
    testLabel.setBounds (area.removeFromTop (40));
    area.removeFromTop (10);
    testSlider.setBounds (area.removeFromTop (40));
}
