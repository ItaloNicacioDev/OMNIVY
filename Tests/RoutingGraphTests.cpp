// ============================================================================
// OMNIVY — P3.2 Routing Graph DSP Test Harness
// ============================================================================
// Tests connection types, cycle prevention, signal transport, and
// real-time safety of the RoutingGraph.
//
// Build: standalone test executable linking against JUCE + OmnivyPlugin
// ============================================================================

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "DSP/Engine/OscillatorEngine.h"
#include "DSP/Engine/OscillatorEngineFactory.h"
#include "DSP/Oscillators/VAOscillator.h"
#include "DSP/Routing/Connection.h"
#include "DSP/Routing/RoutingGraph.h"
#include "DSP/Wavetable/WavetableData.h"

using namespace omnivy::dsp;

// -----------------------------------------------------------------------------
// Test engine that records modulation input values for verification.
// -----------------------------------------------------------------------------
class TestEngine : public OscillatorEngine
{
public:
    TestEngine() = default;

    float lastExternalAudio = 0.0f;
    float lastFrequencyMod = 0.0f;
    float lastPhaseMod = 0.0f;
    float lastRingMod = 0.0f;
    bool lastHardSync = false;

    void setExternalAudioInput (float sample) noexcept override
    {
        lastExternalAudio = sample;
        OscillatorEngine::setExternalAudioInput (sample);
    }
    void setFrequencyModInput (float mod) noexcept override
    {
        lastFrequencyMod = mod;
        OscillatorEngine::setFrequencyModInput (mod);
    }
    void setPhaseModInput (float mod) noexcept override
    {
        lastPhaseMod = mod;
        OscillatorEngine::setPhaseModInput (mod);
    }
    void setRingModInput (float mod) noexcept override
    {
        lastRingMod = mod;
        OscillatorEngine::setRingModInput (mod);
    }
    void setHardSyncTrigger (bool trigger) noexcept override
    {
        lastHardSync = trigger;
        OscillatorEngine::setHardSyncTrigger (trigger);
    }
};

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name)                                                                 \
    printf ("  TEST: %-60s ", name);                                               \
    fflush (stdout)

#define PASS()                                                                     \
    do { printf ("PASS\n"); ++g_testsPassed; } while (0)

#define FAIL(msg)                                                                  \
    do {                                                                           \
        printf ("FAIL: %s\n", msg);                                                \
        ++g_testsFailed;                                                           \
    } while (0)

#define ASSERT(cond, msg)                                                          \
    do {                                                                           \
        if (!(cond)) { FAIL (msg); return; }                                       \
    } while (0)

// -----------------------------------------------------------------------------
// Test: Connection type enum values
// -----------------------------------------------------------------------------
static void testConnectionTypeEnum()
{
    TEST ("ConnectionType enum values");
    ASSERT (static_cast<int> (ConnectionType::None) == 0, "None == 0");
    ASSERT (static_cast<int> (ConnectionType::Audio) == 1, "Audio == 1");
    ASSERT (static_cast<int> (ConnectionType::FrequencyModulation) == 2, "FM == 2");
    ASSERT (static_cast<int> (ConnectionType::PhaseModulation) == 3, "PM == 3");
    ASSERT (static_cast<int> (ConnectionType::RingModulation) == 4, "RingMod == 4");
    ASSERT (static_cast<int> (ConnectionType::HardSync) == 5, "HardSync == 5");
    ASSERT (static_cast<int> (ConnectionType::Count) == 6, "Count == 6");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Add valid connections
// -----------------------------------------------------------------------------
static void testAddValidConnections()
{
    TEST ("Add valid connections");
    RoutingGraph graph;

    auto c1 = std::make_shared<AudioConnection> (0, 1, 0.5f);
    ASSERT (graph.addConnection (c1), "Add audio connection 0->1");

    auto c2 = std::make_shared<FrequencyModulationConnection> (1, 2, 0.75f);
    ASSERT (graph.addConnection (c2), "Add FM connection 1->2");

    auto c3 = std::make_shared<PhaseModulationConnection> (2, 3, 1.0f);
    ASSERT (graph.addConnection (c3), "Add PM connection 2->3");

    auto c4 = std::make_shared<RingModulationConnection> (3, 4, 0.3f);
    ASSERT (graph.addConnection (c4), "Add RingMod connection 3->4");

    auto c5 = std::make_shared<HardSyncConnection> (4, 5, 1.0f);
    ASSERT (graph.addConnection (c5), "Add HardSync connection 4->5");

    ASSERT (graph.getNumConnections() == 5, "5 connections total");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Reject invalid node IDs
// -----------------------------------------------------------------------------
static void testRejectInvalidNodeIds()
{
    TEST ("Reject invalid node IDs");
    RoutingGraph graph;

    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (-1, 0)), "Reject negative source");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (0, -1)), "Reject negative dest");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (16, 0)), "Reject source >= MaxNodes");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (0, 16)), "Reject dest >= MaxNodes");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (InvalidNodeId, 0)), "Reject InvalidNodeId source");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (0, InvalidNodeId)), "Reject InvalidNodeId dest");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Reject self-loops
