#pragma once

#include <juce_core/juce_core.h>

namespace omnivy {
namespace params {

// -----------------------------------------------------------------------------
// Stable parameter identifiers used by both the plugin (APVTS / host automation)
// and the DSP engine. These strings must not change between releases if preset
// compatibility is to be preserved.
// -----------------------------------------------------------------------------

// ---- Global / voice ----------------------------------------------------------
inline constexpr char MasterVolume[]       = "masterVolume";
inline constexpr char Polyphony[]          = "polyphony";
inline constexpr char UnisonVoices[]       = "unisonVoices";
inline constexpr char UnisonDetune[]       = "unisonDetune";

// ---- Oscillator (indexed 0..5, maps to OSC 1..6) ----------------------------
// Base prefix per oscillator.
inline const juce::String oscPrefix (int index) { return juce::String ("osc") + juce::String (index + 1); }

inline juce::String oscActive    (int index) { return oscPrefix (index) + ".active"; }
inline juce::String oscWaveform  (int index) { return oscPrefix (index) + ".waveform"; }
inline juce::String oscLevel     (int index) { return oscPrefix (index) + ".level"; }
inline juce::String oscPan       (int index) { return oscPrefix (index) + ".pan"; }
inline juce::String oscOctave    (int index) { return oscPrefix (index) + ".octave"; }
inline juce::String oscSemitone  (int index) { return oscPrefix (index) + ".semitone"; }
inline juce::String oscDetune    (int index) { return oscPrefix (index) + ".detune"; }
inline juce::String oscFineTune  (int index) { return oscPrefix (index) + ".fineTune"; }
inline juce::String oscPhase     (int index) { return oscPrefix (index) + ".phase"; }
inline juce::String oscEngine    (int index) { return oscPrefix (index) + ".engine"; }

// ---- FM synthesis parameters (indexed 0..5, maps to OSC 1..6) ---------------
inline juce::String oscFmRatio       (int index) { return oscPrefix (index) + ".fmRatio"; }
inline juce::String oscFmFineRatio   (int index) { return oscPrefix (index) + ".fmFineRatio"; }
inline juce::String oscFmAmount      (int index) { return oscPrefix (index) + ".fmAmount"; }
inline juce::String oscPmAmount      (int index) { return oscPrefix (index) + ".pmAmount"; }
inline juce::String oscFmFeedback    (int index) { return oscPrefix (index) + ".fmFeedback"; }
inline juce::String oscFmOutputLevel (int index) { return oscPrefix (index) + ".fmOutputLevel"; }

// ---- Additive synthesis parameters (indexed 0..5, maps to OSC 1..6) -------
inline juce::String oscAddHarmonicCount (int index) { return oscPrefix (index) + ".addHarmonicCount"; }
inline juce::String oscAddMorph (int index) { return oscPrefix (index) + ".addMorph"; }
inline juce::String oscAddTilt (int index) { return oscPrefix (index) + ".addTilt"; }
inline juce::String oscAddOddEven (int index) { return oscPrefix (index) + ".addOddEven"; }
inline juce::String oscAddFundLevel (int index) { return oscPrefix (index) + ".addFundLevel"; }
inline juce::String oscAddOutputLevel (int index) { return oscPrefix (index) + ".addOutputLevel"; }

// ---- Granular synthesis parameters (indexed 0..5, maps to OSC 1..6) -------
inline juce::String oscGranGrainSize (int index) { return oscPrefix (index) + ".granGrainSize"; }
inline juce::String oscGranDensity (int index) { return oscPrefix (index) + ".granDensity"; }
inline juce::String oscGranPosition (int index) { return oscPrefix (index) + ".granPosition"; }
inline juce::String oscGranPositionSpray (int index) { return oscPrefix (index) + ".granPositionSpray"; }
inline juce::String oscGranPitch (int index) { return oscPrefix (index) + ".granPitch"; }
inline juce::String oscGranPitchRandomization (int index) { return oscPrefix (index) + ".granPitchRandomization"; }
inline juce::String oscGranGrainEnvelope (int index) { return oscPrefix (index) + ".granGrainEnvelope"; }
inline juce::String oscGranStereoSpread (int index) { return oscPrefix (index) + ".granStereoSpread"; }
inline juce::String oscGranPlaybackDirection (int index) { return oscPrefix (index) + ".granPlaybackDirection"; }

// Total number of independent oscillators.
inline constexpr int NumOscillators = 6;

// ---- Filter ------------------------------------------------------------------
inline constexpr char FilterType[]     = "filterType";
inline constexpr char FilterCutoff[]   = "filterCutoff";
inline constexpr char FilterResonance[]= "filterResonance";
inline constexpr char FilterEnabled[]  = "filterEnabled";
inline constexpr char FilterKeyTrack[] = "filterKeyTrack";

// ---- Amplitude envelope ------------------------------------------------------
inline constexpr char EnvAttack[]  = "envAttack";
inline constexpr char EnvDecay[]   = "envDecay";
inline constexpr char EnvSustain[] = "envSustain";
inline constexpr char EnvRelease[] = "envRelease";

// Range helpers (exposed to the host via APVTS)
inline constexpr float MinLevel  = 0.0f;
inline constexpr float MaxLevel  = 1.0f;
inline constexpr float MinPan    = -1.0f;
inline constexpr float MaxPan    = 1.0f;
inline constexpr int   MinOctave = -2;
inline constexpr int   MaxOctave =  2;
inline constexpr int   MinSemi   = -12;
inline constexpr int   MaxSemi   =  12;
inline constexpr float MinDetune = -50.0f;
inline constexpr float MaxDetune =  50.0f;

} // namespace params
} // namespace omnivy
