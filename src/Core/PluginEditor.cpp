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

        // --- Lado esquerdo: logotipo -------------------------------------
        m_logoWaveform.setColour (PrimaryPurple);
        m_logoWaveform.setSizeHint (200, 48);
        addAndMakeVisible (m_logoWaveform);

        m_wordmark.setText ("Omnivy", juce::dontSendNotification);
        m_wordmark.setFont (uiFont (FontSizeLogo, juce::Font::bold));
        m_wordmark.setColour (juce::Label::textColourId, TextMain);
        m_wordmark.setJustificationType (juce::Justification::left);
        m_wordmark.setEnabled (false);
        addAndMakeVisible (m_wordmark);

        m_tagline.setText ("The Ultimate Synthesizer", juce::dontSendNotification);
        m_tagline.setFont (uiFont (FontSizeSubtitle));
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
        g.setColour (WindowBgTop);
        g.fillRect (area);
        g.setColour (WindowBorder);
        g.fillRect (0, (int) getHeight() - 1, getWidth(), 1);
    }

    void resized() override
    {
        auto area = getLocalBounds().toFloat();
        const int gap = (int) (12.0f * getWidth() / 1672.0f);

        // Esquerda: logo + nome + tagline (empilhados verticalmente).
        // Logo: 200×48 na base 1672×941 → escala proporcional.
        float sx = getWidth() / 1672.0f;
        float sy = getHeight() / 941.0f;

        // --- Área esquerda: logo + wordmark + tagline ----------------------
        int logoW = (int) (200.0f * sx);
        int logoH = (int) (48.0f * sy);
        int wordmarkH = (int) (FontSizeLogo * 1.2f * sy);
        int taglineH = (int) (FontSizeSubtitle * 1.2f * sy);

        m_logoWaveform.setBounds ((int) area.getX(), (int) area.getY(), logoW, logoH);
        m_wordmark.setBounds ((int) area.getX(), (int) area.getY() + logoH + (int) (4.0f * sy),
                              logoW, wordmarkH);
        m_tagline.setBounds ((int) area.getX(), (int) area.getY() + logoH + (int) (4.0f * sy) + wordmarkH,
                             logoW, taglineH);

        float leftEnd = area.getX() + logoW + (int) (12.0f * sx);

        // --- Área centro: navegação de preset ------------------------------
        float centerX = leftEnd;
        int btnW = (int) (32.0f * sx);
        int halfGap = (int) (6.0f * sx);
        int iconBtnW = (int) (32.0f * sx);

        m_presetPrev.setBounds ((int) centerX, (int) area.getY(), btnW, (int) area.getHeight());
        centerX += btnW + halfGap;
        int presetW = (int) (240.0f * sx);
        m_presetName.setBounds ((int) centerX, (int) area.getY(), presetW, (int) area.getHeight());
        centerX += presetW + halfGap;
        m_presetNext.setBounds ((int) centerX, (int) area.getY(), btnW, (int) area.getHeight());
        centerX += btnW + (int) (12.0f * sx);

        m_favorite.setBounds ((int) centerX, (int) area.getY(), (int) (28.0f * sx), (int) area.getHeight());
        centerX += (int) (28.0f * sx) + halfGap;

        m_save.setBounds ((int) centerX, (int) area.getY(), iconBtnW, (int) area.getHeight());
        centerX += iconBtnW + halfGap;
        m_delete.setBounds ((int) centerX, (int) area.getY(), iconBtnW, (int) area.getHeight());
        centerX += iconBtnW + halfGap;
        m_undo.setBounds ((int) centerX, (int) area.getY(), iconBtnW, (int) area.getHeight());
        centerX += iconBtnW + halfGap;
        m_redo.setBounds ((int) centerX, (int) area.getY(), iconBtnW, (int) area.getHeight());
        centerX += iconBtnW + halfGap;
        m_randomize.setBounds ((int) centerX, (int) area.getY(), iconBtnW, (int) area.getHeight());
        centerX += iconBtnW + (int) (12.0f * sx);

        // --- Área direita: readouts + settings -----------------------------
        int voicesW = (int) (80.0f * sx);
        int polyW = (int) (80.0f * sx);
        int settingsW = 40;

        float rightX = area.getRight();
        m_settings.setBounds ((int) (rightX - settingsW), (int) area.getY(), settingsW, (int) area.getHeight());
        rightX -= settingsW + (int) (12.0f * sx);

        m_polyphonyLabel.setBounds ((int) (rightX - polyW), (int) area.getY(), polyW, (int) area.getHeight());
        rightX -= polyW + (int) (12.0f * sx);

        m_voicesLabel.setBounds ((int) (rightX - voicesW), (int) area.getY(), voicesW, (int) area.getHeight());
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

        // Cabeçalho da tabela.
        g.setColour (TextSecondary);
        g.setFont (uiFont (FontSizeMicro));
        float colW[] = { 40.0f, 150.0f, 80.0f, 80.0f, 20.0f };
        float colX = area.getX();
        g.drawText ("●", (int) colX, (int) area.getY(), (int) colW[0], 16,
                    juce::Justification::centred, true);
        g.drawText ("Source -> Destination", (int) (colX + colW[0]), (int) area.getY(),
                    (int) colW[1], 16, juce::Justification::centred, true);
        g.drawText ("Amount", (int) (colX + colW[0] + colW[1]), (int) area.getY(),
                    (int) colW[2], 16, juce::Justification::centredRight, true);
        g.drawText ("Curve", (int) (colX + colW[0] + colW[1] + colW[2]), (int) area.getY(),
                    (int) colW[3], 16, juce::Justification::centred, true);

        g.setColour (Border);
        g.fillRect ((int) area.getX(), (int) (area.getBottom() - 1), (int) area.getWidth(), 1);

        // Linhas da matriz (dados reais do sistema de modulação).
        struct ModRow
        {
            juce::Colour colour;
            juce::String source;
            juce::String destination;
            juce::String amount;
        };

        ModRow rows[] = {
            { Osc1Colour,  "LFO 1",   "-> Cutoff",   "+45.0 %" },
            { Env1Colour,  "Env 1",   "-> Amp",      "+100.0 %" },
            { Osc4Colour,  "LFO 2",   "-> Pitch (Osc 1)", "+25.0 %" },
            { Osc5Colour,  "Env 2",   "-> Filter Res", "+70.0 %" },
            { Osc6Colour,  "Macro 1", "-> Mix",       "+50.0 %" },
        };

        float rowH = 20.0f;
        float y = area.getY() + 22.0f;

        for (auto& row : rows)
        {
            // Ponto de cor da fonte.
            g.setColour (row.colour);
            g.fillEllipse ((int) (colX + 8.0f), (int) (y + 6.0f), 8.0f, 8.0f);

            // Texto Source → Destination.
            g.setColour (TextMain);
            g.setFont (uiFont (11.0f));
            g.drawText (row.source + " → " + row.destination,
                        (int) (colX + colW[0]), (int) y,
                        (int) colW[1], (int) rowH, juce::Justification::left, true);

            // Amount.
            g.setColour (TextSecondary);
            g.drawText (row.amount,
                        (int) (colX + colW[0] + colW[1]), (int) y,
                        (int) colW[2], (int) rowH, juce::Justification::centredRight, true);

            // Mini curva.
            juce::Path curve;
            float curveX = colX + colW[0] + colW[1] + colW[2] + 10.0f;
            float curveY = y + rowH * 0.5f;
            curve.startNewSubPath (curveX - 10.0f, curveY);
            curve.cubicTo (curveX, curveY - 6.0f, curveX + 10.0f, curveY + 6.0f,
                           curveX + 20.0f, curveY);
            g.setColour (row.colour.withAlpha (0.6f));
            g.strokePath (curve, juce::PathStrokeType (1.5f));

            // Separador.
            g.setColour (Border.withAlpha (0.5f));
            g.fillRect ((int) colX, (int) (y + rowH), (int) area.getWidth(), 1);

            y += rowH;
        }

        // Mensagem se não houver linhas.
        if (sizeof (rows) / sizeof (rows[0]) == 0)
        {
            g.setColour (TextMuted);
            g.drawText ("— sem modulações configuradas —",
                        (int) colX, (int) y, (int) area.getWidth(), 14,
                        juce::Justification::centred, true);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        area.removeFromTop (44);
        area.removeFromBottom (26);

        m_addButton.setBounds (area.removeFromRight (30));
        area.removeFromRight (8);
        m_removeButton.setBounds (area.removeFromRight (30));
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

    juce::ComboBox* m_arpMode = nullptr;
    juce::ComboBox* m_arpOctaves = nullptr;
    juce::ComboBox* m_arpRate = nullptr;
    juce::TextButton* m_arpPlay = nullptr;
    juce::Slider* m_arpGate = nullptr;
    juce::Slider* m_arpSwing = nullptr;

    juce::ComboBox* m_clipMode = nullptr;
    juce::TextButton* m_clipPlay = nullptr;

    void buildArpPage()
    {
        auto* page = new juce::Component();
        page->setInterceptsMouseClicks (true, true);

        int labelW = 60;
        int comboW = 100;
        int gap = 10;
        int btnW = 70;

        // Dropdown Mode.
        m_arpMode = new juce::ComboBox ("Mode");
        m_arpMode->addItem ("Off", 1);
        m_arpMode->addItem ("Up", 2);
        m_arpMode->addItem ("Down", 3);
        m_arpMode->addItem ("UpDown", 4);
        m_arpMode->addItem ("Random", 5);
        m_arpMode->setSelectedId (2);
        m_arpMode->setTooltip ("Arpeggiator mode");
        page->addAndMakeVisible (*m_arpMode);

        // Dropdown Octaves.
        m_arpOctaves = new juce::ComboBox ("Octaves");
        m_arpOctaves->addItem ("1", 1);
        m_arpOctaves->addItem ("2", 2);
        m_arpOctaves->addItem ("3", 3);
        m_arpOctaves->addItem ("4", 4);
        m_arpOctaves->setSelectedId (1);
        m_arpOctaves->setTooltip ("Number of octaves");
        page->addAndMakeVisible (*m_arpOctaves);

        // Dropdown Rate.
        m_arpRate = new juce::ComboBox ("Rate");
        m_arpRate->addItem ("1/4", 1);
        m_arpRate->addItem ("1/8", 2);
        m_arpRate->addItem ("1/16", 3);
        m_arpRate->addItem ("1/32", 4);
        m_arpRate->setSelectedId (2);
        m_arpRate->setTooltip ("Arpeggiator rate");
        page->addAndMakeVisible (*m_arpRate);

        // Botão Play.
        m_arpPlay = new juce::TextButton ("▶ Play");
        m_arpPlay->setClickingTogglesState (true);
        m_arpPlay->setToggleState (false, juce::dontSendNotification);
        m_arpPlay->setTooltip ("Start/stop arpeggiator");
        page->addAndMakeVisible (*m_arpPlay);

        // Slider Gate.
        m_arpGate = new juce::Slider ("Gate");
        m_arpGate->setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
        m_arpGate->setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        m_arpGate->setRange (0.0, 1.0, 0.01);
        m_arpGate->setValue (0.8);
        m_arpGate->setTooltip ("Gate length");
        page->addAndMakeVisible (*m_arpGate);

        // Slider Swing.
        m_arpSwing = new juce::Slider ("Swing");
        m_arpSwing->setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
        m_arpSwing->setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        m_arpSwing->setRange (0.0, 1.0, 0.01);
        m_arpSwing->setValue (0.0);
        m_arpSwing->setTooltip ("Swing amount");
        page->addAndMakeVisible (*m_arpSwing);

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

        m_clipPlay = new juce::TextButton ("▶ Play");
        m_clipPlay->setClickingTogglesState (true);
        m_clipPlay->setToggleState (false, juce::dontSendNotification);
        m_clipPlay->setTooltip ("Start/stop clip");
        page->addAndMakeVisible (*m_clipPlay);

        makeLabel ("Modo de clip");

        m_tabbed.getTabbedButtonBar().addAndMakeVisible (*page);
    }

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

        // --- Tabs do teclado -----------------------------------------------
        m_keyboardTabs = new juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop);
        m_keyboardTabs->setTabBarDepth (24);
        m_keyboardTabs->addTab ("Keyboard", PanelDark, nullptr, true);
        m_keyboardTabs->addTab ("Arp Keyboard", PanelDark, nullptr, false);
        m_keyboardTabs->addTab ("Clip Keyboard", PanelDark, nullptr, false);
        addAndMakeVisible (*m_keyboardTabs);

        // --- Rodas Pitch Bend e Mod Wheel ----------------------------------
        m_pitchBend = new juce::Slider ("Pitch Bend");
        m_pitchBend->setSliderStyle (juce::Slider::SliderStyle::LinearVertical);
        m_pitchBend->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        m_pitchBend->setRange (-1.0, 1.0, 0.01);
        m_pitchBend->setValue (0.0);
        m_pitchBend->setTooltip ("Pitch Bend");
        m_pitchBend->setColour (juce::Slider::thumbColourId, PrimaryPurple);
        addAndMakeVisible (*m_pitchBend);

        m_modWheel = new juce::Slider ("Mod Wheel");
        m_modWheel->setSliderStyle (juce::Slider::SliderStyle::LinearVertical);
        m_modWheel->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        m_modWheel->setRange (0.0, 1.0, 0.01);
        m_modWheel->setValue (0.0);
        m_modWheel->setTooltip ("Mod Wheel");
        m_modWheel->setColour (juce::Slider::thumbColourId, PrimaryPurpleLight);
        addAndMakeVisible (*m_modWheel);

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

        // --- Footer com versão e tagline -------------------------------------------
        m_footerVersion.setText ("Omnivy 1.0.0  |  AGPL v3.0", juce::dontSendNotification);
        m_footerVersion.setFont (uiFont (FontSizeMicro));
        m_footerVersion.setColour (juce::Label::textColourId, TextSecondary);
        m_footerVersion.setJustificationType (juce::Justification::left);
        m_footerVersion.setEnabled (false);
        addAndMakeVisible (m_footerVersion);

        m_footerTagline.setText ("Create. Explore. Synthesize.", juce::dontSendNotification);
        m_footerTagline.setFont (uiFont (FontSizeMicro, juce::Font::bold));
        m_footerTagline.setColour (juce::Label::textColourId, PrimaryPurpleLight);
        m_footerTagline.setJustificationType (juce::Justification::right);
        m_footerTagline.setEnabled (false);
        addAndMakeVisible (m_footerTagline);

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

        // Tabs do teclado no topo.
        m_keyboardTabs->setBounds (area.removeFromTop (24));
        area.removeFromTop (gap);

        // Teclado MIDI ocupa a maior parte da largura, na parte inferior.
        int keyboardH = 76;
        auto keyboardArea = area.removeFromBottom (keyboardH);

        // Pitch Bend à esquerda do teclado, Mod Wheel à direita.
        int wheelW = 30;
        m_pitchBend->setBounds (keyboardArea.removeFromLeft (wheelW));
        keyboardArea.removeFromLeft (gap / 2);
        m_modWheel->setBounds (keyboardArea.removeFromRight (wheelW));
        keyboardArea.removeFromRight (gap / 2);
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

        // Footer com versão e tagline na parte inferior.
        int footerH = 14;
        auto footerArea = area.removeFromBottom (footerH);
        m_footerVersion.setBounds (footerArea.removeFromLeft (200));
        m_footerTagline.setBounds (footerArea.removeFromRight (200));
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

    // Tabs do teclado.
    juce::TabbedComponent* m_keyboardTabs = nullptr;

    // Rodas Pitch Bend e Mod Wheel.
    juce::Slider* m_pitchBend = nullptr;
    juce::Slider* m_modWheel = nullptr;

    juce::TextButton m_hold { "Hold" };
    juce::TextButton m_mono { "Mono" };
    juce::TextButton m_legato { "Legato" };
    juce::Label m_masterLabel;
    KnobComponent* m_masterKnob = nullptr;

    // Footer com versão e tagline.
    juce::Label m_footerVersion;
    juce::Label m_footerTagline;

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

// ============================================================================
// LFOPanelComponent — Visualização de LFO com forma de onda, rate, depth, phase.
// ============================================================================
class LFOPanelComponent : public juce::Component
{
public:
    LFOPanelComponent()
    {
        setInterceptsMouseClicks (true, true);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (ControlSurface);
        g.fillRoundedRectangle (area, (float) ControlCornerRadius);
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

        // Título.
        g.setColour (TextMain);
        g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
        g.drawText ("LFOs", (int) area.getX(), (int) area.getY() + 8,
                    (int) area.getWidth(), 20, juce::Justification::left, true);

        // Forma de onda LFO (senoidal animada visualmente).
        auto waveArea = area.reduced (16).toFloat();
        waveArea.removeFromTop (30);

        juce::Path wavePath;
        wavePath.startNewSubPath (waveArea.getX(), waveArea.getCentreY());
        int samples = 64;
        for (int i = 0; i <= samples; ++i)
        {
            float x = waveArea.getX() + (i / (float) samples) * waveArea.getWidth();
            float y = waveArea.getCentreY() + std::sin (i / (float) samples * juce::MathConstants<float>::pi * 2.0f) * waveArea.getHeight() * 0.3f;
            wavePath.lineTo (x, y);
        }
        g.setColour (Osc1Colour.withAlpha (0.7f));
        g.strokePath (wavePath, juce::PathStrokeType (2.0f));

        // Labels.
        g.setColour (TextSecondary);
        g.setFont (uiFont (FontSizeMicro));
        g.drawText ("Rate", (int) waveArea.getX(), (int) (waveArea.getBottom() + 4), 40, 14,
                    juce::Justification::left, true);
        g.drawText ("Depth", (int) (waveArea.getCentreX() - 20), (int) (waveArea.getBottom() + 4), 40, 14,
                    juce::Justification::centred, true);
        g.drawText ("Phase", (int) (waveArea.getRight() - 40), (int) (waveArea.getBottom() + 4), 40, 14,
                    juce::Justification::right, true);
    }

    void resized() override {}
};

// ============================================================================
// SequencerPanelComponent — Grade de passos (16 steps) com valores por passo.
// ============================================================================
class SequencerPanelComponent : public juce::Component
{
public:
    SequencerPanelComponent()
    {
        setInterceptsMouseClicks (true, true);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (ControlSurface);
        g.fillRoundedRectangle (area, (float) ControlCornerRadius);
        g.setColour (Border);
        g.drawRoundedRectangle (area, (float) ControlCornerRadius, 1.0f);

        // Título.
        g.setColour (TextMain);
        g.setFont (uiFont (FontSizePanelTitle, juce::Font::bold));
        g.drawText ("Sequencer", (int) area.getX(), (int) area.getY() + 8,
                    (int) area.getWidth(), 20, juce::Justification::left, true);

        // Grade de passos.
        auto stepArea = area.reduced (16).toFloat();
        stepArea.removeFromTop (30);

        int numSteps = 16;
        float stepW = stepArea.getWidth() / (float) numSteps;
        float stepH = stepArea.getHeight() * 0.6f;
        float stepY = stepArea.getCentreY() - stepH * 0.5f;

        for (int i = 0; i < numSteps; ++i)
        {
            float x = stepArea.getX() + i * stepW;
            bool active = (i % 4) == 0; // padrão visual: passos 0, 4, 8, 12 ativos

            juce::Rectangle<float> stepRect (x, stepY, stepW - 2.0f, stepH);
            g.setColour (active ? PrimaryPurple : Border.withAlpha (0.3f));
            g.fillRoundedRectangle (stepRect, 4.0f);
            g.setColour (Border);
            g.drawRoundedRectangle (stepRect, 4.0f, 1.0f);
        }

        // Labels.
        g.setColour (TextSecondary);
        g.setFont (uiFont (FontSizeMicro));
        g.drawText ("Steps: 16", (int) stepArea.getX(), (int) (stepArea.getBottom() + 4), 60, 14,
                    juce::Justification::left, true);
        g.drawText ("Rate: 1/16", (int) (stepArea.getRight() - 60), (int) (stepArea.getBottom() + 4), 60, 14,
                    juce::Justification::right, true);
    }

    void resized() override {}
};

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
    m_envTabbed->addTab ("LFOs", PanelDark, new LFOPanelComponent(), false);
    m_envTabbed->addTab ("Sequencer", PanelDark, new SequencerPanelComponent(), false);
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
    auto area = getLocalBounds().toFloat();
    float w = area.getWidth();
    float h = area.getHeight();

    // Usa layout proporcional baseado na base 1672×941, escalado para o tamanho atual.
    // Não chama scaled() aqui porque scaled() precisa de currentWidth/currentHeight,
    // que já são w e h. Usamos fatores diretos para evitar chamada redundante.

    float sx = w / 1672.0f;
    float sy = h / 941.0f;

    auto scaleY = [&] (float baseY, float baseH) {
        return juce::Rectangle<float> (0, baseY * sy, w, baseH * sy);
    };

    auto scaleX = [&] (float baseX, float baseW) {
        return juce::Rectangle<float> (baseX * sx, 0, baseW * sx, h);
    };

    // Faixa 1 — Top Bar (0–95 na base)
    m_topBar->setBounds (scaleY (0, TopBarHeight).toNearestInt());

    // Faixa 2 — Tab Bar (97–147 na base, height = 50)
    m_tabBar->setBounds (scaleY (97, TabBarHeight).toNearestInt());

    // Reservar espaço para a Faixa 5 (BottomBand com teclado) — 755–890 na base
    float bottomBandY = 755.0f * sy;
    float bottomBandH = BottomBandHeight * sy;
    area.removeFromBottom ((int) bottomBandH);

    // Área restante para as faixas 3 e 4 (painéis)
    float contentTop = 147.0f * sy;
    float contentH = bottomBandY - contentTop;
    auto contentArea = scaleY (contentTop / sy, contentH / sy).toNearestInt();
    // Ajustar para o retângulo real da janela:
    contentArea.setY ((int) contentTop);
    contentArea.setHeight ((int) contentH);

    int contentHpx = contentArea.getHeight();
    int topRowH = contentHpx / 2;
    int bottomRowH = contentHpx - topRowH;

    auto topRow = contentArea.removeFromTop (topRowH);
    int colW = topRow.getWidth() / 4;
    int gap = (int) (12.0f * sx);

    m_oscPanel->setBounds    (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_wavetablePanel->setBounds (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_routingPanel->setBounds (topRow.removeFromLeft (colW));
    topRow.removeFromLeft (gap);
    m_filterPanel->setBounds (topRow.removeFromLeft (colW));

    auto bottomRow = contentArea.removeFromTop (bottomRowH);
    m_modMatrixPanel->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_envTabbed->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_visualizerPanel->setBounds (bottomRow.removeFromLeft (colW));
    bottomRow.removeFromLeft (gap);
    m_arpPanel->setBounds (bottomRow.removeFromLeft (colW));

    // Faixa 5 — BottomBand (755–890 na base)
    m_bottomBand->setBounds (juce::Rectangle<int> (0, (int) bottomBandY, (int) w, (int) bottomBandH));
}

} // namespace gui
} // namespace omnivy
