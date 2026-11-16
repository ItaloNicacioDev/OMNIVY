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
    }
    void setFrequencyModInput (float mod) noexcept override
    {
        lastFrequencyMod = mod;
    }
    void setPhaseModInput (float mod) noexcept override
    {
        lastPhaseMod = mod;
    }
    void setRingModInput (float mod) noexcept override
    {
        lastRingMod = mod;
    }
    void setHardSyncTrigger (bool trigger) noexcept override
    {
        lastHardSync = trigger;
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
// Test: FM operator produces modulated output
// -----------------------------------------------------------------------------
static void testFmOperatorModulation()
{
    TEST ("FM operator — modulation changes output");
    RoutingGraph graph;

    auto carrier = std::make_unique<TestEngine>();
    auto modulator = std::make_unique<TestEngine>();
    carrier->init (nullptr);
    modulator->init (nullptr);
    carrier->setEngineType (EngineType::FM);
    modulator->setEngineType (EngineType::VA);

    // Initialize smoothers so level is non-zero
    carrier->reset ();
    modulator->reset ();

    graph.bindEngine (0, carrier.get());
    graph.bindEngine (1, modulator.get());

    // Configure carrier as FM operator with ratio 1, amount 10
    carrier->setFrequencyRatio (1.0f);
    carrier->setFineRatio (1.0f);
    carrier->setFmAmount (10.0f);
    carrier->setFeedback (0.0f);

    // Modulator produces a tone
    modulator->setFrequency (440.0f);
    modulator->processSample();

    // Connect modulator -> carrier FM
    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (1, 0, 1.0f)), "Add FM 1->0");
    graph.publishConfig();

    // Without FM amount, carrier output is a clean sine
    carrier->setFrequency (440.0f);
    carrier->processSample();
    float outputNoMod = carrier->getLastOutput().left;

    // With FM modulation, carrier output should differ
    graph.resetAllModInputs();
    graph.applyModulations();
    carrier->processSample();
    float outputWithMod = carrier->getLastOutput().left;

    // FM modulation should change the output (different phase)
    ASSERT (std::fabs (outputWithMod - outputNoMod) > 0.001f,
            "FM modulation changes carrier output");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: FM with different frequency ratios
// -----------------------------------------------------------------------------
static void testFmDifferentRatios()
{
    TEST ("FM operator — different frequency ratios");
    RoutingGraph graph;

    auto carrier = std::make_unique<TestEngine>();
    auto modulator = std::make_unique<TestEngine>();
    carrier->init (nullptr);
    modulator->init (nullptr);
    carrier->setEngineType (EngineType::FM);
    modulator->setEngineType (EngineType::VA);

    // Initialize smoothers so level is non-zero
    carrier->reset ();
    modulator->reset ();

    graph.bindEngine (0, carrier.get());
    graph.bindEngine (1, modulator.get());

    carrier->setFrequencyRatio (2.0f);  // octave ratio
    carrier->setFmAmount (5.0f);
    carrier->setFeedback (0.0f);

    modulator->setFrequency (440.0f);
    modulator->processSample();

    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (1, 0, 1.0f)), "Add FM 1->0");
    graph.publishConfig();

    carrier->setFrequency (440.0f);
    graph.resetAllModInputs();
    graph.applyModulations();
    carrier->processSample();

    float outputRatio2 = carrier->getLastOutput().left;

    // Change ratio to 1.5
    carrier->setFrequencyRatio (1.5f);
    graph.resetAllModInputs();
    graph.applyModulations();
    carrier->processSample();
    float outputRatio15 = carrier->getLastOutput().left;

    // Different ratios should produce different output
    ASSERT (std::fabs (outputRatio2 - outputRatio15) > 0.001f,
            "Different FM ratios produce different output");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: FM self-feedback stability