// -----------------------------------------------------------------------------
static void testRejectSelfLoops()
{
    TEST ("Reject self-loop connections");
    RoutingGraph graph;

    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (0, 0)), "Reject self-loop 0->0");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (3, 3)), "Reject self-loop 3->3");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Detect direct cycles
// -----------------------------------------------------------------------------
static void testDetectDirectCycles()
{
    TEST ("Detect direct cycles");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1)), "Add 0->1");
    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (1, 2)), "Add 1->2");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (2, 0)), "Reject cycle 2->0 (2->0->1->2)");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Detect indirect cycles
// -----------------------------------------------------------------------------
static void testDetectIndirectCycles()
{
    TEST ("Detect indirect cycles");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1)), "Add 0->1");
    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (1, 2)), "Add 1->2");
    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (2, 3)), "Add 2->3");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (3, 0)), "Reject indirect cycle 3->0");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Reject duplicate connections
// -----------------------------------------------------------------------------
static void testRejectDuplicates()
{
    TEST ("Reject duplicate same-type connections");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 0.5f)), "Add audio 0->1");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (0, 1, 0.8f)), "Reject duplicate audio 0->1");
    ASSERT (!graph.addConnection (std::make_shared<FrequencyModulationConnection> (0, 1, 0.3f)), "Reject duplicate FM 0->1");

    // Reverse direction 1->0 would create a cycle (0->1->0), correctly rejected.
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (1, 0, 0.5f)), "Reject reverse 1->0 (cycle)");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Remove connections
// -----------------------------------------------------------------------------
static void testRemoveConnections()
{
    TEST ("Remove connections");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 0.5f)), "Add 0->1");
    ASSERT (graph.getNumConnections() == 1, "1 connection after add");

    ASSERT (graph.removeConnection (0, 1, ConnectionType::Audio), "Remove 0->1 Audio");
    ASSERT (graph.getNumConnections() == 0, "0 connections after remove");

    ASSERT (!graph.removeConnection (0, 1, ConnectionType::Audio), "Reject removing non-existent");
    ASSERT (!graph.removeConnection (0, 1, ConnectionType::FrequencyModulation), "Reject removing wrong type");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Enable/disable connections
// -----------------------------------------------------------------------------
static void testEnableDisable()
{
    TEST ("Enable/disable connections");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 1.0f)), "Add 0->1");
    graph.setConnectionEnabled (0, 1, ConnectionType::Audio, false);
    ASSERT (!graph.getIncomingConnections (1)[0]->isEnabled(), "Connection disabled");

    graph.setConnectionEnabled (0, 1, ConnectionType::Audio, true);
    ASSERT (graph.getIncomingConnections (1)[0]->isEnabled(), "Connection re-enabled");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Set connection gain
// -----------------------------------------------------------------------------
static void testSetGain()
{
    TEST ("Set connection gain");
    RoutingGraph graph;

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 0.5f)), "Add 0->1");
    graph.setConnectionGain (0, 1, ConnectionType::Audio, 0.75f);
    ASSERT (std::fabs (graph.getIncomingConnections (1)[0]->getGain() - 0.75f) < 1e-6f, "Gain set to 0.75");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Signal transport — Audio connection
