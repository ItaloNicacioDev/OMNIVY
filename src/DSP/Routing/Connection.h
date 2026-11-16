#pragma once

#include <cstdint>
#include <memory>

#include "DSP/Engine/OscillatorEngine.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Typed connection types. Each type has distinct processing semantics;
// adding a new type requires only a new Connection subclass, no changes to
// existing engines or other connection types.
// -----------------------------------------------------------------------------
enum class ConnectionType : int
{
    None = 0,
    Audio,              // Source audio summed into destination external input
    FrequencyModulation, // Source audio added to destination frequency mod input
    PhaseModulation,     // Source audio added to destination phase mod input
    RingModulation,      // Source audio multiplies destination output
    HardSync,            // Source zero-crossing resets destination phase

    Count
};

using NodeId = int32_t;
static constexpr NodeId InvalidNodeId = -1;

// -----------------------------------------------------------------------------
// Abstract connection between two nodes in the routing graph.
// The graph owns connections and applies them on the audio thread via the
// virtual apply methods. Each derived class overrides only the methods
// relevant to its type; others are no-op.
// -----------------------------------------------------------------------------
class Connection
{
public:
    Connection (NodeId source, NodeId destination, ConnectionType type, float gain);
    virtual ~Connection() = default;

    Connection (const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;

    NodeId getSource() const noexcept { return m_source; }
    NodeId getDestination() const noexcept { return m_destination; }
    ConnectionType getType() const noexcept { return m_type; }
    float getGain() const noexcept { return m_gain; }
    bool isEnabled() const noexcept { return m_enabled; }

    void setGain (float gain) noexcept { m_gain = gain; }
    void setEnabled (bool enabled) noexcept { m_enabled = enabled; }

    // --- Apply this connection to a destination engine ----------------------
    // sourceValue is the value read from the source engine by readSourceValue().
    // Default implementations are no-op.
    virtual void applyExternalAudio (OscillatorEngine& dest, float sourceValue) const noexcept;
    virtual void applyFrequencyMod (OscillatorEngine& dest, float sourceValue) const noexcept;
    virtual void applyPhaseMod (OscillatorEngine& dest, float sourceValue) const noexcept;
    virtual void applyRingMod (OscillatorEngine& dest, float sourceValue) const noexcept;
    virtual void applyHardSync (OscillatorEngine& dest, bool sourceTrigger) const noexcept;

    // --- Read the value this connection type extracts from a source engine --
    virtual float readSourceValue (const OscillatorEngine& source) const noexcept;
    virtual bool readSourceTrigger (const OscillatorEngine& source) const noexcept;

private:
    NodeId m_source;
    NodeId m_destination;
    ConnectionType m_type;
    float m_gain;
    bool m_enabled;
};

// -----------------------------------------------------------------------------
// Typed connection implementations.
// -----------------------------------------------------------------------------
class AudioConnection : public Connection
{
public:
    AudioConnection (NodeId source, NodeId destination, float gain = 1.0f)
        : Connection (source, destination, ConnectionType::Audio, gain) {}

    void applyExternalAudio (OscillatorEngine& dest, float sourceValue) const noexcept override;
    float readSourceValue (const OscillatorEngine& source) const noexcept override;
};

class FrequencyModulationConnection : public Connection
{
public:
    FrequencyModulationConnection (NodeId source, NodeId destination, float gain = 1.0f)
        : Connection (source, destination, ConnectionType::FrequencyModulation, gain) {}

    void applyFrequencyMod (OscillatorEngine& dest, float sourceValue) const noexcept override;
    float readSourceValue (const OscillatorEngine& source) const noexcept override;
};

class PhaseModulationConnection : public Connection
{
public:
    PhaseModulationConnection (NodeId source, NodeId destination, float gain = 1.0f)
        : Connection (source, destination, ConnectionType::PhaseModulation, gain) {}

    void applyPhaseMod (OscillatorEngine& dest, float sourceValue) const noexcept override;
    float readSourceValue (const OscillatorEngine& source) const noexcept override;
};

class RingModulationConnection : public Connection
{
public:
    RingModulationConnection (NodeId source, NodeId destination, float gain = 1.0f)
        : Connection (source, destination, ConnectionType::RingModulation, gain) {}

    void applyRingMod (OscillatorEngine& dest, float sourceValue) const noexcept override;
    float readSourceValue (const OscillatorEngine& source) const noexcept override;
};

class HardSyncConnection : public Connection
{
public:
    HardSyncConnection (NodeId source, NodeId destination, float gain = 1.0f)
        : Connection (source, destination, ConnectionType::HardSync, gain) {}

    void applyHardSync (OscillatorEngine& dest, bool sourceTrigger) const noexcept override;
    bool readSourceTrigger (const OscillatorEngine& source) const noexcept override;
};

} // namespace dsp
} // namespace omnivy
