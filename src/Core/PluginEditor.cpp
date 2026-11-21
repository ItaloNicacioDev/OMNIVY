#include "PluginEditor.h"

#include "PluginProcessor.h"

#include "GUI/DesignSystem.h"
#include "GUI/OscillatorPanel.h"
#include "GUI/WavetablePanel.h"
#include "GUI/RoutingPanel.h"
#include "GUI/FilterPanel.h"
#include "GUI/EnvelopePanel.h"
#include "GUI/VisualizerPanel.h"
#include "GUI/EnvelopeDisplay.h"
#include "GUI/WaveformComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/gui/juce_KeyboardComponentBase.h>
#include <juce_audio_utils/gui/juce_MidiKeyboardComponent.h>

#include <cmath>
#include <memory>
#include <random>

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

// ============================================================================
// Faixa 1 — TopBarComponent
// Cabeçalho: logo com waveform, nome "OMNIVY", tagline "The Ultimate Synthesizer",
// navegação de presets, favorito, salvar/excluir/desfazer/refazer/randomize,
// poly/voices, settings.
// ============================================================================
class TopBarComponent : public juce::Component, private juce::Timer
{
public:
    TopBarComponent (OmnivyAudioProcessor& processor)
        : m_processor (processor)
    {
        setInterceptsMouseClicks (true, true);

        // --- Lado esquerdo: logotipo ---------------------------------------
        m_logoWaveform.setColour (PrimaryPurple);
        m_logoWaveform.setSizeHint (200, 48);
        addAndMakeVisible (m_logoWaveform);

        m_wordmark.setText ("OMNIVY", juce::dontSendNotification);
        m_wordmark.setFont (uiFont (42.0f, juce::Font::bold));
        m_wordmark.setColour (juce::Label::textColourId, TextMain);
        m_wordmark.setJustificationType (juce::Justification::left);
        m_wordmark.setEnabled (false);
        addAndMakeVisible (m_wordmark);

        m_tagline.setText ("The Ultimate Synthesizer", juce::dontSendNotification);
        m_tagline.setFont (uiFont (16.0f));
        m_tagline.setColour (juce::Label::textColourId, TextSecondary);
        m_tagline.setJustificationType (juce::Justification::left);
        m_tagline.setEnabled (false);
        addAndMakeVisible (m_tagline);

        // --- Centro: navegação de presets ----------------------------------
        m_presetPrev.setButtonText ("‹");
        m_presetPrev.setEnabled (false);
        m_presetPrev.setTooltip ("Nenhum sistema de presets implementado ainda.");
        m_presetPrev.setClickingTogglesState (false);
        addAndMakeVisible (m_presetPrev);

        m_presetName.setText ("Atmospheric Dreamscape", juce::dontSendNotification);
        m_presetName.setFont (uiFont (18.0f, juce::Font::bold));
        m_presetName.setColour (juce::Label::textColourId, TextMain);
        m_presetName.setJustificationType (juce::Justification::centred);
        m_presetName.setEnabled (false);
        addAndMakeVisible (m_presetName);

        m_presetNext.setButtonText ("›");
        m_presetNext.setEnabled (false);
        m_presetNext.setTooltip ("Nenhum sistema de presets implementado ainda.");
        m_presetNext.setClickingTogglesState (false);
        addAndMakeVisible (m_presetNext);

        m_favorite.setToggleState (false, juce::dontSendNotification);
        m_favorite.setTooltip ("Favorito (apenas esta sessão).");
        addAndMakeVisible (m_favorite);

        m_save.setButtonText ("💾");
        m_save.setTooltip ("Salva o estado atual dos parâmetros.");
        m_save.onClick = [this] { savePreset(); };
        addAndMakeVisible (m_save);

        m_delete.setButtonText ("🗑");
        m_delete.setEnabled (false);
        m_delete.setTooltip ("Nenhum preset para excluir.");
        addAndMakeVisible (m_delete);

        m_undo.setButtonText ("↩");
        m_undo.setEnabled (false);
        m_undo.setTooltip ("Undo não suportado nesta versão.");
        addAndMakeVisible (m_undo);

        m_redo.setButtonText ("↪");
        m_redo.setEnabled (false);
        m_redo.setTooltip ("Redo não suportado nesta versão.");
        addAndMakeVisible (m_redo);

        m_randomize.setButtonText ("🎲");
        m_randomize.setTooltip ("Atribui valores válidos aleatórios aos parâmetros.");
        m_randomize.onClick = [this] { randomize(); };
        addAndMakeVisible (m_randomize);

        // --- Lado direito: indicadores e settings --------------------------
        m_polyphonyLabel.setText ("Poly: 16", juce::dontSendNotification);
        m_polyphonyLabel.setFont (uiFont (13.0f));
        m_polyphonyLabel.setColour (juce::Label::textColourId, TextSecondary);
        m_polyphonyLabel.setJustificationType (juce::Justification::right);
        m_polyphonyLabel.setEnabled (false);
        addAndMakeVisible (m_polyphonyLabel);

        m_voicesLabel.setText ("Voices: 0", juce::dontSendNotification);
        m_voicesLabel.setFont (uiFont (13.0f));
        m_voicesLabel.setColour (juce::Label::textColourId, TextMuted);
        m_voicesLabel.setJustificationType (juce::Justification::right);
        m_voicesLabel.setEnabled (false);
        addAndMakeVisible (m_voicesLabel);

        m_settings.setButtonText ("⚙");
        m_settings.setTooltip ("Abre informações de build.");
        m_settings.onClick = [this] { showSettings(); };
        addAndMakeVisible (m_settings);

        startTimerHz (15);
    }

