#include "DSP/Routing/Connection.h"

#include <cmath>
#include <algorithm>

namespace omnivy {
namespace dsp {

Connection::Connection (NodeId source, NodeId destination,
                        ConnectionType type, float gain)
    : m_source (source),
      m_destination (destination),
      m_type (type),
      m_gain (std::clamp (gain, 0.0f, 2.0f)),
      m_enabled (true)
{
}

// -----------------------------------------------------------------------------
// Base class: default no-op implementations.
// -----------------------------------------------------------------------------
void Connection::applyExternalAudio (OscillatorEngine&, float) const noexcept {}
void Connection::applyFrequencyMod (OscillatorEngine&, float) const noexcept {}
void Connection::applyPhaseMod (OscillatorEngine&, float) const noexcept {}
void Connection::applyRingMod (OscillatorEngine&, float) const noexcept {}
void Connection::applyHardSync (OscillatorEngine&, bool) const noexcept {}

float Connection::readSourceValue (const OscillatorEngine&) const noexcept { return 0.0f; }
bool Connection::readSourceTrigger (const OscillatorEngine&) const noexcept { return false; }

// -----------------------------------------------------------------------------
// AudioConnection — sums source output into destination's external audio input.
// -----------------------------------------------------------------------------
void AudioConnection::applyExternalAudio (OscillatorEngine& dest, float sourceValue) const noexcept
{
    dest.setExternalAudioInput (sourceValue * getGain());
}

float AudioConnection::readSourceValue (const OscillatorEngine& source) const noexcept
{
    const auto frame = source.getLastOutput();
    return (frame.left + frame.right) * 0.5f; // mono sum
}

// -----------------------------------------------------------------------------
// FrequencyModulationConnection — adds to destination's frequency mod input.
// -----------------------------------------------------------------------------
void FrequencyModulationConnection::applyFrequencyMod (OscillatorEngine& dest, float sourceValue) const noexcept
{
    dest.setFrequencyModInput (sourceValue * getGain());
}

float FrequencyModulationConnection::readSourceValue (const OscillatorEngine& source) const noexcept
{
    const auto frame = source.getLastOutput();
    return (frame.left + frame.right) * 0.5f;
}

// -----------------------------------------------------------------------------
// PhaseModulationConnection — adds to destination's phase mod input.
// -----------------------------------------------------------------------------
void PhaseModulationConnection::applyPhaseMod (OscillatorEngine& dest, float sourceValue) const noexcept
{
    dest.setPhaseModInput (sourceValue * getGain());
}

float PhaseModulationConnection::readSourceValue (const OscillatorEngine& source) const noexcept
{
    const auto frame = source.getLastOutput();
    return (frame.left + frame.right) * 0.5f;
}

// -----------------------------------------------------------------------------
// RingModulationConnection — multiplies destination output by source output.
// -----------------------------------------------------------------------------
void RingModulationConnection::applyRingMod (OscillatorEngine& dest, float sourceValue) const noexcept
{
    dest.setRingModInput (sourceValue * getGain());
}

float RingModulationConnection::readSourceValue (const OscillatorEngine& source) const noexcept
{
    const auto frame = source.getLastOutput();
    return (frame.left + frame.right) * 0.5f;
}

// -----------------------------------------------------------------------------
// HardSyncConnection — resets destination phase on source zero-crossing.
// -----------------------------------------------------------------------------
void HardSyncConnection::applyHardSync (OscillatorEngine& dest, bool sourceTrigger) const noexcept
{
    dest.setHardSyncTrigger (sourceTrigger);
}

bool HardSyncConnection::readSourceTrigger (const OscillatorEngine& source) const noexcept
{
    // Simple zero-crossing detector: phase near wrap boundary.
    const float phase = source.getPhase();
    return phase < 0.01f || phase > 0.99f;
}

} // namespace dsp
} // namespace omnivy
