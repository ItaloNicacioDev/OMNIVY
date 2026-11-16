#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>

#include "DSP/Routing/Connection.h"

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Immutable routing configuration published to the audio thread.
// Once constructed, it is never modified — only replaced wholesale.
// This lets the audio thread read lock-free without mutexes.
// -----------------------------------------------------------------------------
struct RoutingConfig
{
    std::vector<std::shared_ptr<const Connection>> connections;
};

// -----------------------------------------------------------------------------
// RoutingGraph manages typed connections between oscillator nodes.
//
// Threading model:
//   - Graph modifications (add/remove/enable/gain) happen on the message
//     thread. Cycle validation runs there (DFS is NOT audio-thread safe).
//   - The audio thread reads an atomic snapshot (shared_ptr<const RoutingConfig>)
//     and applies modulations without allocations, mutexes, or graph searches.
//
// Node IDs:
//   - 0..5 = oscillator slots 1..6 within a voice
//   - 6+   = reserved for future mixer/bus nodes
// -----------------------------------------------------------------------------
class RoutingGraph
{
public:
    RoutingGraph();
    ~RoutingGraph() = default;

    RoutingGraph (const RoutingGraph&) = delete;
    RoutingGraph& operator= (const RoutingGraph&) = delete;

    // --- Node binding (setup only, not audio thread) -----------------------
    // Bind an OscillatorEngine to a node ID. Node 0..5 map to OSC 1..6.
    void bindEngine (NodeId nodeId, OscillatorEngine* engine);
    OscillatorEngine* getEngine (NodeId nodeId) const noexcept;

    // --- Graph modification (message thread only) --------------------------
    // Add a connection. Returns false if the connection is invalid
    // (bad node IDs, would create an audio-critical cycle, or is a
    // duplicate of an existing same-type connection).
    bool addConnection (std::shared_ptr<Connection> connection);

    // Remove a connection matching source/destination/type. Returns false
    // if no such connection exists.
    bool removeConnection (NodeId source, NodeId destination, ConnectionType type);

    void setConnectionEnabled (NodeId source, NodeId destination,
                               ConnectionType type, bool enabled);
    void setConnectionGain (NodeId source, NodeId destination,
                            ConnectionType type, float gain);

    void clearAllConnections();

    // --- Query (message thread) --------------------------------------------
    bool hasConnection (NodeId source, NodeId destination,
                        ConnectionType type) const noexcept;

    std::vector<std::shared_ptr<const Connection>>
    getConnectionsForNode (NodeId nodeId) const;

    std::vector<std::shared_ptr<const Connection>>
    getIncomingConnections (NodeId nodeId) const;

    std::vector<std::shared_ptr<const Connection>>
    getOutgoingConnections (NodeId nodeId) const;

    int getNumConnections() const noexcept;

    // --- Publish / consume --------------------------------------------------
    // Called on the message thread after graph modifications. Builds an
    // immutable snapshot and publishes it atomically to the audio thread.
    void publishConfig();

    // Called on the audio thread. Returns the current immutable snapshot.
    std::shared_ptr<const RoutingConfig> getCurrentConfig() const noexcept;

    // --- Signal transport (audio thread) -----------------------------------
    // Zero all modulation inputs on all bound engines. Call once per block
    // before applyModulations().
    void resetAllModInputs() noexcept;

    // Apply all enabled connections from the current snapshot to bound
    // engines. Multiple connections to the same destination/type accumulate.
    // Call once per block, after resetAllModInputs() and before rendering.
    void applyModulations() noexcept;

private:
    static constexpr int MaxNodes = 16;

    // Cycle detection: DFS from source through existing outgoing connections
    // plus the proposed new edge. Returns true if adding source->destination
    // would create a cycle (destination can reach source).
    bool wouldCreateCycle (NodeId source, NodeId destination) const;
    bool hasPathDFS (NodeId from, NodeId to,
                     std::vector<bool>& visited) const;

    // Find an existing connection matching source/destination (type ignored
    // under the one-connection-per-pair contract).
    int findConnectionIndex (NodeId source, NodeId destination) const;

    // Node bindings (setup only).
    OscillatorEngine* m_engines[MaxNodes] = { nullptr };

    // Working configuration built on the message thread.
    std::vector<std::shared_ptr<Connection>> m_workingConnections;

    // Immutable snapshot published to the audio thread.
    std::atomic<std::shared_ptr<const RoutingConfig>> m_currentConfig;
};

} // namespace dsp
} // namespace omnivy
