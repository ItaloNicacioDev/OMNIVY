#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.h>
#include <juce_audio_utils/gui/juce_KeyboardComponentBase.h>
#include <juce_audio_utils/gui/juce_MidiKeyboardComponent.h>
#include <fstream>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static void logToFile (const juce::String& message)
{
    std::ofstream logFile ("OmnivyStandalone_log.txt", std::ios::app);
    logFile << message.toStdString() << "\n";
    logFile.close();
}

class OmnivyStandaloneApplication : public juce::JUCEApplication
{
public:
    OmnivyStandaloneApplication() {}

    const juce::String getApplicationName() override       { return "Omnivy"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
        logToFile ("Omnivy: application initialise()");

        mainWindow = std::make_unique<OmnivyStandaloneWindow> ("Omnivy");
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        logToFile ("Omnivy: application shutdown()");
        mainWindow = nullptr; // destrói a janela, que libera o áudio
    }

    class OmnivyStandaloneWindow : public juce::DocumentWindow,
                                   public juce::AudioIODeviceCallback
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

            // --- Audio: inicializa o device manager (0 entrada, 2 saídas estéreo) ---
            audioDeviceManager.initialise (0, 2, nullptr, true, {});
            logToFile ("Omnivy: audioDeviceManager.initialise(0 in, 2 out) called");

            // Registra a janela como callback de áudio (chama audioDeviceIOCallback na thread de áudio).
            audioDeviceManager.addAudioCallback (this);

            // Registra listener do teclado virtual (injeta Note On/Off no coletor de MIDI).
            midiKeyboardState.addListener (&midiCollector);

            // --- Processor (mesmo do VST3; nenhum novo motor DSP) ---
            processor = std::make_unique<OmnivyAudioProcessor>();

            // --- Editor ---
            editor = std::make_unique<OmnivyAudioProcessorEditor> (*processor);
            editor->setSize (600, 400);

            // --- Seletor de entrada MIDI ---
            midiDevices = juce::MidiInput::getAvailableDevices();
            juce::StringArray deviceNames;
            for (const auto& d : midiDevices)
                deviceNames.add (d.name);
            midiInputSelector.addItemList (deviceNames, 1);
            if (midiDevices.size() > 0)
            {
                midiInputSelector.setSelectedId (1);
                selectMidiInput (midiInputSelector.getSelectedItemIndex());
            }
            midiInputSelector.onChange = [this] { selectMidiInput (midiInputSelector.getSelectedItemIndex()); };
            addAndMakeVisible (midiInputSelector);

            // --- Container com editor + seletor MIDI + botão de configuração de áudio ---
            auto* container = new ContentContainer (*editor, midiInputSelector, configButton, midiKeyboardComponent);
            container->setSize (600, 460);

            configButton.setButtonText ("Audio Setup...");
            configButton.onClick = [this] { showAudioDeviceDialog(); };

            setContentOwned (container, true);
            setSize (600, 460);

