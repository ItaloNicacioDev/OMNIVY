// -----------------------------------------------------------------------------
// Omnivy DSP unit tests — Phase 3.4 validation.
//
// Build: compile this file together with TestMain.cpp and the DSP translation
// units listed in CMakeLists (OmnivyTests target). No external test framework
// is used (project rule: no new external dependencies). Only the JUCE headers
// are required on the include path; the DSP code under test is header-inline /
// standard-library only, so no JUCE library linking is needed.
//
// Run the resulting OmnivyTests executable. Exit code is non-zero if any test
// fails.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <vector>
#include <cmath>
#include <limits>

// -----------------------------------------------------------------------------
// Process-wide allocation interception for the A1-1 test.
// Defined here (non-static) so every TU compiled into the test executable uses
// this counting allocation function; the CRT's own operator new is simply never
// pulled in because all references are resolved by this definition. No delete
// of the CRT symbol occurs because there is no second definition.
// -----------------------------------------------------------------------------
static std::atomic<long long> g_allocCount { 0 };

void* operator new (std::size_t size)
{
    ++g_allocCount;
    return std::malloc (size);
}
void operator delete (void* ptr) noexcept
{
    ++g_allocCount;
    if (ptr != nullptr)
        std::free (ptr);
}
void* operator new[] (std::size_t size)
{
    ++g_allocCount;
    return std::malloc (size);
}
void operator delete[] (void* ptr) noexcept
{
    ++g_allocCount;
    if (ptr != nullptr)
        std::free (ptr);
}

#include "tests/TestFramework.h"

#include "DSP/Oscillators/GranularOscillator.h"
#include "DSP/Oscillators/SampleData.h"
#include "DSP/Engine/OscillatorEngine.h"
#include "DSP/Engine/AudioEngine.h"
#include "DSP/Engine/SynthVoice.h"
#include "DSP/Engine/VoiceManager.h"
#include "DSP/Utilities/DSPUtilities.h"

using namespace omnivy;
using namespace omnivy::dsp;
using namespace omnivy::tests;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static SampleData makeConstantSample (float value,
                                      int numChannels = 1,
                                      int numSamples = 44100,
                                      double sampleRate = 44100.0)
{
    SampleData s;
    s.allocate (numChannels, numSamples, sampleRate);
    for (int c = 0; c < s.getNumChannels(); ++c)
    {
        float* data = s.getChannelData (c);
        for (int i = 0; i < numSamples; ++i)
            data[i] = value;
    }
    return s;
}

static float peakAbs (const StereoFrame& f) noexcept { return std::fmaxf (std::fabsf (f.left), std::fabsf (f.right)); }

// -----------------------------------------------------------------------------
// Step 1 — A1-1: Real-time engine allocation
//
// Acceptance: zero dynamic allocations/deallocations on the audio thread during
// engine-type changes.
// -----------------------------------------------------------------------------
TEST_CASE (A1_1_ZeroAllocationsDuringEngineSwitching)
{
    OscillatorEngine engine;

    // Construction performs the 5 preallocated make_unique calls (expected,
    // message-thread setup). Reset the counter to measure only switching.
    TEST_CHECK (g_allocCount.load() > 0); // counter is live: construction allocated
    g_allocCount.store (0);

    EngineType types[] = { EngineType::VA, EngineType::Wavetable, EngineType::FM,
                           EngineType::Additive, EngineType::Granular };

    const int iterations = 20000;
    for (int i = 0; i < iterations; ++i)
    {
        engine.setEngineType (types[i % 5]);       // audio-thread path (updateParameters)
        engine.processSample();                    // exercise the active oscillator pointer
    }

    // Zero allocations AND zero deallocations during the entire switching window.
    TEST_CHECK (g_allocCount.load() == 0);

    // Engine still switches and renders correctly after the stress loop.
    TEST_CHECK (engine.getEngineType() == EngineType::Granular);
    auto out = engine.processSample();
    TEST_CHECK (std::isfinite (out.left) && std::isfinite (out.right));
}