    ~TopBarComponent() override = default;

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (BackgroundMain);
        g.fillRect (area);
        g.setColour (Border);
        g.fillRect (0, (int) getHeight() - 1, getWidth(), 1);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        const int gap = 12;

        // Esquerda: logo + nome + tagline (empilhados verticalmente).
        int logoW = 200;
        m_logoWaveform.setBounds (area.removeFromLeft (logoW));
        m_wordmark.setBounds (area.removeFromLeft (logoW));
        m_tagline.setBounds (area.removeFromLeft (logoW));
        area.removeFromLeft (gap);

        // Centro: navegação de preset com ‹ nome ★.
        int btnW = 32;
        int halfGap = gap / 2;
        m_presetPrev.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (halfGap);
        int presetW = 240;
        m_presetName.setBounds (area.removeFromLeft (presetW));
        area.removeFromLeft (halfGap);
        m_presetNext.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap);

        m_favorite.setBounds (area.removeFromLeft (28));
        area.removeFromLeft (halfGap);

        // Botões de ação como ícones.
        int iconBtnW = 32;
        m_save.setBounds (area.removeFromLeft (iconBtnW));
        area.removeFromLeft (halfGap);
        m_delete.setBounds (area.removeFromLeft (iconBtnW));
        area.removeFromLeft (halfGap);
        m_undo.setBounds (area.removeFromLeft (iconBtnW));
        area.removeFromLeft (halfGap);
        m_redo.setBounds (area.removeFromLeft (iconBtnW));
        area.removeFromLeft (halfGap);
        m_randomize.setBounds (area.removeFromLeft (iconBtnW));
        area.removeFromLeft (gap);

        // Direita: readouts + settings.
        m_voicesLabel.setBounds (area.removeFromRight (80));
        area.removeFromRight (gap);
        m_polyphonyLabel.setBounds (area.removeFromRight (80));
        area.removeFromRight (gap);

        m_settings.setBounds (area.removeFromRight (40));
    }

    void timerCallback() override
    {
        int poly = m_processor.getPolyphony();
        int active = m_processor.getNumActiveVoices();
        m_polyphonyLabel.setText ("Poly: " + juce::String (poly), juce::dontSendNotification);
        m_voicesLabel.setText ("Voices: " + juce::String (active), juce::dontSendNotification);
    }

