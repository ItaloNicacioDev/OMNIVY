#pragma once

#include "DSP/Wavetable/WavetableData.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Generates standard wavetables (sine, etc.) into WavetableData.
// -----------------------------------------------------------------------------
class WavetableGenerator
{
public:
    // Fills table 0 with a sine wave.
    static void generateSine (WavetableData& data, int tableIndex = 0);

    // Fills table 0 with a band-limited-ish saw approximation (harmonic sum).
    static void generateSaw (WavetableData& data, int tableIndex = 0, int numHarmonics = 32);
};

} // namespace dsp
} // namespace omnivy
