#pragma once

#include "DSP/Engine/OscillatorEngine.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Factory that creates an OscillatorEngine pre-configured for a given engine
// type. Adding a new engine requires only extending this factory and adding
// the new oscillator class — no existing engine or slot code is touched.
// -----------------------------------------------------------------------------
class OscillatorEngineFactory
{
public:
    static std::unique_ptr<OscillatorEngine> create (EngineType type,
                                                     dsp::WavetableData* wavetableData)
    {
        auto engine = std::make_unique<OscillatorEngine>();
        engine->init (wavetableData);
        engine->setEngineType (type);
        return engine;
    }

    // Create an engine pre-configured for a specific synthesis type with
    // sensible defaults. Used when the user selects an engine type for an
    // oscillator slot.
    static std::unique_ptr<OscillatorEngine> createForType (EngineType type,
                                                            dsp::WavetableData* wavetableData)
    {
        auto engine = create (type, wavetableData);
        if (type == EngineType::FM)
        {
            // FM defaults: sine-like operator with moderate modulation index.
            engine->setWaveform (Waveform::Sine);
        }
        return engine;
    }
};

} // namespace dsp
} // namespace omnivy