private:
    OmnivyAudioProcessor& m_processor;

    WaveformComponent m_logoWaveform;
    juce::Label m_wordmark;
    juce::Label m_tagline;

    juce::TextButton m_presetPrev { "‹" };
    juce::Label m_presetName;
    juce::TextButton m_presetNext { "›" };
    IconButtonComponent m_favorite { "★", "Favorito", PrimaryPurpleLight };
    juce::TextButton m_save { "💾" };
    juce::TextButton m_delete { "🗑" };
    juce::TextButton m_undo { "↩" };
    juce::TextButton m_redo { "↪" };
    juce::TextButton m_randomize { "🎲" };
    juce::Label m_polyphonyLabel;
    juce::Label m_voicesLabel;
    juce::TextButton m_settings { "⚙" };

    void savePreset()
    {
        juce::File file = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile ("Omnivy Preset.omnivy");
        juce::MemoryBlock block;
        m_processor.getStateInformation (block);
        if (auto stream = file.createOutputStream())
        {
            stream->write (block.getData(), static_cast<int> (block.getSize()));
            stream->flush();
            m_presetName.setText (file.getFileNameWithoutExtension(), juce::dontSendNotification);
        }
    }

    void randomize()
    {
        auto& apvts = m_processor.apvts;
        std::mt19937 rng (std::random_device {}());

        auto r01 = [&] () { return static_cast<float> (rng()) / static_cast<float> (rng.max()); };

        if (auto* p = apvts.getParameter (params::MasterVolume))
            p->setValue (0.4f + 0.6f * r01());
        if (auto* p = apvts.getParameter (params::Polyphony))
            p->setValue (static_cast<float> (4 + (rng() % 20)));

        for (int i = 0; i < params::NumOscillators; ++i)
        {
            if (auto* p = apvts.getParameter (params::oscActive (i)))
                p->setValue (r01() > 0.15f ? 1.0f : 0.0f);
            if (auto* p = apvts.getParameter (params::oscWaveform (i)))
                p->setValue (static_cast<float> (rng() % 6));
            if (auto* p = apvts.getParameter (params::oscLevel (i)))
                p->setValue (0.2f + 0.8f * r01());
            if (auto* p = apvts.getParameter (params::oscPan (i)))
                p->setValue (-0.7f + 1.4f * r01());
            if (auto* p = apvts.getParameter (params::oscOctave (i)))
                p->setValue (static_cast<float> (rng() % 5 - 2));
            if (auto* p = apvts.getParameter (params::oscSemitone (i)))
                p->setValue (static_cast<float> (rng() % 25 - 12));
            if (auto* p = apvts.getParameter (params::oscDetune (i)))
                p->setValue (-25.0f + 50.0f * r01());
            if (auto* p = apvts.getParameter (params::oscEngine (i)))
                p->setValue (static_cast<float> (rng() % 5));
            if (auto* p = apvts.getParameter (params::oscFmAmount (i)))
                p->setValue (100.0f * r01());
            if (auto* p = apvts.getParameter (params::oscAddHarmonicCount (i)))
                p->setValue (static_cast<float> (1 + (rng() % 32)));
            if (auto* p = apvts.getParameter (params::oscGranDensity (i)))
                p->setValue (500.0f * r01());
        }

        if (auto* p = apvts.getParameter (params::FilterCutoff))
            p->setValue (200.0f + 8000.0f * r01());
        if (auto* p = apvts.getParameter (params::FilterResonance))
            p->setValue (1.5f * r01());
        if (auto* p = apvts.getParameter (params::FilterType))
            p->setValue (static_cast<float> (rng() % 3));

        if (auto* p = apvts.getParameter (params::EnvAttack))
            p->setValue (0.005f + 0.5f * r01());
        if (auto* p = apvts.getParameter (params::EnvDecay))
            p->setValue (0.05f + 1.5f * r01());
        if (auto* p = apvts.getParameter (params::EnvSustain))
            p->setValue (r01());
        if (auto* p = apvts.getParameter (params::EnvRelease))
            p->setValue (0.05f + 1.0f * r01());
    }

    void showSettings()
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                                "Omnivy — Configurações",
                                                "Omnivy v1.0.0\nJUCE " + juce::SystemStats::getJUCEVersion()
                                                + "\n\nConfigurações detalhadas em implementação.");
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBarComponent)
};

// ============================================================================
// Faixa 2 — TabBarComponent
// Seis abas com ícone, nome, formato chanfrado, destaque neon roxo ativo.
// ============================================================================
class TabButtonComponent : public juce::Button
{
public:
    TabButtonComponent (const juce::String& icon, const juce::String& name)
        : juce::Button (name), m_icon (icon), m_name (name), m_active (false)
    {
        setClickingTogglesState (true);
        setToggleState (false, juce::dontSendNotification);
        setWantsKeyboardFocus (false);
    }

    void setActive (bool active)
    {
        if (m_active != active)
        {
            m_active = active;
            repaint();
        }
    }
    bool isActive() const noexcept { return m_active; }