// -----------------------------------------------------------------------------
// Step 2 — A4-1: Master volume thread safety
//
// Validates that the atomic master volume can be written from multiple threads
// while the audio-thread side reads it, without crashes or out-of-range values.
// -----------------------------------------------------------------------------
TEST_CASE (A4_1_MasterVolumeConcurrency)
{
    AudioEngine engine;

    // Getter reads back what the atomic setter stores.
    engine.setMasterVolume (0.5f);
    TEST_CHECK (approxEqual (engine.getMasterVolume(), 0.5f, 1e-6f));
    TEST_CHECK (engine.getMasterVolume() >= 0.0f && engine.getMasterVolume() <= 1.0f);

    std::atomic<bool> stop { false };
    constexpr int numThreads = 4;
    std::vector<std::thread> workers;
    workers.reserve (numThreads);

    for (int t = 0; t < numThreads; ++t)
    {
        workers.emplace_back ([&engine, &stop] ()
        {
            float v = 0.0f;
            while (!stop.load (std::memory_order_acquire))
            {
                v += 0.01f;
                if (v > 1.0f)
                    v = 0.0f;
                engine.setMasterVolume (v);
            }
        });
    }

    // Audio-thread side: read + smooth many times (mirrors processBlock).
    for (int i = 0; i < 200000; ++i)
    {
        float v = engine.getMasterVolume();
        TEST_CHECK (v >= 0.0f && v <= 1.0f);
        TEST_CHECK (std::isfinite (v));
    }

    stop.store (true, std::memory_order_release);
    for (auto& w : workers)
        w.join();

    float finalV = engine.getMasterVolume();
    TEST_CHECK (finalV >= 0.0f && finalV <= 1.0f);
    TEST_CHECK (std::isfinite (finalV));
}

// -----------------------------------------------------------------------------
// Step 3 — A2-4: Granular gain thread safety
//
// m_grainGainScale (and the dependent m_grainSize/m_grainDensity) are atomic so
// concurrent setters cannot expose an inconsistent gain to the audio thread.
// -----------------------------------------------------------------------------
TEST_CASE (A2_4_GranularGainThreadSafety)
{
    GranularOscillator osc;
    osc.setSampleRate (44100.0);
    auto sample = makeConstantSample (0.5f);
    osc.setSample (&sample);

    std::atomic<bool> stop { false };
    std::vector<std::thread> workers;
    workers.reserve (3);
    for (int t = 0; t < 3; ++t)
    {
        workers.emplace_back ([&osc, &stop] ()
        {
            float gs = 0.01f;
            float density = 10.0f;
            while (!stop.load (std::memory_order_acquire))
            {
                osc.setGrainSize (gs);
                osc.setGrainDensity (density);
                gs += 0.01f;
                if (gs > 2.0f)
                    gs = 0.01f;
                density += 100.0f;
                if (density > 1000.0f)
                    density = 10.0f;
            }
        });
    }

    // Audio-thread side: consume the atomic gain scale via processSample.
    for (int i = 0; i < 100000; ++i)
    {
        auto frame = osc.processSample();
        TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
        TEST_CHECK (frame.left >= -1.0f && frame.left <= 1.0f);
        TEST_CHECK (frame.right >= -1.0f && frame.right <= 1.0f);
    }

    stop.store (true, std::memory_order_release);
    for (auto& w : workers)
        w.join();
}

// -----------------------------------------------------------------------------
// Step 4 — A2-2/A2-3: Granular gain integrity
//
// gainScale = 1/sqrt(expectedActive) clamps output; correlated grains can still
// drive the pre-clamp sum above 0 dBFS, so the output clamp is the actual
// protection. We never claim zero clipping without measurement.
// -----------------------------------------------------------------------------

