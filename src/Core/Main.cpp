#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

class OmnivyStandaloneApplication : public juce::JUCEApplication
{
public:
    OmnivyStandaloneApplication() {}

    const juce::String getApplicationName() override       { return "Omnivy"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);

        auto* mainWindow = new OmnivyStandaloneWindow ("Omnivy");
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
    }

    class OmnivyStandaloneWindow : public juce::DocumentWindow
    {
    public:
        OmnivyStandaloneWindow (const juce::String& name)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new OmnivyAudioProcessorEditor (*new OmnivyAudioProcessor()), true);

            centreWithSize (getWidth(), getHeight());
            setResizable (true, false);
        }

        ~OmnivyStandaloneWindow() {}

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
};

START_JUCE_APPLICATION (OmnivyStandaloneApplication)