    void paintButton (juce::Graphics& g, bool isMouseOver, bool /*isButtonDown*/) override
    {
        auto area = getLocalBounds().toFloat();

        if (m_active)
        {
            // Gradiente roxo para aba ativa.
            juce::ColourGradient grad (PrimaryPurpleDark, area.getX(), area.getY(),
                                       PrimaryPurple,     area.getX(), area.getBottom(),
                                       false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);

            // Glow externo sutil.
            g.setColour (GlowActive);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.5f);
        }
        else if (isMouseOver)
        {
            g.setColour (PrimaryPurple.withAlpha (0.18f));
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);
            g.setColour (Border);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
        }
        else
        {
            g.setColour (PanelDark);
            g.fillRoundedRectangle (area, (float) ControlCornerRadius);
            g.setColour (Border);
            g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
        }

        g.setColour (m_active ? juce::Colours::white
                              : (isMouseOver ? TextMain : TextSecondary));
        g.setFont (uiFont (FontSizeLabel, juce::Font::bold));
        auto text = m_icon + "  " + m_name;
        g.drawFittedText (text, (int) area.getX(), (int) area.getY(),
                          (int) area.getWidth(), (int) area.getHeight(),
                          juce::Justification::centred, 1);
    }

private:
    juce::String m_icon;
    juce::String m_name;
    bool m_active;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabButtonComponent)
};

class TabBarComponent : public juce::Component
{
public:
    TabBarComponent()
    {
        const struct { const char* icon; const char* name; } tabs[6] =
        {
            { "≋", "Oscillators" },
            { "→", "Routing" },
            { "∿", "Modulation" },
            { "▭", "Filters & FX" },
            { "▭○", "Visualizer" },
            { "⚙", "Settings" },
        };

        for (int i = 0; i < 6; ++i)
        {
            m_buttons[i] = new TabButtonComponent (tabs[i].icon, tabs[i].name);
            m_buttons[i]->setToggleState (i == 0, juce::dontSendNotification);
            m_buttons[i]->onClick = [this, i] { selectTab (i); };
            addAndMakeVisible (*m_buttons[i]);
        }
        m_activeTab = 0;
    }

    ~TabBarComponent() override = default;

    int getActiveTab() const noexcept { return m_activeTab; }

    using Callback = std::function<void(int)>;
    void setOnTabChanged (Callback cb) { m_onTabChanged = std::move (cb); }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (BackgroundMain);
        g.fillRect (area);

        // Acento diagonal roxo na extremidade direita.
        juce::Path accent;
        accent.startNewSubPath (area.getRight() - 4.0f, area.getY());
            accent.lineTo (area.getRight(), area.getY() + 16.0f);
            accent.lineTo (area.getRight(), area.getBottom());
        g.setColour (PrimaryPurple);
        g.fillPath (accent);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4);
        int tabW = area.getWidth() / 6;
        for (int i = 0; i < 6; ++i)
            m_buttons[i]->setBounds (area.removeFromLeft (tabW));
    }

    void selectTab (int index)
    {
        if (index < 0 || index >= 6 || index == m_activeTab)
            return;
        m_buttons[m_activeTab]->setToggleState (false, juce::dontSendNotification);
        m_buttons[index]->setToggleState (true, juce::dontSendNotification);
        m_activeTab = index;
        if (m_onTabChanged)
            m_onTabChanged (index);
    }

private:
    TabButtonComponent* m_buttons[6] = { nullptr };
    int m_activeTab = 0;
    Callback m_onTabChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabBarComponent)
};

// ============================================================================
// Faixa 4, coluna 1 — ModulationMatrixPanel
// Tabela: Source | Destination | Amount | Curve | + | -
// ============================================================================
class ModulationMatrixPanel : public PanelComponent
{
public:
    ModulationMatrixPanel (juce::AudioProcessorValueTreeState& /*apvts*/)
        : PanelComponent ("Modulation Matrix", false)
    {
        setInterceptsMouseClicks (true, true);

        m_addButton.setButtonText ("+");
        m_addButton.setTooltip ("Adicionar roteamento de modulação");
        m_addButton.onClick = [this] { /* preparada para implementação DSP */ };
        addAndMakeVisible (m_addButton);

        m_removeButton.setButtonText ("–");
        m_removeButton.setTooltip ("Remover roteamento selecionado");
        m_removeButton.setEnabled (false);
        m_removeButton.onClick = [this] { /* preparada para implementação DSP */ };
        addAndMakeVisible (m_removeButton);

        m_note.setText ("Matriz de modulação preparada para o sistema real de modulação "
                        "do Omnivy. Sem conexões funcionais até que o DSP as suporte.",
                        juce::dontSendNotification);
        m_note.setFont (uiFont (FontSizeMicro));
        m_note.setColour (juce::Label::textColourId, TextMuted);
        m_note.setJustificationType (juce::Justification::centred);
        m_note.setEnabled (false);
        addAndMakeVisible (m_note);
    }

    ~ModulationMatrixPanel() override = default;