            centreWithSize (getWidth(), getHeight());
            logToFile ("Omnivy: window created and visible");
        }

        ~OmnivyStandaloneWindow()
        {
            logToFile ("Omnivy: ~OmnivyStandaloneWindow()");
            // Remove os callbacks de áudio e MIDI primeiro (para o callback de áudio,
            // que chama releaseResources no processador), depois destrói editor/processor.
            audioDeviceManager.removeAudioCallback (this);
            audioDeviceManager.removeMidiInputCallback (midiInputSelector.getText().isNotEmpty()
                                                            ? midiInputSelector.getText()
                                                            : juce::String(), nullptr);
            editor = nullptr;
            processor = nullptr;
        }

        void closeButtonPressed() override
        {
            logToFile ("Omnivy: closeButtonPressed()");
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void resized() override
        {
            if (auto* c = getContentComponent())
                c->setBounds (getLocalBounds());
        }

        // --- juce::AudioIODeviceCallback (chamado pela thread de áudio JUCE) ---

        void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                               int numInputChannels,
                                               float* const* outputChannelData,
                                               int numOutputChannels,
                                               int numSamples,
                                               const juce::AudioIODeviceCallbackContext&) override
        {
            juce::AudioBuffer<float> buffer (outputChannelData, numOutputChannels, numSamples);
            juce::MidiBuffer midiBuffer;

            // Drena o coletor de MIDI externo (preenchido na thread de mensagem
            // por addMidiInputCallback) para um MidiBuffer local, sem alocação.
            midiCollector.removeNextBlockOfMessages (midiBuffer, numSamples);

            // --- Processamento: mesma pipeline do VST3 (prepareToPlay/processBlock) ---
            processor->processBlock (buffer, midiBuffer);
        }

        void audioDeviceAboutToStart (juce::AudioIODevice* device) override
        {
            juce::ignoreUnused (device);
            logToFile ("Omnivy: audioDeviceAboutToStart()");
            processor->prepareToPlay (audioDeviceManager.getCurrentAudioDevice()->getCurrentSampleRate(),
                                      audioDeviceManager.getCurrentAudioDevice()->getCurrentBufferSizeSamples());
            midiCollector.reset (audioDeviceManager.getCurrentAudioDevice()->getCurrentSampleRate());
        }

        void audioDeviceStopped() override
        {
            logToFile ("Omnivy: audioDeviceStopped()");
            processor->releaseResources();
        }

    private:
        void selectMidiInput (int index)
        {
            if (index < 0 || index >= (int) midiDevices.size())
                return;

            const auto& deviceInfo = midiDevices [(std::size_t) index];
            const auto deviceName = deviceInfo.identifier;

            // Remove callback anterior (se houver).
            audioDeviceManager.removeMidiInputCallback (midiDevices.isEmpty()
                                                            ? juce::String()
                                                            : previousMidiDeviceIdentifier, nullptr);

            // Registra o coletor para o novo dispositivo MIDI (thread de mensagem).
            if (deviceName.isNotEmpty())
                audioDeviceManager.addMidiInputCallback (deviceName, &midiCollector);

            previousMidiDeviceIdentifier = deviceName;
            midiInputSelector.setSelectedId (index + 1);
        }
        class ContentContainer : public juce::Component
        {
        public:
            ContentContainer (juce::AudioProcessorEditor& ed,
                              juce::ComboBox& midiSel,
                              juce::TextButton& btn,
                              juce::MidiKeyboardComponent& keybd)
                : editor (ed), midiSelector (midiSel), button (btn), keyboard (keybd)
            {
                addAndMakeVisible (editor);
                addAndMakeVisible (midiSelector);
                addAndMakeVisible (button);
                addAndMakeVisible (keyboard);
            }

            void resized() override
            {
                auto area = getLocalBounds();
                editor.setBounds (area.removeFromBottom (340));
                area.removeFromTop (10);
                midiSelector.setBounds (10, 350, 220, 24);
                button.setBounds (240, 350, 200, 24);
                keyboard.setBounds (10, 384, getWidth() - 20, 66);
            }

        private:
            juce::AudioProcessorEditor& editor;
            juce::ComboBox& midiSelector;
            juce::TextButton& button;
            juce::MidiKeyboardComponent& keyboard;

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
        std::unique_ptr<OmnivyAudioProcessorEditor> editor;
        std::unique_ptr<AudioDeviceDialog> audioDeviceDialog;
        juce::ComboBox midiInputSelector;
        juce::TextButton configButton;
        juce::MidiMessageCollector midiCollector;
        juce::Array<juce::MidiDeviceInfo> midiDevices;
        juce::String previousMidiDeviceIdentifier;
        juce::MidiKeyboardState midiKeyboardState;
        juce::MidiKeyboardComponent midiKeyboardComponent { midiKeyboardState, juce::KeyboardComponentBase::horizontalKeyboard };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmnivyStandaloneWindow)
    };

    std::unique_ptr<OmnivyStandaloneWindow> mainWindow;
};

START_JUCE_APPLICATION (OmnivyStandaloneApplication)
