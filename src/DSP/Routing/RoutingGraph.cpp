#include "DSP/Routing/RoutingGraph.h"

#include <algorithm>

namespace omnivy {
namespace dsp {

RoutingGraph::RoutingGraph()
{
    // Initialise the atomic snapshot with an empty config.
    m_currentConfig.store (std::make_shared<const RoutingConfig>(),
                           std::memory_order_release);
}

void RoutingGraph::bindEngine (NodeId nodeId, OscillatorEngine* engine)
{
    if (nodeId < 0 || nodeId >= MaxNodes)
        return;
    m_engines[nodeId] = engine;
}

OscillatorEngine* RoutingGraph::getEngine (NodeId nodeId) const noexcept
{
    if (nodeId < 0 || nodeId >= MaxNodes)
        return nullptr;
    return m_engines[nodeId];
}

bool RoutingGraph::addConnection (std::shared_ptr<Connection> connection)
{
    if (!connection)
        return false;

    const NodeId src = connection->getSource();
    const NodeId dst = connection->getDestination();

    // Validate node IDs.
    if (src == InvalidNodeId || dst == InvalidNodeId ||
        src < 0 || src >= MaxNodes || dst < 0 || dst >= MaxNodes)
        return false;

    // Reject self-loops (trivial cycles).
    if (src == dst)
        return false;

    // Reject duplicate same-type connections.
    if (hasConnection (src, dst, connection->getType()))
        return false;

    // Cycle prevention: adding src->dst creates a cycle if dst can already
    // reach src through existing connections.
    if (wouldCreateCycle (src, dst))
        return false;

    m_workingConnections.push_back (connection);
    return true;
}

bool RoutingGraph::removeConnection (NodeId source, NodeId destination,
                                     ConnectionType type)
{
    const int index = findConnectionIndex (source, destination);
    if (index < 0)
        return false;

    m_workingConnections.erase (m_workingConnections.begin() + index);
    return true;
}

void RoutingGraph::setConnectionEnabled (NodeId source, NodeId destination,
                                         ConnectionType type, bool enabled)
{
    juce::ignoreUnused (type);
    const int index = findConnectionIndex (source, destination);
    if (index >= 0)
        m_workingConnections[index]->setEnabled (enabled);
}

void RoutingGraph::setConnectionGain (NodeId source, NodeId destination,
                                      ConnectionType type, float gain)
{
    juce::ignoreUnused (type);
    const int index = findConnectionIndex (source, destination);
    if (index >= 0)
        m_workingConnections[index]->setGain (gain);
}

void RoutingGraph::clearAllConnections()
{
    m_workingConnections.clear();
}

bool RoutingGraph::hasConnection (NodeId source, NodeId destination,
                                  ConnectionType type) const noexcept
{
    juce::ignoreUnused (type);
    return findConnectionIndex (source, destination) >= 0;
}

std::vector<std::shared_ptr<const Connection>>
RoutingGraph::getConnectionsForNode (NodeId nodeId) const
{
    std::vector<std::shared_ptr<const Connection>> result;
    if (nodeId < 0 || nodeId >= MaxNodes)
        return result;

    for (const auto& conn : m_workingConnections)
    {
        if (!conn)
            continue;
        if (conn->getSource() == nodeId || conn->getDestination() == nodeId)
            result.push_back (conn);
    }
    return result;
}

std::vector<std::shared_ptr<const Connection>>
RoutingGraph::getIncomingConnections (NodeId nodeId) const
{
    std::vector<std::shared_ptr<const Connection>> result;
    if (nodeId < 0 || nodeId >= MaxNodes)
        return result;

    for (const auto& conn : m_workingConnections)
    {
        if (!conn)
            continue;
        if (conn->getDestination() == nodeId)
            result.push_back (conn);
    }
    return result;
}

std::vector<std::shared_ptr<const Connection>>
RoutingGraph::getOutgoingConnections (NodeId nodeId) const
{
    std::vector<std::shared_ptr<const Connection>> result;
    if (nodeId < 0 || nodeId >= MaxNodes)
        return result;

    for (const auto& conn : m_workingConnections)
    {
        if (!conn)
            continue;
        if (conn->getSource() == nodeId)
            result.push_back (conn);
    }
    return result;
}

int RoutingGraph::getNumConnections() const noexcept
{
    return static_cast<int> (m_workingConnections.size());
}

void RoutingGraph::publishConfig()
{
    // Build a non-const RoutingConfig, then publish as immutable shared_ptr.
    RoutingConfig config;
    config.connections.reserve (m_workingConnections.size());
    for (const auto& conn : m_workingConnections)
        config.connections.push_back (conn);

    m_currentConfig.store (std::make_shared<const RoutingConfig> (std::move (config)),
                           std::memory_order_release);
}

std::shared_ptr<const RoutingConfig>
RoutingGraph::getCurrentConfig() const noexcept
{
    return m_currentConfig.load(std::memory_order_acquire);
}

void RoutingGraph::resetAllModInputs() noexcept
{
    for (int i = 0; i < MaxNodes; ++i)
    {
        if (m_engines[i] != nullptr)
            m_engines[i]->resetModInputs();
    }
}

void RoutingGraph::applyModulations() noexcept
{
    auto config = m_currentConfig.load(std::memory_order_acquire);
    if (!config)
        return;

    // Reset all mod inputs on bound engines.
    resetAllModInputs();

    // Accumulators per destination node and connection type.
    // Type index: Audio=0, FM=1, PM=2, RingMod=3.
    float accumulators[MaxNodes][4] = { { 0.0f } };
    bool syncTriggers[MaxNodes] = { false };

    for (const auto& conn : config->connections)
    {
        if (!conn || !conn->isEnabled())
            continue;

        OscillatorEngine* src = m_engines[conn->getSource()];
        OscillatorEngine* dst = m_engines[conn->getDestination()];
        if (!src || !dst)
            continue;

        switch (conn->getType())
        {
            case ConnectionType::Audio:
            case ConnectionType::FrequencyModulation:
            case ConnectionType::PhaseModulation:
            case ConnectionType::RingModulation:
            {
                const int typeIndex =
                    static_cast<int> (conn->getType()) - 1;
                const float value = conn->readSourceValue (*src);
                accumulators[conn->getDestination()][typeIndex] +=
                    value * conn->getGain();
                break;
            }
            case ConnectionType::HardSync:
                syncTriggers[conn->getDestination()] |=
                    conn->readSourceTrigger (*src);
                break;
            default:
                break;
        }
    }

    // Apply accumulated values to destination engines.
    for (int node = 0; node < MaxNodes; ++node)
    {
        OscillatorEngine* engine = m_engines[node];
        if (!engine)
            continue;

        engine->setExternalAudioInput (accumulators[node][0]);
        engine->setFrequencyModInput (accumulators[node][1]);
        engine->setPhaseModInput (accumulators[node][2]);
        engine->setRingModInput (accumulators[node][3]);
        engine->setHardSyncTrigger (syncTriggers[node]);
    }
}

bool RoutingGraph::wouldCreateCycle (NodeId source, NodeId destination) const
{
    // Adding source->destination creates a cycle if destination can reach
    // source through existing connections.
    std::vector<bool> visited (MaxNodes, false);
    return hasPathDFS (destination, source, visited);
}

bool RoutingGraph::hasPathDFS (NodeId from, NodeId to,
                               std::vector<bool>& visited) const
{
    if (from == to)
        return true;
    if (from < 0 || from >= MaxNodes || visited[from])
        return false;
    visited[from] = true;

    for (const auto& conn : m_workingConnections)
    {
        if (!conn || !conn->isEnabled())
            continue;
        if (conn->getSource() == from)
        {
            if (hasPathDFS (conn->getDestination(), to, visited))
                return true;
        }
    }
    return false;
}

int RoutingGraph::findConnectionIndex (NodeId source, NodeId destination) const
{
    juce::ignoreUnused (/*type*/);
    for (int i = 0; i < static_cast<int> (m_workingConnections.size()); ++i)
    {
        const auto& conn = m_workingConnections[i];
        if (conn && conn->getSource() == source && conn->getDestination() == destination)
            return i;
    }
    return -1;
}

} // namespace dsp
} // namespace omnivy