    void paint (juce::Graphics& g) override
    {
        PanelComponent::paint (g);

        auto area = getLocalBounds().toFloat().reduced (10);
        area.removeFromTop (44);

        int colW = area.getWidth() / 5;
        g.setColour (TextSecondary);
        g.setFont (uiFont (FontSizeMicro));
        g.drawText ("Source", area.getX(), area.getY(), colW, 16, juce::Justification::centred, true);
        g.drawText ("Destination", area.getX() + colW, area.getY(), colW, 16, juce::Justification::centred, true);
        g.drawText ("Amount", area.getX() + colW * 2, area.getY(), colW, 16, juce::Justification::centred, true);
        g.drawText ("Curve", area.getX() + colW * 3, area.getY(), colW, 16, juce::Justification::centred, true);
        g.drawText ("", area.getX() + colW * 4, area.getY(), colW, 16, juce::Justification::centred, true);

        g.setColour (Border);
        g.fillRect ((int) area.getX(), (int) (area.getBottom() - 1), (int) area.getWidth(), 1);

        g.setColour (TextMuted);
        g.drawText ("— sem modulações configuradas —",
                    (int) area.getX(), (int) (area.getBottom() + 6), (int) area.getWidth(), 14,
                    juce::Justification::centred, true);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        area.removeFromTop (44);
        area.removeFromBottom (26);

        m_addButton.setBounds (area.removeFromRight (30));
        area.removeFromRight (8);
        m_removeButton.setBounds (area.removeFromRight (30));

        m_note.setBounds (area.removeFromBottom (22));
    }

private:
    juce::TextButton m_addButton { "+" };
    juce::TextButton m_removeButton { "–" };
    juce::Label m_note;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationMatrixPanel)
};

// ============================================================================
// Faixa 4, coluna 4 — ArpeggiatorPanel
// Abas: Arpeggiator | Clip
// ============================================================================
class ArpeggiatorPanel : public juce::Component
{
public:
    ArpeggiatorPanel (juce::AudioProcessorValueTreeState& /*apvts*/)
    {
        m_tabbed.setTabBarDepth (36);
        m_tabbed.addTab ("Arpeggiator", PanelDark, nullptr, true);
        m_tabbed.addTab ("Clip", PanelDark, nullptr, false);
        addAndMakeVisible (m_tabbed);

        buildArpPage();
        buildClipPage();

        m_tabbed.setCurrentTabIndex (0);
    }

    ~ArpeggiatorPanel() override = default;

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (ControlSurface);
        g.fillRoundedRectangle (area, (float) ControlCornerRadius);
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        area.removeFromTop (40);
        m_tabbed.setBounds (area);
    }

private:
    juce::TabbedComponent m_tabbed { juce::TabbedButtonBar::TabsAtTop };

    void buildArpPage()
    {
        auto* page = new juce::Component();
        page->setInterceptsMouseClicks (true, true);

        auto makeLabel = [&] (const juce::String& text)
        {
            auto* l = new juce::Label (text, text);
            l->setFont (uiFont (FontSizeMicro));
            l->setColour (juce::Label::textColourId, TextSecondary);
            l->setEnabled (false);
            page->addAndMakeVisible (*l);
            return l;
        };

        m_arpMode = new juce::ComboBox ("Mode");
        m_arpMode->addItem ("Off", 1);
        m_arpMode->addItem ("Up", 2);
        m_arpMode->addItem ("Down", 3);
        m_arpMode->addItem ("UpDown", 4);
        m_arpMode->addItem ("Random", 5);
        m_arpMode->setSelectedId (1);
        m_arpMode->setEnabled (false);
        page->addAndMakeVisible (*m_arpMode);

        m_arpPlay.setButtonText ("▶ Play");
        m_arpPlay.setEnabled (false);
        page->addAndMakeVisible (m_arpPlay);

        makeLabel ("Modo");

        m_tabbed.getTabbedButtonBar().addAndMakeVisible (*page);
    }

    void buildClipPage()
    {
        auto* page = new juce::Component();
        page->setInterceptsMouseClicks (true, true);

        auto makeLabel = [&] (const juce::String& text)
        {
            auto* l = new juce::Label (text, text);
            l->setFont (uiFont (FontSizeMicro));
            l->setColour (juce::Label::textColourId, TextSecondary);
            l->setEnabled (false);
            page->addAndMakeVisible (*l);
            return l;
        };

        m_clipMode = new juce::ComboBox ("Clip Mode");
        m_clipMode->addItem ("Off", 1);
        m_clipMode->addItem ("Step", 2);
        m_clipMode->setSelectedId (1);
        m_clipMode->setEnabled (false);
        page->addAndMakeVisible (*m_clipMode);

        m_clipPlay.setButtonText ("▶ Play");
        m_clipPlay.setEnabled (false);
        page->addAndMakeVisible (m_clipPlay);

        makeLabel ("Modo de clip");

        m_tabbed.getTabbedButtonBar().addAndMakeVisible (*page);
    }

    juce::ComboBox* m_arpMode = nullptr;
    juce::TextButton m_arpPlay { "▶ Play" };
    juce::ComboBox* m_clipMode = nullptr;
    juce::TextButton m_clipPlay { "▶ Play" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpeggiatorPanel)
};

