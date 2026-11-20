#include "PluginEditor.h"

#include "GUI/DesignSystem.h"
#include "GUI/OscillatorPanel.h"
#include "GUI/WavetablePanel.h"
#include "GUI/RoutingPanel.h"
#include "GUI/FilterPanel.h"
#include "GUI/EnvelopePanel.h"
#include "GUI/VisualizerPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <memory>
#include <random>

#include "Parameters/ParameterIDs.h"

namespace omnivy {
namespace gui {

// ============================================================================
// TopBarComponent
// Faixa 1 — Barra superior: logo, presets, controles de arquivo e indicadores.
// ============================================================================

class TopBarComponent : public juce::Component, private juce::Timer
{
public:
    TopBarComponent (OmnivyAudioProcessor& processor)
        : m_processor (processor)
    {
        setInterceptsMouseClicks (true, true);

        m_wordmark.setText ("OMNIVY", juce::dontSendNotification);
        m_wordmark.setFont (uiFont (19.0f, juce::Font::bold));
        m_wordmark.setColour (juce::Label::textColourId, TextMain);
        m_wordmark.setJustificationType (juce::Justification::left);
        m_wordmark.setEnabled (false);
        addAndMakeVisible (m_wordmark);

        m_tagline.setText ("Dark Neon Cyber-Synth", juce::dontSendNotification);
        m_tagline.setFont (uiFont (FontSizeMicro));
        m_tagline.setColour (juce::Label::textColourId, TextMuted);
        m_tagline.setJustificationType (juce::Justification::left);
        m_tagline.setEnabled (false);
        addAndMakeVisible (m_tagline);

        m_presetPrev.setButtonText ("◀");
        m_presetPrev.setEnabled (false);
        m_presetPrev.setTooltip ("Nenhum sistema de presets implementado ainda.");
        addAndMakeVisible (m_presetPrev);

        m_presetNext.setButtonText ("▶");
        m_presetNext.setEnabled (false);
        m_presetNext.setTooltip ("Nenhum sistema de presets implementado ainda.");
        addAndMakeVisible (m_presetNext);

        m_presetName.setText ("Estado atual", juce::dontSendNotification);
        m_presetName.setFont (uiFont (FontSizeLabel, juce::Font::bold));
        m_presetName.setColour (juce::Label::textColourId, TextSecondary);
        m_presetName.setJustificationType (juce::Justification::centred);
        m_presetName.setEnabled (false);
        addAndMakeVisible (m_presetName);

        m_favorite.setToggleState (false, juce::dontSendNotification);
        m_favorite.setTooltip ("Favorito (apenas esta sessão).");
        addAndMakeVisible (m_favorite);

        m_save.setButtonText ("Salvar");
        m_save.setTooltip ("Salva o estado atual dos parâmetros em um arquivo .omnivy.");
        m_save.onClick = [this] { savePreset(); };
        addAndMakeVisible (m_save);

        m_delete.setButtonText ("Excluir");
        m_delete.setEnabled (false);
        m_delete.setTooltip ("Nenhum preset para excluir.");
        addAndMakeVisible (m_delete);

        m_undo.setButtonText ("Desfazer");
        m_undo.setEnabled (false);
        m_undo.setTooltip ("Undo/Redo não suportado nesta versão.");
        addAndMakeVisible (m_undo);

        m_redo.setButtonText ("Refazer");
        m_redo.setEnabled (false);
        m_redo.setTooltip ("Undo/Redo não suportado nesta versão.");
        addAndMakeVisible (m_redo);

        m_randomize.setButtonText ("Randomizar");
        m_randomize.setTooltip ("Atribui valores válidos aleatórios aos parâmetros do sintetizador.");
        m_randomize.onClick = [this] { randomize(); };
        addAndMakeVisible (m_randomize);

        m_polyphonyLabel.setText ("Poly: --", juce::dontSendNotification);
        m_polyphonyLabel.setFont (uiFont (FontSizeMicro));
        m_polyphonyLabel.setColour (juce::Label::textColourId, TextMuted);
        m_polyphonyLabel.setJustificationType (juce::Justification::right);
        m_polyphonyLabel.setEnabled (false);
        addAndMakeVisible (m_polyphonyLabel);

        m_voicesLabel.setText ("Voices: 0", juce::dontSendNotification);
        m_voicesLabel.setFont (uiFont (FontSizeMicro));
        m_voicesLabel.setColour (juce::Label::textColourId, TextSecondary);
        m_voicesLabel.setJustificationType (juce::Justification::right);
        m_voicesLabel.setEnabled (false);
        addAndMakeVisible (m_voicesLabel);

        m_settings.setButtonText ("Configurações");
        m_settings.setTooltip ("Abre informações de build (configurações em implementação).");
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
        auto area = getLocalBounds().reduced (10);
        const int gap = 10;

        m_wordmark.setBounds (area.removeFromLeft (130));
        area.removeFromTop (2);
        m_tagline.setBounds (area.removeFromLeft (130));
        area.removeFromLeft (gap);

        m_presetPrev.setBounds (area.removeFromLeft (26));
        area.removeFromLeft (gap / 2);
        m_presetNext.setBounds (area.removeFromLeft (26));
        area.removeFromLeft (gap);

        m_presetName.setBounds (area.removeFromRight (150));

        m_favorite.setBounds (area.removeFromLeft (26));
        area.removeFromLeft (gap / 2);
        m_save.setBounds (area.removeFromLeft (56));
        area.removeFromLeft (gap / 2);
        m_delete.setBounds (area.removeFromLeft (56));
        area.removeFromLeft (gap / 2);
        m_undo.setBounds (area.removeFromLeft (56));
        area.removeFromLeft (gap / 2);
        m_redo.setBounds (area.removeFromLeft (56));
        area.removeFromLeft (gap);
        m_randomize.setBounds (area.removeFromLeft (80));
        area.removeFromLeft (gap);

        m_voicesLabel.setBounds (area.removeFromRight (70));
        area.removeFromRight (gap);
        m_polyphonyLabel.setBounds (area.removeFromRight (70));
        area.removeFromRight (gap);

        m_settings.setBounds (area.removeFromRight (90));
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

    juce::Label m_wordmark;
    juce::Label m_tagline;
    juce::TextButton m_presetPrev { "◀" };
    juce::TextButton m_presetNext { "▶" };
    juce::Label m_presetName;
    IconButtonComponent m_favorite { "★", "Favorito", MacroColour };
    juce::TextButton m_save { "Salvar" };
    juce::TextButton m_delete { "Excluir" };
    juce::TextButton m_undo { "Desfazer" };
    juce::TextButton m_redo { "Refazer" };
    juce::TextButton m_randomize { "Randomizar" };
    juce::Label m_polyphonyLabel;
    juce::Label m_voicesLabel;
    juce::TextButton m_settings { "Configurações" };

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

OmnivyAudioProcessorEditor::OmnivyAudioProcessorEditor (OmnivyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1200, 760);
    setLookAndFeel (&m_lookAndFeel);

    using namespace omnivy::gui;

    m_tabbed.setTabBarDepth (TabBarHeight);

    m_topBar = new TopBarComponent (audioProcessor);
    addAndMakeVisible (*m_topBar);

    buildTabs();
    m_tabbed.setCurrentTabIndex (TabOscillators);
    addAndMakeVisible (m_tabbed);

    m_masterBand = new MasterBandComponent (audioProcessor);
    addAndMakeVisible (*m_masterBand);
}

OmnivyAudioProcessorEditor::~OmnivyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OmnivyAudioProcessorEditor::buildTabs()
{
    using namespace omnivy::gui;

    m_tabbed.addTab ("Osciladores", PanelDark,
                     new OscillatorPanel (audioProcessor.apvts), true);
    m_tabbed.addTab ("Wavetable", PanelDark,
                     new WavetablePanel (audioProcessor.apvts), true);
    m_tabbed.addTab ("Roteamento", PanelDark,
                     new RoutingPanel (audioProcessor), true);
    m_tabbed.addTab ("Filtro", PanelDark,
                     new FilterPanel (audioProcessor.apvts), true);
    m_tabbed.addTab ("Envelope", PanelDark,
                     new EnvelopePanel (audioProcessor.apvts), true);
    m_tabbed.addTab ("Visualizer", PanelDark,
                     new VisualizerPanel (audioProcessor.apvts), true);
}

void OmnivyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (omnivy::gui::BackgroundMain);
}

void OmnivyAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    m_topBar->setBounds (area.removeFromTop (omnivy::gui::TopBarHeight));
    m_masterBand->setBounds (area.removeFromBottom (86));
    m_tabbed.setBounds (area);
}