TEST_CASE (A2_2_GranularMaxDensityAndDuration)
{
    GranularOscillator osc;
    osc.setSampleRate (44100.0);
    auto sample = makeConstantSample (0.5f);
    osc.setSample (&sample);

    // Worst case: maximum density (1000/s) x maximum grain size (2.0 s).
    osc.setGrainDensity (1000.0f);
    osc.setGrainSize (2.0f);
    osc.setPositionSpray (0.0f);
    osc.setGrainEnvelope (GranularOscillator::GrainEnvelope::None);

    float maxPeak = 0.0f;
    const int numSamples = 44100; // 1 second
    for (int i = 0; i < numSamples; ++i)
    {
        auto frame = osc.processSample();
        TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
        maxPeak = std::fmaxf (maxPeak, peakAbs (frame));
    }

    // Output must never exceed 0 dBFS (the clamp protects against correlated
    // grain summation that would otherwise exceed 1.0).
    TEST_CHECK (maxPeak <= 1.0f);
    TEST_CHECK (osc.getActiveGrainCount() <= GranularOscillator::kMaxGrains);
}

TEST_CASE (A2_3_GranularCorrelatedVsUncorrelatedPeak)
{
    auto sine = [] () {
        SampleData s;
        s.allocate (1, 44100, 44100.0);
        float* data = s.getChannelData (0);
        for (int i = 0; i < 44100; ++i)
            data[i] = 0.5f * std::sin (static_cast<float> (omnivy::dsp::TwoPi) * static_cast<float> (i) / 44100.0f);
        return s;
    } ();

    auto measurePeak = [&] (float spray) {
        GranularOscillator osc;
        osc.setSampleRate (44100.0);
        osc.setSample (&sine);
        osc.setGrainDensity (500.0f);
        osc.setGrainSize (0.5f);
        osc.setPosition (0.5f);
        osc.setPositionSpray (spray);
        osc.setGrainEnvelope (GranularOscillator::GrainEnvelope::None);
        float maxPeak = 0.0f;
        for (int i = 0; i < 44100; ++i)
        {
            auto frame = osc.processSample();
            TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
            maxPeak = std::fmaxf (maxPeak, peakAbs (frame));
            TEST_CHECK (maxPeak <= 1.0f);
        }
        return maxPeak;
    };

    float correlatedPeak = measurePeak (0.0f);   // zero spray: fully correlated
    float uncorrelatedPeak = measurePeak (1.0f); // full spray: decorrelated

    TEST_CHECK (correlatedPeak <= 1.0f);
    TEST_CHECK (uncorrelatedPeak <= 1.0f);
    // Correlation raises the peak; the clamp keeps both within range.
    TEST_CHECK (correlatedPeak >= uncorrelatedPeak * 0.5f);
}

TEST_CASE (A2_3_GranularZeroSprayDeterminism)
{
    auto buffer = makeConstantSample (0.7f);

    auto render = [&] () {
        GranularOscillator osc;
        osc.setSampleRate (44100.0);
        osc.setSample (&buffer);
        osc.setGrainDensity (100.0f);
        osc.setGrainSize (0.2f);
        osc.setPosition (0.3f);
        osc.setPositionSpray (0.0f); // zero spray: deterministic positions
        osc.setGrainEnvelope (GranularOscillator::GrainEnvelope::None);
        float acc = 0.0f;
        for (int i = 0; i < 4410; ++i) // 100 ms
        {
            auto frame = osc.processSample();
            acc += std::fabsf (frame.left) + std::fabsf (frame.right);
        }
        return acc;
    };

    float a = render();
    float b = render();
    // Fixed LCG seed + zero spray => identical output for identical parameters.
    TEST_CHECK (approxEqual (a, b, 1e-3f));
    TEST_CHECK (a > 0.0f);
}

TEST_CASE (A2_3_GranularPeakAmplitudeNeverClips)
{
    GranularOscillator osc;
    osc.setSampleRate (44100.0);
    auto sample = makeConstantSample (1.0f); // loudest possible source
    osc.setSample (&sample);

    // Sweep a range of densities and sizes; every output must stay in [-1,1].
    float densities[] = { 0.0f, 1.0f, 20.0f, 500.0f, 1000.0f };
    float sizes[]     = { 0.01f, 0.1f, 0.5f, 1.0f, 2.0f };

    float globalMax = 0.0f;
    for (float d : densities)
    {
        for (float s : sizes)
        {
            osc.setGrainDensity (d);
            osc.setGrainSize (s);
            for (int i = 0; i < 4410; ++i)
            {
                auto frame = osc.processSample();
                TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
                TEST_CHECK (frame.left >= -1.0f && frame.left <= 1.0f);
                TEST_CHECK (frame.right >= -1.0f && frame.right <= 1.0f);
                globalMax = std::fmaxf (globalMax, peakAbs (frame));
            }
        }
    }
    TEST_CHECK (globalMax <= 1.0f);
}