// ============================================================================
// Faixa 5 — BottomBandComponent
// Teclado MIDI integrado + Hold/Mono/Legato + Master com VU vertical + footer.
// O teclado MIDI preserva entrada MIDI, interação por mouse, Pitch Bend,
// Mod Wheel e comunicação com o processamento de áudio.
// ============================================================================
class BottomBandComponent : public juce::Component, private juce::Timer
{
public:
    BottomBandComponent (OmnivyAudioProcessor& processor)
        : m_processor (processor)
    {
        setInterceptsMouseClicks (true, true);

        // --- Teclado MIDI --------------------------------------------------
        m_keyboardComponent = std::make_unique<juce::MidiKeyboardComponent> (
            m_keyboardState, juce::KeyboardComponentBase::horizontalKeyboard);
        addAndMakeVisible (*m_keyboardComponent);

        // --- Controles globais ---------------------------------------------
        m_hold.setButtonText ("Hold");
        m_hold.setClickingTogglesState (true);
        m_hold.setTooltip ("Hold (sustenta notas)");
        addAndMakeVisible (m_hold);

        m_mono.setButtonText ("Mono");
        m_mono.setClickingTogglesState (true);
        m_mono.setTooltip ("Mono (uma nota por vez)");
        addAndMakeVisible (m_mono);

        m_legato.setButtonText ("Legato");
        m_legato.setClickingTogglesState (true);
        m_legato.setTooltip ("Legato");
        addAndMakeVisible (m_legato);

        m_masterLabel.setText ("Master", juce::dontSendNotification);
        m_masterLabel.setFont (uiFont (FontSizeLabel, juce::Font::bold));
        m_masterLabel.setColour (juce::Label::textColourId, TextMain);
        m_masterLabel.setJustificationType (juce::Justification::centred);
        m_masterLabel.setEnabled (false);
        addAndMakeVisible (m_masterLabel);

        m_masterKnob = new KnobComponent (m_processor.apvts, params::MasterVolume,
                                          "Volume", "", KnobComponent::Size::Medium, PrimaryPurple);
        addAndMakeVisible (*m_masterKnob);

        m_vuMeter.setColour (PrimaryPurple);
        addAndMakeVisible (m_vuMeter);

        startTimerHz (30);
    }

    ~BottomBandComponent() override = default;

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (PanelDark);
        g.fillRoundedRectangle (area, (float) PanelCornerRadius);
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) PanelCornerRadius, (float) BorderWidth);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        const int gap = 10;

        // Teclado MIDI ocupa a maior parte da largura, na metade inferior da faixa.
        int keyboardH = 76;
        auto keyboardArea = area.removeFromBottom (keyboardH);
        m_keyboardComponent->setBounds (keyboardArea);

        // Controles acima do teclado.
        area.removeFromBottom (gap);

        int btnW = 64;
        m_hold.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap / 2);
        m_mono.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap / 2);
        m_legato.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap);

        int masterW = 110;
        m_masterLabel.setBounds (area.removeFromRight (masterW));
        m_masterKnob->setBounds (area.removeFromRight (masterW));
        m_vuMeter.setBounds (area.removeFromRight (56));
    }

    void timerCallback() override
    {
        float peak = m_processor.getOutputPeak();
        m_vuMeter.setLevel (peak);
        m_vuMeter.repaint();
    }

    juce::MidiKeyboardState& getKeyboardState() noexcept { return m_keyboardState; }

