// Teste instrumentado JUCE - verificação de HWND e ComponentPeer
// Executar como Standalone temporário para diagnóstico
// Compilar junto com o projeto Omnivy, substituindo Main.cpp temporariamente

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

class InstrumentedWindow : public juce::DocumentWindow
{
public:
    InstrumentedWindow()
        : DocumentWindow ("Instrumented",
                          juce::Desktop::getInstance().getDefaultLookAndFeel()
                              .findColour (juce::ResizableWindow::backgroundColourId),
                          DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setSize (800, 600);
        centreWithSize (800, 600);

        // Verificar HWND após construção
        if (auto* peer = getComponentPeer())
        {
            void* hwnd = peer->getNativeHandle();
            juce::String debugMsg;
            debugMsg << "InstrumentedWindow: ComponentPeer criado, HWND = " << hwnd;
            juce::Logger::writeToLog (debugMsg);
            
            // Verificar com Win32 API
            #if JUCE_WINDOWS
            HWND winHwnd = (HWND) hwnd;
            juce::String visibleMsg;
            visibleMsg << "IsWindow: " << ::IsWindow(winHwnd)
                       << ", IsWindowVisible: " << ::IsWindowVisible(winHwnd)
                       << ", GetLastError: " << ::GetLastError();
            juce::Logger::writeToLog (visibleMsg);
            #endif
        }
        else
        {
            juce::Logger::writeToLog ("InstrumentedWindow: ERRO - ComponentPeer NULO!");
        }

        setVisible (true);

        // Verificar HWND após setVisible
        if (auto* peer = getComponentPeer())
        {
            void* hwnd = peer->getNativeHandle();
            juce::String debugMsg;
            debugMsg << "InstrumentedWindow: APÓS setVisible - HWND = " << hwnd;
            juce::Logger::writeToLog (debugMsg);
            
            #if JUCE_WINDOWS
            HWND winHwnd = (HWND) hwnd;
            juce::String visibleMsg;
            visibleMsg << "APÓS setVisible - IsWindow: " << ::IsWindow(winHwnd)
                       << ", IsWindowVisible: " << ::IsWindowVisible(winHwnd);
            juce::Logger::writeToLog (visibleMsg);
            #endif
        }

        // Tentar encontrar a janela via EnumWindows
        #if JUCE_WINDOWS
        juce::Logger::writeToLog ("InstrumentedWindow: Iniciando EnumWindows...");
        ::EnumWindows ([](HWND hwnd, LPARAM lParam) -> BOOL {
            char title[256] = {};
            ::GetWindowTextA (hwnd, title, sizeof (title));
            if (title[0] != '\0')
            {
                juce::String debugMsg;
                debugMsg << "EnumWindows: HWND=" << hwnd << " title='" << title << "'";
                juce::Logger::writeToLog (debugMsg);
            }
            return TRUE;
        }, 0);
        juce::Logger::writeToLog ("InstrumentedWindow: EnumWindows concluído.");
        #endif
    }

    ~InstrumentedWindow() override = default;

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class InstrumentedApp : public juce::JUCEApplication
{
public:
    InstrumentedApp() {}
    const juce::String getApplicationName() override { return "Instrumented"; }
    const juce::String getApplicationVersion() override { return "1.0"; }

    void initialise (const juce::String&) override
    {
        juce::Logger::writeToLog ("InstrumentedApp: initialise() chamado");
        juce::Logger::writeToLog ("InstrumentedApp: MessageManager instance: " 
                                  + juce::String ((void*) juce::MessageManager::getInstance()));
        
        // Verificar se estamos na thread de mensagem
        juce::Logger::writeToLog ("InstrumentedApp: IsMessageThread: " 
                                  + juce::String (juce::MessageManager::getInstance()->isThisTheMessageThread()));

        window = std::make_unique<InstrumentedWindow>();
        window->setVisible (true);
        
        juce::Logger::writeToLog ("InstrumentedApp: Janela criada e setVisible chamado");
    }

    void shutdown() override { window = nullptr; }

private:
    std::unique_ptr<InstrumentedWindow> window;
};

START_JUCE_APPLICATION (InstrumentedApp)