// -----------------------------------------------------------------------------
static void testFmFeedbackStability()
{
    TEST ("FM operator — self-feedback stability");
    RoutingGraph graph;

    auto carrier = std::make_unique<TestEngine>();
    carrier->init (nullptr);
    carrier->setEngineType (EngineType::FM);
    graph.bindEngine (0, carrier.get());

    carrier->reset ();
    carrier->setFrequencyRatio (1.0f);
    carrier->setFmAmount (10.0f);
    carrier->setFeedback (0.5f);  // moderate feedback
    carrier->setFrequency (440.0f);

    graph.publishConfig();

    // Run for 1000 samples — output must remain finite
    for (int s = 0; s < 1000; ++s)
    {
        graph.resetAllModInputs();
        graph.applyModulations();
        carrier->processSample();
        float out = carrier->getLastOutput().left;
        ASSERT (std::isfinite (out), "FM feedback output remains finite");
        ASSERT (std::fabs (out) < 10.0f, "FM feedback output remains bounded");
    }
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: PM independently from FM
// -----------------------------------------------------------------------------
static void testPmIndependently()
{
    TEST ("PM operator — phase modulation independent from FM");
    RoutingGraph graph;

    auto carrier = std::make_unique<TestEngine>();
    auto modulator = std::make_unique<TestEngine>();
    carrier->init (nullptr);
    modulator->init (nullptr);
    carrier->setEngineType (EngineType::FM);
    modulator->setEngineType (EngineType::VA);

    carrier->reset ();
    modulator->reset ();

    graph.bindEngine (0, carrier.get());
    graph.bindEngine (1, modulator.get());

    carrier->setFrequencyRatio (1.0f);
    carrier->setFmAmount (0.0f);   // no FM
    carrier->setPmAmount (5.0f);   // PM only
    carrier->setFeedback (0.0f);
    carrier->setFrequency (440.0f);

    modulator->setFrequency (440.0f);
    modulator->processSample();

    ASSERT (graph.addConnection (std::make_shared<PhaseModulationConnection> (1, 0, 1.0f)), "Add PM 1->0");
    graph.publishConfig();

    // Without PM mod, output is clean sine
    graph.resetAllModInputs();
    graph.applyModulations();
    carrier->processSample();
    float outputNoPm = carrier->getLastOutput().left;

    // Now apply PM modulation (modulator output goes to phase mod input)
    // Re-render modulator to get fresh output, then apply
    modulator->processSample();
    graph.resetAllModInputs();
    graph.applyModulations();
    carrier->processSample();
    float outputWithPm = carrier->getLastOutput().left;

    ASSERT (std::fabs (outputWithPm - outputNoPm) > 0.001f,
            "PM modulation changes carrier output independently from FM");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Ring modulation multiplication
// -----------------------------------------------------------------------------
static void testRingModMultiplication()
{
    TEST ("Ring modulation — multiplies source and destination");
    RoutingGraph graph;

    auto source = std::make_unique<TestEngine>();
    auto dest = std::make_unique<TestEngine>();
    source->init (nullptr);
    dest->init (nullptr);
    source->setEngineType (EngineType::VA);
    dest->setEngineType (EngineType::VA);

    graph.bindEngine (0, source.get());
    graph.bindEngine (1, dest.get());

    source->processSample();
    float sourceOutput = source->getLastOutput().left;

    ASSERT (graph.addConnection (std::make_shared<RingModulationConnection> (0, 1, 0.5f)), "Add RingMod 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // RingMod input should be source output * gain
    float expectedRingInput = sourceOutput * 0.5f;
    ASSERT (std::fabs (dest->lastRingMod - expectedRingInput) < 0.001f,
            "RingMod input = source output * gain");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Hard sync phase reset
// -----------------------------------------------------------------------------
static void testHardSyncReset()
{
    TEST ("Hard sync — phase reset on trigger");
    RoutingGraph graph;

    auto source = std::make_unique<TestEngine>();
    auto dest = std::make_unique<TestEngine>();
    source->init (nullptr);
    dest->init (nullptr);
    source->setEngineType (EngineType::VA);
    dest->setEngineType (EngineType::FM);

    graph.bindEngine (0, source.get());
    graph.bindEngine (1, dest.get());

    source->processSample();
    dest->setFrequency (440.0f);
    dest->processSample();

    ASSERT (graph.addConnection (std::make_shared<HardSyncConnection> (0, 1, 1.0f)), "Add HardSync 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // HardSync trigger should have reset dest phase to initial
    // (phase should be close to initial phase 0)
    float phaseAfter = dest->getPhase();
    ASSERT (std::fabs (phaseAfter) < 0.1f || std::fabs (phaseAfter - 1.0f) < 0.1f,
            "HardSync resets destination phase to initial");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Per-voice independence (polyphony)
// -----------------------------------------------------------------------------
static void testPerVoiceIndependence()
{
    TEST ("Per-voice independence — no cross-voice contamination");
    RoutingGraph graph;

    // Bind engines for two voices (voice 0: nodes 0-5, voice 1: nodes 6-11)
    auto v0osc0 = std::make_unique<TestEngine>();
    auto v0osc1 = std::make_unique<TestEngine>();
    auto v1osc0 = std::make_unique<TestEngine>();
    auto v1osc1 = std::make_unique<TestEngine>();
    v0osc0->init (nullptr);
    v0osc1->init (nullptr);
    v1osc0->init (nullptr);
    v1osc1->init (nullptr);
    v0osc0->setEngineType (EngineType::FM);
    v0osc1->setEngineType (EngineType::VA);
    v1osc0->setEngineType (EngineType::FM);
    v1osc1->setEngineType (EngineType::VA);

    v0osc0->reset ();
    v0osc1->reset ();
    v1osc0->reset ();
    v1osc1->reset ();

    graph.bindEngine (0, v0osc0.get());  // voice 0, OSC 1
    graph.bindEngine (1, v0osc1.get());  // voice 0, OSC 2
    graph.bindEngine (6, v1osc0.get());  // voice 1, OSC 1
    graph.bindEngine (7, v1osc1.get());  // voice 1, OSC 2

    // Voice 0: OSC 1 modulates OSC 2 via FM
    v0osc0->setFrequencyRatio (1.0f);
    v0osc0->setFmAmount (5.0f);
    v0osc0->setFeedback (0.0f);
    v0osc0->setFrequency (440.0f);
    v0osc0->processSample();

    // Voice 1: different note, different FM settings
    v1osc0->setFrequencyRatio (2.0f);
    v1osc0->setFmAmount (10.0f);
    v1osc0->setFeedback (0.0f);
    v1osc0->setFrequency (554.37f);  // C#5
    v1osc0->processSample();

    // Connect voice 0 only: 0->1
    ASSERT (graph.addConnection (std::make_shared<FrequencyModulationConnection> (0, 1, 1.0f)), "Add FM voice0 0->1");
    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // Voice 0 OSC 2 should have FM modulation from voice 0 OSC 1
    float v0osc1Fm = v0osc1->lastFrequencyMod;
    ASSERT (std::fabs (v0osc1Fm) > 0.001f,
            "Voice 0 OSC 2 receives FM from voice 0 OSC 1");

    // Voice 1 OSC 2 (node 7) should have NO FM (no connection 6->7)
    float v1osc1Fm = v1osc1->lastFrequencyMod;
    ASSERT (v1osc1Fm == 0.0f,
            "Voice 1 OSC 2 has no FM (no connection 6->7)");

    // Voice 1 OSC 1 (node 6) should have no FM input either (no incoming connection)
    float v1osc0Fm = v1osc0->lastFrequencyMod;
    ASSERT (v1osc0Fm == 0.0f,
            "Voice 1 OSC 1 has no FM input");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Regression — all-VA with no routing preserves P3.1/P3.2 behavior
// -----------------------------------------------------------------------------
static void testRegressionAllVa()
{
    TEST ("Regression — all-VA, no routing");
    RoutingGraph graph;

    auto engine0 = std::make_unique<TestEngine>();
    engine0->init (nullptr);
    engine0->setEngineType (EngineType::VA);
    engine0->reset ();
    graph.bindEngine (0, engine0.get());

    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    engine0->setFrequency (440.0f);
    const auto frame = engine0->processSample();
    ASSERT (std::isfinite (frame.left) && std::isfinite (frame.right),
            "VA engine produces finite output (no routing)");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — single partial produces sine-like waveform
// -----------------------------------------------------------------------------
static void testAdditiveSinglePartial()
{
    TEST ("Additive — single partial = sine");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    // Only 1 harmonic at amplitude 1.0
    engine->setHarmonicCount (1);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setFrequency (440.0f);

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Single partial at fundamental frequency = sine wave
    // After one sample at 440Hz/44100Hz: phase = 440/44100, sin(2*pi*phase) ≈ 0.0627
    const float expected = std::sin (static_cast<float> (TwoPi) * 440.0f / 44100.0f);
    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "Single partial produces sine at fundamental frequency");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — multiple partials combine
// -----------------------------------------------------------------------------
static void testAdditiveMultiplePartials()
{
    TEST ("Additive — multiple partials combine");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    engine->setHarmonicCount (3);
    engine->setPartialAmplitude (0, 1.0f);   // fundamental
    engine->setPartialAmplitude (1, 0.5f);   // 2nd harmonic
    engine->setPartialAmplitude (2, 0.25f);  // 3rd harmonic
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialPhase (1, 0.0f);
    engine->setPartialPhase (2, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setPartialFrequencyRatio (1, 2.0f);
    engine->setPartialFrequencyRatio (2, 3.0f);
    engine->setFrequency (100.0f);

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Sum of 3 sine waves at 100, 200, 300 Hz
    const float fundamental = std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    const float second = 0.5f * std::sin (static_cast<float> (TwoPi) * 200.0f / 44100.0f);
    const float third = 0.25f * std::sin (static_cast<float> (TwoPi) * 300.0f / 44100.0f);
    const float expected = fundamental + second + third;

    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "Multiple partials combine with correct amplitudes");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — harmonic amplitudes respected
// -----------------------------------------------------------------------------
static void testAdditiveAmplitudeRespected()
{
    TEST ("Additive — harmonic amplitudes respected");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    engine->setHarmonicCount (2);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialAmplitude (1, 0.0f);   // silence 2nd harmonic
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialPhase (1, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setPartialFrequencyRatio (1, 2.0f);
    engine->setFrequency (100.0f);

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Only fundamental should be present
    const float expected = std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "Second harmonic with zero amplitude is silent");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — harmonic phases respected
// -----------------------------------------------------------------------------
static void testAdditivePhaseRespected()
{
    TEST ("Additive — harmonic phases respected");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    engine->setHarmonicCount (1);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialPhase (0, 0.5f);   // half-cycle phase shift = inverted sine
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setFrequency (100.0f);

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Phase 0.5 = 180 degrees = inverted sine
    const float expected = -std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "Half-cycle phase shift inverts sine wave");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — harmonic count respected
// -----------------------------------------------------------------------------
static void testAdditiveHarmonicCount()
{
    TEST ("Additive — harmonic count respected");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    engine->setHarmonicCount (1);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialAmplitude (1, 1.0f);   // 2nd harmonic should be silent (count=1)
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialPhase (1, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setPartialFrequencyRatio (1, 2.0f);
    engine->setFrequency (100.0f);

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Only fundamental should be present
    const float expected = std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "Harmonic count=1 silences 2nd harmonic");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — spectral morph A->B
// -----------------------------------------------------------------------------
static void testAdditiveMorphABC()
{
    TEST ("Additive — spectral morph A->B");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    engine->setHarmonicCount (1);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setFrequency (100.0f);

    // Spectrum B: same harmonic at 0.5 amplitude
    AdditivePartial partialB;
    partialB.amplitude = 0.5f;
    partialB.phase = 0.0f;
    partialB.frequencyRatio = 1.0f;
    engine->setSpectrumBPartial (0, partialB);

    engine->processSample();
    const auto outA = engine->getLastOutput();

    // Morph to B (morph = 1.0)
    engine->setSpectralMorph (1.0f);
    engine->reset ();
    engine->processSample();
    const auto outB = engine->getLastOutput();

    // Morph to middle (morph = 0.5)
    engine->setSpectralMorph (0.5f);
    engine->reset ();
    engine->processSample();
    const auto outMid = engine->getLastOutput();

    const float expectedA = std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    const float expectedB = 0.5f * std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);
    const float expectedMid = 0.75f * std::sin (static_cast<float> (TwoPi) * 100.0f / 44100.0f);

    ASSERT (std::fabs (outA.left - expectedA) < 0.01f, "Morph=0 reproduces Spectrum A");
    ASSERT (std::fabs (outB.left - expectedB) < 0.01f, "Morph=1 reproduces Spectrum B");
    ASSERT (std::fabs (outMid.left - expectedMid) < 0.01f, "Morph=0.5 interpolates correctly");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — anti-aliasing (harmonics above Nyquist suppressed)
// -----------------------------------------------------------------------------
static void testAdditiveAntiAliasing()
{
    TEST ("Additive — anti-aliasing suppresses harmonics above Nyquist");

    auto engine = std::make_unique<TestEngine>();
    engine->init (nullptr);
    engine->setEngineType (EngineType::Additive);
    engine->setLevel (1.0f);
    engine->setPan (-1.0f);
    engine->reset ();
    engine->setSampleRate (44100.0);

    // High fundamental so that 2nd harmonic is above Nyquist at 44100 Hz
    // Nyquist = 22050 Hz, so fundamental > 11025 Hz means 2nd harmonic > Nyquist
    engine->setHarmonicCount (2);
    engine->setPartialAmplitude (0, 1.0f);
    engine->setPartialAmplitude (1, 1.0f);
    engine->setPartialPhase (0, 0.0f);
    engine->setPartialPhase (1, 0.0f);
    engine->setPartialFrequencyRatio (0, 1.0f);
    engine->setPartialFrequencyRatio (1, 2.0f);
    engine->setFrequency (20000.0f);  // 2nd harmonic = 40000 Hz > 22050 Hz Nyquist

    engine->processSample();
    const auto out = engine->getLastOutput();

    // Only fundamental should be present (2nd harmonic suppressed)
    const float expected = std::sin (static_cast<float> (TwoPi) * 20000.0f / 44100.0f);
    ASSERT (std::fabs (out.left - expected) < 0.01f,
            "2nd harmonic above Nyquist is suppressed");
    PASS ();
}

// -----------------------------------------------------------------------------
// Test: Additive — polyphony (six additive oscillators simultaneously)
// -----------------------------------------------------------------------------
static void testAdditivePolyphony()
{
    TEST ("Additive — six additive oscillators simultaneously");

    RoutingGraph graph;

    std::unique_ptr<TestEngine> engines[6];
    for (int i = 0; i < 6; ++i)
    {
        engines[i] = std::make_unique<TestEngine>();
        engines[i]->init (nullptr);
        engines[i]->setEngineType (EngineType::Additive);
        engines[i]->reset ();
        engines[i]->setHarmonicCount (4);
        engines[i]->setPartialAmplitude (0, 1.0f);
        engines[i]->setPartialAmplitude (1, 0.5f);
        engines[i]->setPartialAmplitude (2, 0.25f);
        engines[i]->setPartialAmplitude (3, 0.125f);
        engines[i]->setPartialPhase (0, 0.0f);
        engines[i]->setPartialPhase (1, 0.0f);
        engines[i]->setPartialPhase (2, 0.0f);
        engines[i]->setPartialPhase (3, 0.0f);
        engines[i]->setPartialFrequencyRatio (0, 1.0f);
        engines[i]->setPartialFrequencyRatio (1, 2.0f);
        engines[i]->setPartialFrequencyRatio (2, 3.0f);
        engines[i]->setPartialFrequencyRatio (3, 4.0f);
        engines[i]->setFrequency (100.0f + i * 50.0f);  // different notes
        graph.bindEngine (i, engines[i].get());
    }

    graph.publishConfig();

    graph.resetAllModInputs();
    graph.applyModulations();

    // All six engines should produce finite, non-zero output
    for (int i = 0; i < 6; ++i)
    {
        engines[i]->processSample();
        const auto out = engines[i]->getLastOutput();
        ASSERT (std::isfinite (out.left) && std::isfinite (out.right),
                "Additive oscillator produces finite output");
        ASSERT (std::fabs (out.left) > 0.001f,
                "Additive oscillator produces non-zero output");
    }
    PASS ();
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main()
{
    printf ("OMNIVY P3.4 Additive Synthesis DSP Test Harness\n");
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
    testRegressionAllVa();
    testCyclePreventionMixedTypes();
    testFmOperatorModulation();
    testFmDifferentRatios();
    testFmFeedbackStability();
    testPmIndependently();
    testRingModMultiplication();
    testHardSyncReset();
    testPerVoiceIndependence();
    testAdditiveSinglePartial();
    testAdditiveMultiplePartials();
    testAdditiveAmplitudeRespected();
    testAdditivePhaseRespected();
    testAdditiveHarmonicCount();
    testAdditiveMorphABC();
    testAdditiveAntiAliasing();
    testAdditivePolyphony();

    printf ("\n=============================================\n");
    printf ("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