private:
    OmnivyAudioProcessor& m_processor;
    juce::MidiKeyboardState m_keyboardState;
    std::unique_ptr<juce::MidiKeyboardComponent> m_keyboardComponent;

    juce::TextButton m_hold { "Hold" };
    juce::TextButton m_mono { "Mono" };
    juce::TextButton m_legato { "Legato" };
    juce::Label m_masterLabel;
    KnobComponent* m_masterKnob = nullptr;

    class VUMeter : public juce::Component
    {
    public:
        void setColour (juce::Colour c) { m_colour = c; repaint(); }
        void setLevel (float level) { m_level = juce::jlimit (0.0f, 1.0f, level); repaint(); }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.setColour (ControlSurface);
            g.fillRoundedRectangle (area, 6.0f);
            g.setColour (Border);
            g.drawRoundedRectangle (area, 6.0f, 1.0f);

            int segments = 12;
            float segW = (area.getWidth() - 8.0f) / (float) segments;
            float segH = area.getHeight() * 0.6f;
            float y = area.getBottom() - segH - 4.0f;

            int lit = juce::roundToInt (m_level * (float) segments);
            for (int i = 0; i < segments; ++i)
            {
                bool on = i < lit;
                juce::Colour c;
                if (i < 6)       c = on ? Osc4Colour : Osc4Colour.withAlpha (0.15f);
                else if (i < 10) c = on ? Osc5Colour : Osc5Colour.withAlpha (0.15f);
                else             c = on ? PrimaryPurple : PrimaryPurple.withAlpha (0.15f);

                g.setColour (c);
                g.fillRoundedRectangle ((int) (4.0f + i * segW), (int) y, (int) (segW - 4.0f), (int) segH, 3.0f);
            }

            g.setColour (TextMuted);
            g.setFont (uiFont (FontSizeMicro));
            g.drawText ("VU", (int) area.getX(), (int) area.getY() + 2, (int) area.getWidth(), 12,
                        juce::Justification::centred, true);
        }

    private:
        juce::Colour m_colour = PrimaryPurple;
        float m_level = 0.0f;
    };

    VUMeter m_vuMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomBandComponent)
};

// ============================================================================
// MasterBandComponent
// Faixa 5 (direita) — Volume master real + VU meter com pico real de saída.
// ============================================================================

class MasterBandComponent : public juce::Component, private juce::Timer
{
public:
    MasterBandComponent (OmnivyAudioProcessor& processor)
        : m_processor (processor)
    {
        setInterceptsMouseClicks (true, true);

        m_volumeLabel.setText ("Master", juce::dontSendNotification);
        m_volumeLabel.setFont (uiFont (FontSizeLabel, juce::Font::bold));
        m_volumeLabel.setColour (juce::Label::textColourId, TextMain);
        m_volumeLabel.setJustificationType (juce::Justification::left);
        m_volumeLabel.setEnabled (false);
        addAndMakeVisible (m_volumeLabel);

        m_volumeKnob = new KnobComponent (processor.apvts, params::MasterVolume,
                                          "Volume", "", KnobComponent::Size::Medium, PrimaryPurple);
        addAndMakeVisible (*m_volumeKnob);

        m_vuLabel.setText ("VU", juce::dontSendNotification);
        m_vuLabel.setFont (uiFont (FontSizeMicro));
        m_vuLabel.setColour (juce::Label::textColourId, TextMuted);
        m_vuLabel.setJustificationType (juce::Justification::centred);
        m_vuLabel.setEnabled (false);
        addAndMakeVisible (m_vuLabel);

        startTimerHz (30);
    }

    ~MasterBandComponent() override = default;

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (ControlSurface);
        g.fillRoundedRectangle (area, (float) ControlCornerRadius);
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

        int segments = 10;
        float peak = m_processor.getOutputPeak();
        int lit = juce::roundToInt (juce::jlimit (0.0f, 1.0f, peak / 1.2f) * segments);

        float segW = (area.getWidth() - 40.0f) / (float) segments;
        float segH = area.getHeight() * 0.5f;
        float y = area.getCentreY() - segH * 0.5f;

        for (int i = 0; i < segments; ++i)
        {
            bool on = i < lit;
            juce::Colour segColour;
            if (i < 6)
                segColour = on ? Osc4Colour : Osc4Colour.withAlpha (0.15f);
            else if (i < 9)
                segColour = on ? Osc5Colour : Osc5Colour.withAlpha (0.15f);
            else
                segColour = on ? PrimaryPurple : PrimaryPurple.withAlpha (0.15f);

            g.setColour (segColour);
            g.fillRoundedRectangle (30.0f + i * segW + 2.0f, y,
                                    segW - 4.0f, segH, 3.0f);
        }

