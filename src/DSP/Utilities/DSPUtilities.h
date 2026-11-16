#pragma once

#include <cmath>
#include <algorithm>
#include <limits>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Mathematical constants
// -----------------------------------------------------------------------------
inline constexpr double Pi = 3.14159265358979323846;
inline constexpr double TwoPi = 6.28318530717958647692;
inline constexpr double PiOverTwo = 1.57079632679489661923;
inline constexpr double InvTwoPi = 0.15915494309189533577;

// -----------------------------------------------------------------------------
// MIDI / frequency helpers
// -----------------------------------------------------------------------------
inline constexpr double MidiNoteToFrequency (double midiNote) noexcept
{
    return 440.0 * std::pow (2.0, (midiNote - 69.0) / 12.0);
}

inline constexpr double FrequencyToMidi (double frequencyHz) noexcept
{
    return 69.0 + 12.0 * std::log2 (frequencyHz / 440.0);
}

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------
template <typename T>
inline T clamp (T value, T minValue, T maxValue) noexcept
{
    return std::min (std::max (value, minValue), maxValue);
}

inline float wrapPhase (float phase) noexcept
{
    while (phase >= 1.0f) phase -= 1.0f;
    while (phase < 0.0f) phase += 1.0f;
    return phase;
}

// Cents to ratio (100 cents = 1 semitone)
inline constexpr float centsToRatio (float cents) noexcept
{
    return static_cast<float> (std::pow (2.0, cents / 1200.0));
}

// -----------------------------------------------------------------------------
// Waveform selection shared by all oscillators
// -----------------------------------------------------------------------------
enum class Waveform : int
{
    Sine = 0,
    Saw,
    Square,
    Triangle,
    Pulse,
    Noise,

    Count
};

inline const char* waveformToString (Waveform w) noexcept
{
    switch (w)
    {
        case Waveform::Sine:     return "Sine";
        case Waveform::Saw:      return "Saw";
        case Waveform::Square:   return "Square";
        case Waveform::Triangle: return "Triangle";
        case Waveform::Pulse:    return "Pulse";
        case Waveform::Noise:    return "Noise";
        default:                 return "Unknown";
    }
}

inline Waveform waveformFromString (const juce::String& name) noexcept
{
    if (name.equalsIgnoreCase ("Sine"))     return Waveform::Sine;
    if (name.equalsIgnoreCase ("Saw"))      return Waveform::Saw;
    if (name.equalsIgnoreCase ("Square"))   return Waveform::Square;
    if (name.equalsIgnoreCase ("Triangle")) return Waveform::Triangle;
    if (name.equalsIgnoreCase ("Pulse"))    return Waveform::Pulse;
    if (name.equalsIgnoreCase ("Noise"))    return Waveform::Noise;
    return Waveform::Sine;
}

// -----------------------------------------------------------------------------
// Stereo frame used by oscillator / voice rendering
// -----------------------------------------------------------------------------
struct StereoFrame
{
    float left = 0.0f;
    float right = 0.0f;
};

// Constant-power pan: pan in [-1, 1] (left .. right)
inline StereoFrame applyPan (float mono, float pan) noexcept
{
    pan = clamp (pan, -1.0f, 1.0f);
    const double angle = (pan + 1.0f) * PiOverTwo * 0.5;
    const float gainL = static_cast<float> (std::cos (angle));
    const float gainR = static_cast<float> (std::sin (angle));
    return StereoFrame { mono * gainL, mono * gainR };
}

} // namespace dsp
} // namespace omnivy