TEST_CASE (A2_3_GranularNaNAndInfinityPropagation)
{
    // Buffer containing NaN.
    SampleData nanBuf;
    nanBuf.allocate (1, 44100, 44100.0);
    {
        float* data = nanBuf.getChannelData (0);
        for (int i = 0; i < 44100; ++i)
            data[i] = std::numeric_limits<float>::quiet_NaN();
    }

    // Buffer containing Inf.
    SampleData infBuf;
    infBuf.allocate (1, 44100, 44100.0);
    {
        float* data = infBuf.getChannelData (0);
        for (int i = 0; i < 44100; ++i)
            data[i] = std::numeric_limits<float>::infinity();
    }

    auto testBuffer = [&] (SampleData& buf) {
        GranularOscillator osc;
        osc.setSampleRate (44100.0);
        osc.setSample (&buf);
        osc.setGrainDensity (100.0f);
        osc.setGrainSize (0.1f);
        osc.setGrainEnvelope (GranularOscillator::GrainEnvelope::None);
        for (int i = 0; i < 4410; ++i)
        {
            auto frame = osc.processSample();
            // Non-finite inputs are clamped to 0; output is always finite.
            TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
            TEST_CHECK (frame.left >= -1.0f && frame.left <= 1.0f);
            TEST_CHECK (frame.right >= -1.0f && frame.right <= 1.0f);
        }
    };

    testBuffer (nanBuf);
    testBuffer (infBuf);
}

// -----------------------------------------------------------------------------
// Step 5 — A3-1: Oscillator gain staging
// (Analysis only — see report. No limiter added; see rules 3 & 9.)
//
// The code already documents the theoretical ceiling (6 slots x up to 2.0 =
// 12.0 pre-envelope) and relies on envelope + master volume for headroom, with
// a per-engine output clamp in GranularOscillator. No code change here.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Step 6 — A1-2: Voice rendering optimization
//
// Active-voice tracking lets renderNextSample() return early and skip idle
// voices without compromising note stealing, release, or lifecycle.
// -----------------------------------------------------------------------------
TEST_CASE (A1_2_ActiveVoiceTracking)
{
    WavetableData wavetable;
    VoiceManager vm (&wavetable);
    vm.setSampleRate (44100.0);
    vm.setPolyphony (4);

    // Idle pool: immediate silence, no iteration.
    {
        auto silence = vm.renderNextSample();
        TEST_CHECK (silence.left == 0.0f && silence.right == 0.0f);
        TEST_CHECK (vm.getNumActiveVoices() == 0);
    }

    vm.noteOn (60, 0.5f);
    TEST_CHECK (vm.getNumActiveVoices() == 1);

    auto frame = vm.renderNextSample();
    TEST_CHECK (vm.getNumActiveVoices() >= 1);
    TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));

    vm.noteOff (60);
    // Let the release envelope run to completion.
    for (int i = 0; i < 44100 * 2; ++i)
        vm.renderNextSample();
    TEST_CHECK (vm.getNumActiveVoices() == 0);

    {
        auto silence2 = vm.renderNextSample();
        TEST_CHECK (silence2.left == 0.0f && silence2.right == 0.0f);
    }
}