        if (peak > 0.005f)
        {
            float peakX = 30.0f + juce::jlimit (0.0f, 1.0f, peak / 1.2f) * (area.getWidth() - 40.0f);
            g.setColour (juce::Colours::white);
            g.drawLine (peakX, area.getY() + 6.0f, peakX, area.getBottom() - 6.0f, 1.5f);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        m_volumeLabel.setBounds (area.removeFromLeft (60));
        m_volumeKnob->setBounds (area.removeFromLeft (56));
        area.removeFromLeft (8);
        m_vuLabel.setBounds (area.removeFromLeft (22));
    }

    void timerCallback() override { repaint(); }

private:
    OmnivyAudioProcessor& m_processor;
    juce::Label m_volumeLabel;
    KnobComponent* m_volumeKnob = nullptr;
    juce::Label m_vuLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterBandComponent)
};

} // namespace gui
} // namespace omnivy

// ============================================================================
// OmnivyAudioProcessorEditor
// ============================================================================

namespace omnivy {
namespace gui {

OmnivyAudioProcessorEditor::OmnivyAudioProcessorEditor (OmnivyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&m_lookAndFeel);

    using namespace omnivy::gui;

    // Faixa 1 — Top Bar.
    m_topBar = new TopBarComponent (audioProcessor);
    addAndMakeVisible (*m_topBar);

    // Faixa 2 — Tab Bar.
    m_tabBar = new TabBarComponent();
    m_tabBar->setOnTabChanged ([this] (int index) { tabSelected (index); });
    addAndMakeVisible (*m_tabBar);

    // Faixa 3 — Painéis superiores (quatro colunas).
    m_oscPanel    = new OscillatorPanel (audioProcessor.apvts);
    m_wavetablePanel = new WavetablePanel (audioProcessor.apvts);
    m_routingPanel = new RoutingPanel (audioProcessor);
    m_filterPanel = new FilterPanel (audioProcessor.apvts);

    addAndMakeVisible (*m_oscPanel);
    addAndMakeVisible (*m_wavetablePanel);
    addAndMakeVisible (*m_routingPanel);
    addAndMakeVisible (*m_filterPanel);

    // Faixa 4 — Painéis inferiores (quatro colunas).
    m_modMatrixPanel = new ModulationMatrixPanel (audioProcessor.apvts);
    m_visualizerPanel = new VisualizerPanel (audioProcessor.apvts);
    m_arpPanel = new ArpeggiatorPanel (audioProcessor.apvts);

    addAndMakeVisible (*m_modMatrixPanel);
    addAndMakeVisible (*m_visualizerPanel);
    addAndMakeVisible (*m_arpPanel);

    m_envTabbed = new juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop);
    m_envTabbed->setTabBarDepth (36);
    m_envTabbed->addTab ("Envelopes", PanelDark, new EnvelopePanel (audioProcessor.apvts), true);
    m_envTabbed->addTab ("LFOs", PanelDark, new juce::Component(), false);
    m_envTabbed->addTab ("Sequencer", PanelDark, new juce::Component(), false);
    addAndMakeVisible (*m_envTabbed);

    // Faixa 5 — Teclado + Master + Footer.
    m_bottomBand = new BottomBandComponent (audioProcessor);
    addAndMakeVisible (*m_bottomBand);

    m_activeTab = 0;
}

OmnivyAudioProcessorEditor::~OmnivyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OmnivyAudioProcessorEditor::tabSelected (int index)
{
    m_activeTab = index;
    repaint();
}

void OmnivyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BackgroundMain);
}

void OmnivyAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    m_topBar->setBounds (area.removeFromTop (TopBarHeight));
    m_tabBar->setBounds (area.removeFromTop (TabBarHeight));

    // Reservar espaço para a Faixa 5 (BottomBand com teclado).
    area.removeFromBottom (BottomBandHeight);

    int contentH = area.getHeight();
    int topRowH = contentH / 2;
    int bottomRowH = contentH - topRowH;

    auto topRow = area.removeFromTop (topRowH);
    int colW = topRow.getWidth() / 4;
    int gap = 12;

    m_oscPanel->setBounds    (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_wavetablePanel->setBounds (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_routingPanel->setBounds (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_filterPanel->setBounds (topRow.removeFromLeft (colW));

    auto bottomRow = area.removeFromTop (bottomRowH);
    m_modMatrixPanel->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_envTabbed->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_visualizerPanel->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_arpPanel->setBounds (bottomRow.removeFromLeft (colW));

    m_bottomBand->setBounds (area);
}

} // namespace gui
} // namespace omnivy
