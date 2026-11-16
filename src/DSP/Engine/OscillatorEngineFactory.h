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
};

} // namespace dsp
} // namespace omnivy