// -----------------------------------------------------------------------------
// Step 7 — Key tracking optimization
//
// Numerical comparison: cached m_keyTrackRatio vs direct std::pow() calculation.
// Also validates that the cache is recomputed (not stale) when note/keyTrack
// change.
// -----------------------------------------------------------------------------
TEST_CASE (Step7_KeyTrackRatioNumericalComparison)
{
    WavetableData wavetable;
    SynthVoice voice (&wavetable);
    voice.setSampleRate (44100.0);

    struct Case
    {
        int note;
        float keyTrack;
    };
    Case cases[] = {
        { 69, 0.0f }, { 69, 0.5f }, { 69, 1.0f },
        { 60, 0.5f }, { 78, 0.5f }, { 81, 1.0f }, { 57, 1.0f },
        { 12, 0.25f }, { 108, 0.75f },
    };

    for (auto& c : cases)
    {
        voice.setFilterKeyTrack (c.keyTrack);
        voice.noteOn (c.note, 0.5f, 44100.0);
        float cached = voice.getKeyTrackRatio();

        // Direct ("original") calculation performed per-sample before caching.
        float expected = static_cast<float> (std::pow (2.0,
            (static_cast<double> (c.note) - 69.0) / 12.0 * c.keyTrack));

        TEST_CHECK (approxEqual (cached, expected, 1e-4f));
    }

    // Staleness check: changing keyTrack + note recomputes the cache.
    voice.setFilterKeyTrack (0.0f);
    voice.noteOn (69, 0.5f, 44100.0);
    TEST_CHECK (approxEqual (voice.getKeyTrackRatio(), 1.0f, 1e-6f));

    voice.setFilterKeyTrack (1.0f);
    voice.noteOn (81, 0.5f, 44100.0); // one octave above 69, full key tracking => 2.0
    TEST_CHECK (approxEqual (voice.getKeyTrackRatio(), 2.0f, 1e-4f));
}

// -----------------------------------------------------------------------------
// Step 8 — Granular pitch optimization
//
// effectivePitchRatio() caches 3x std::pow() and invalidates on pitch parameter
// changes. Validate correctness at pitch extremes and that changing pitch
// parameters produces a different (non-stale) result.
// -----------------------------------------------------------------------------
TEST_CASE (Step8_GranularPitchAccuracyAndCache)
{
    // Buffer de seno (não constante): mudar o pitch altera o increment do grão
    // e, portanto, o output. Um buffer constante mascararia a diferença.
    SampleData sineBuf;
    sineBuf.allocate (1, 44100, 44100.0);
    {
        float* data = sineBuf.getChannelData (0);
        for (int i = 0; i < 44100; ++i)
            data[i] = 0.5f * std::sin (static_cast<float> (omnivy::dsp::TwoPi) * static_cast<float> (i) / 44100.0f);
    }

    GranularOscillator osc;
    osc.setSampleRate (44100.0);
    osc.setSample (&sineBuf);
    osc.setGrainDensity (200.0f);
    osc.setGrainSize (0.5f);
    osc.setPositionSpray (0.0f);
    osc.setGrainEnvelope (GranularOscillator::GrainEnvelope::None);

    // Prime the cache.
    osc.processSample();

    // Pitch extremes must remain finite (cache math must not overflow).
    float pitches[] = { -48.0f, -24.0f, 0.0f, 24.0f, 48.0f };
    for (float p : pitches)
    {
        osc.setPitch (p);
        for (int i = 0; i < 4410; ++i)
        {
            auto frame = osc.processSample();
            TEST_CHECK (std::isfinite (frame.left) && std::isfinite (frame.right));
            TEST_CHECK (frame.left >= -1.0f && frame.left <= 1.0f);
            TEST_CHECK (frame.right >= -1.0f && frame.right <= 1.0f);
        }
    }

    // Changing pitch must change the output (cache invalidated, not stale).
    osc.setPitch (0.0f);
    float acc0 = 0.0f;
    for (int i = 0; i < 4410; ++i)
        acc0 += std::fabsf (osc.processSample().left);

    osc.setPitch (24.0f);
    float acc24 = 0.0f;
    for (int i = 0; i < 4410; ++i)
        acc24 += std::fabsf (osc.processSample().left);

    TEST_CHECK (acc0 > 0.0f);
    TEST_CHECK (acc24 > 0.0f);
    TEST_CHECK (!approxEqual (acc0, acc24, 1e-3f)); // pitch change has an effect
}