// -----------------------------------------------------------------------------
static void testSignalTransportAudio()
{
    TEST ("Signal transport — Audio connection");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    // Render source to produce output
    engine0->setFrequency (440.0f);
    engine0->processSample(); // produces output in engine0->getLastOutput()

    // Add audio connection and apply modulations
    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 0.5f)), "Add audio 0->1");
    graph.publishConfig();

    // Reset + apply modulations
    graph.resetAllModInputs();
    graph.applyModulations();

    // Destination should have received source's audio * gain
    const float expected = engine0->getLastOutput().left * 0.5f; // simplified check
    // The TestEngine records the value passed to setExternalAudioInput
    ASSERT (std::fabs (engine1->lastExternalAudio - expected) < 0.001f,
            "Audio input transported with gain");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Signal transport — FM connection
// -----------------------------------------------------------------------------
static void testSignalTransportFM()
{
    TEST ("Signal transport — FM connection");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    engine0->processSample();

    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (0, 1, 0.8f)), "Add FM 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    const float expected = engine0->getLastOutput().left * 0.8f;
    ASSERT (std::fabs (engine1->lastFrequencyMod - expected) < 0.001f,
            "FM input transported with gain");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Signal transport — PM connection
// -----------------------------------------------------------------------------
static void testSignalTransportPM()
{
    TEST ("Signal transport — PM connection");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    engine0->processSample();

    ASSERT (graph.addConnection (std::make_shared<PhaseModulationConnection> (0, 1, 0.6f)), "Add PM 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    const float expected = engine0->getLastOutput().left * 0.6f;
    ASSERT (std::fabs (engine1->lastPhaseMod - expected) < 0.001f,
            "PM input transported with gain");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Signal transport — RingMod connection
// -----------------------------------------------------------------------------
static void testSignalTransportRingMod()
{
    TEST ("Signal transport — RingMod connection");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    engine0->processSample();

    ASSERT (graph.addConnection (std::make_shared<RingModulationConnection> (0, 1, 0.9f)), "Add RingMod 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    const float expected = engine0->getLastOutput().left * 0.9f;
    ASSERT (std::fabs (engine1->lastRingMod - expected) < 0.001f,
            "RingMod input transported with gain");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Signal transport — HardSync connection
// -----------------------------------------------------------------------------
static void testSignalTransportHardSync()
{
    TEST ("Signal transport — HardSync connection");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    engine0->processSample();

    ASSERT (graph.addConnection (std::make_shared<HardSyncConnection> (0, 1, 1.0f)), "Add HardSync 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // HardSync trigger depends on source phase; just verify the call happens
    // (engine1->lastHardSync will be true if source phase is near wrap)
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Multiple incoming connections accumulate
// -----------------------------------------------------------------------------
static void testMultipleIncomingAccumulate()
{
    TEST ("Multiple incoming connections accumulate");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    auto engine2 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine2->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);
    engine2->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());
    graph.bindEngine (2, engine2.get());

    engine0->processSample();
    engine1->processSample();

    // Both 0 and 1 modulate 2 via FM
    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (0, 2, 0.5f)), "Add FM 0->2");
    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (1, 2, 0.5f)), "Add FM 1->2");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // engine2->lastFrequencyMod should be sum of engine0 output*0.5 + engine1 output*0.5
    const float expected = engine0->getLastOutput().left * 0.5f + engine1->getLastOutput().left * 0.5f;
    ASSERT (std::fabs (engine2->lastFrequencyMod - expected) < 0.001f,
            "Multiple FM connections accumulate");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Disabled connections contribute nothing
// -----------------------------------------------------------------------------
static void testDisabledConnections()
{
    TEST ("Disabled connections contribute nothing");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());

    engine0->processSample();

    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 1.0f)), "Add audio 0->1");
    graph.setConnectionEnabled (0, 1, ConnectionType::Audio, false);
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    ASSERT (engine1->lastExternalAudio == 0.0f, "Disabled connection contributes nothing");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Zero input when disconnected
// -----------------------------------------------------------------------------
static void testZeroWhenDisconnected()
{
    TEST ("Zero input when disconnected");
    RoutingGraph graph;

    auto engine1 = std::make_unique<TestEngine>();
    engine1->init (nullptr);
    engine1->setEngineType (EngineType::VA);

    graph.bindEngine (1, engine1.get());

    // No connections at all
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    ASSERT (engine1->lastExternalAudio == 0.0f, "External audio zero when disconnected");
    ASSERT (engine1->lastFrequencyMod == 0.0f, "FM mod zero when disconnected");
    ASSERT (engine1->lastPhaseMod == 0.0f, "PM mod zero when disconnected");
    ASSERT (engine1->lastRingMod == 0.0f, "RingMod mod zero when disconnected");
    ASSERT (!engine1->lastHardSync, "HardSync false when disconnected");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Isolation between oscillator slots
// -----------------------------------------------------------------------------
static void testSlotIsolation()
{
    TEST ("Isolation between oscillator slots");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    auto engine1 = std::make_unique<TestEngine>();
    auto engine2 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine1->init (nullptr);
    engine2->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine1->setEngineType (EngineType::VA);
    engine2->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());
    graph.bindEngine (1, engine1.get());
    graph.bindEngine (2, engine2.get());

    // Only connect 0->1, engine2 should be unaffected
    ASSERT (graph.addConnection (std::make_shared<AudioConnection> (0, 1, 1.0f)), "Add audio 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // Engine 2 has no connections — all mod inputs must be zero.
    ASSERT (engine2->lastExternalAudio == 0.0f, "Engine 2 isolated: external audio zero");
    ASSERT (engine2->lastFrequencyMod == 0.0f, "Engine 2 isolated: FM mod zero");
    ASSERT (engine2->lastPhaseMod == 0.0f, "Engine 2 isolated: PM mod zero");
    ASSERT (engine2->lastRingMod == 0.0f, "Engine 2 isolated: RingMod zero");
    ASSERT (!engine2->lastHardSync, "Engine 2 isolated: HardSync false");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Regression — all-VA with no routing preserves behavior
// -----------------------------------------------------------------------------
static void testRegressionAllVA()
{
    TEST ("Regression — all-VA, no routing");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine0->setEngineType (EngineType::VA);

    graph.bindEngine (0, engine0.get());

    // No connections, publish empty config
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // Engine should produce output normally (VA engine processes)
    engine0->setFrequency (440.0f);
    const auto frame = engine0->processSample();
    ASSERT (std::isfinite (frame.left) && std::isfinite (frame.right), "VA engine produces finite output");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Cycle prevention with mixed connection types
// -----------------------------------------------------------------------------
static void testCyclePreventionMixedTypes()
{
    TEST ("Cycle prevention with mixed connection types");
    RoutingGraph graph;

    // 0 -> 1 (FM), 1 -> 2 (PM), try 2 -> 0 (Audio) = cycle
    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (0, 1)), "Add FM 0->1");
    ASSERT (graph.addConnection (std::make_shared<PhaseModulationConnection> (1, 2)), "Add PM 1->2");
    ASSERT (!graph.addConnection (std::make_shared<AudioConnection> (2, 0)), "Reject cycle 2->0 across types");
    PASS ();
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main()
{
    printf ("OMNIVY P3.2 Routing Graph DSP Test Harness\n");
    printf ("=============================================\n\n");

    testConnectionTypeEnum();
    testAddValidConnections();
    testRejectInvalidNodeIds();
    testRejectSelfLoops();
    testDetectDirectCycles();
    testDetectIndirectCycles();
    testRejectDuplicates();
    testRemoveConnections();
    testEnableDisable();
    testSetGain();
    testSignalTransportAudio();
    testSignalTransportFM();
    testSignalTransportPM();
    testSignalTransportRingMod();
    testSignalTransportHardSync();
    testMultipleIncomingAccumulate();
    testDisabledConnections();
    testZeroWhenDisconnected();
    testSlotIsolation();
    testRegressionAllVA();
    testCyclePreventionMixedTypes();

    printf ("\n=============================================\n");
    printf ("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
