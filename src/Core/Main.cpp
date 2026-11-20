#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/players/juce_AudioProcessorPlayer.h>
#include <juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.h>
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

        mainWindow = std::make_unique<OmnivyStandaloneWindow> ("Omnivy");
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        mainWindow = nullptr; // destrói a janela, que libera o áudio
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
            setResizable (true, false);

            // --- Áudio: inicializa o device manager (0 entrada, 2 saídas estéreo) ---
            audioDeviceManager.initialise (0, 2, nullptr, true, {});

            // --- Processor (mesmo do VST3; nenhum novo motor DSP) ---
            processor = std::make_unique<OmnivyAudioProcessor>();

            // --- Player: aciona prepareToPlay/processBlock/releaseResources no callback de áudio ---
            player = std::make_unique<juce::AudioProcessorPlayer>();
            player->setProcessor (processor.get());

            // --- Editor ---
            editor = std::make_unique<OmnivyAudioProcessorEditor> (*processor);
            editor->setSize (600, 400);

            // --- Container com editor + botão de configuração de áudio ---
            auto* container = new ContentContainer (*editor, configButton);
            container->setSize (600, 440);

            configButton.setButtonText ("Configurar Áudio...");
            configButton.onClick = [this] { showAudioDeviceDialog(); };

            setContentOwned (container, true);
            setSize (600, 440);

            centreWithSize (getWidth(), getHeight());
        }

        ~OmnivyStandaloneWindow()
        {
            // Para o callback de áudio (chama releaseResources) antes de destruir
            // editor/processor.
            player = nullptr;
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void resized() override
        {
            if (auto* c = getContentComponent())
                c->setBounds (getLocalBounds());
        }

    private:
        class ContentContainer : public juce::Component
        {
        public:
            ContentContainer (juce::AudioProcessorEditor& ed, juce::TextButton& btn)
                : editor (ed), button (btn)
            {
                addAndMakeVisible (editor);
                addAndMakeVisible (button);
            }

            void resized() override
            {
                auto area = getLocalBounds();
                editor.setBounds (area.removeFromBottom (400));
                button.setBounds (10, 405, 200, 30);
            }

        private:
            juce::AudioProcessorEditor& editor;
            juce::TextButton& button;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentContainer)
        };

        class AudioDeviceDialog : public juce::DialogWindow
        {
        public:
            AudioDeviceDialog (juce::AudioDeviceManager& dm, OmnivyStandaloneWindow& ownerWindow)
                : DialogWindow ("Configurar Dispositivo de Áudio",
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                true),
                  owner (ownerWindow)
            {
                auto* selector = new juce::AudioDeviceSelectorComponent (dm,
                                                                         0, 256, // min/max inputs
                                                                         0, 256, // min/max outputs
                                                                         true,   // show MIDI inputs
                                                                         true,   // show MIDI outputs
                                                                         true,   // show channels
                                                                         true);  // show sample rates
                selector->setSize (520, 480);
                setContentOwned (selector, true);
                setResizable (true, false);
                centreWithSize (520, 480);
                setVisible (true);
            }

            void closeButtonPressed() override
            {
                owner.audioDeviceDialog = nullptr;
                setVisible (false);
            }

        private:
            OmnivyStandaloneWindow& owner;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceDialog)
        };

        void showAudioDeviceDialog()
        {
            if (audioDeviceDialog != nullptr)
            {
                audioDeviceDialog->toFront (true);
                return;
            }
            audioDeviceDialog = std::make_unique<AudioDeviceDialog> (audioDeviceManager, *this);
        }

        juce::AudioDeviceManager audioDeviceManager;
        std::unique_ptr<OmnivyAudioProcessor> processor;
        std::unique_ptr<juce::AudioProcessorPlayer> player;
        std::unique_ptr<OmnivyAudioProcessorEditor> editor;
        std::unique_ptr<AudioDeviceDialog> audioDeviceDialog;
        juce::TextButton configButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyStandaloneWindow)
    };

    std::unique_ptr<OmnivyStandaloneWindow> mainWindow;
};

START_JUCE_APPLICATION (OmnivyStandaloneApplication)
