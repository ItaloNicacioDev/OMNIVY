#pragma once

// -----------------------------------------------------------------------------
// Minimal, dependency-free unit test framework.
// -----------------------------------------------------------------------------
// No external test library is used (project rule: no new external dependencies).
// Tests are registered with TEST_CASE() and run from a single main() in
// TestMain.cpp. CHECK() records a failure and continues; REQUIRE() records a
// failure and aborts the current test case.
// -----------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace omnivy {
namespace tests {

// -----------------------------------------------------------------------------
// Counters (defined in TestMain.cpp).
// -----------------------------------------------------------------------------
extern int g_testsRun;
extern int g_testsPassed;
extern int g_testsFailed;
extern int g_checksRun;
extern int g_checksFailed;

// -----------------------------------------------------------------------------
// Registry.
// -----------------------------------------------------------------------------
struct TestEntry
{
    std::string name;
    std::function<void()> fn;
};

class Registry
{
public:
    static Registry& instance() noexcept
    {
        static Registry reg;
        return reg;
    }

    void add (std::string name, std::function<void()> fn)
    {
        entries.push_back (TestEntry { std::move (name), std::move (fn) });
    }

    const std::vector<TestEntry>& getEntries() const noexcept { return entries; }

private:
    std::vector<TestEntry> entries;
};

// -----------------------------------------------------------------------------
// Assertion macros.
// -----------------------------------------------------------------------------
#define TEST_CHECK(condition)                                                    \
    do                                                                           \
    {                                                                            \
        ++omnivy::tests::g_checksRun;                                            \
        if (!(condition))                                                        \
        {                                                                        \
            ++omnivy::tests::g_checksFailed;                                     \
            std::fprintf (stderr, "  CHECK failed %s:%d: %s\n", __FILE__,        \
                          __LINE__, #condition);                                 \
        }                                                                        \
    } while (false)

#define TEST_REQUIRE(condition)                                                  \
    do                                                                           \
    {                                                                            \
        ++omnivy::tests::g_checksRun;                                            \
        if (!(condition))                                                        \
        {                                                                        \
            ++omnivy::tests::g_checksFailed;                                     \
            std::fprintf (stderr, "  REQUIRED failed %s:%d: %s\n", __FILE__,     \
                          __LINE__, #condition);                                 \
            return;                                                              \
        }                                                                        \
    } while (false)

// -----------------------------------------------------------------------------
// Test case registration macro.
// -----------------------------------------------------------------------------
#define TEST_CASE(name)                                                          \
    static void name##_body();                                                   \
    struct name##_registrar                                                      \
    {                                                                            \
        name##_registrar()                                                       \
        {                                                                        \
            omnivy::tests::Registry::instance().add (#name, name##_body);        \
        }                                                                        \
    };                                                                           \
    static name##_registrar name##_registrar_instance;                           \
    static void name##_body()

// -----------------------------------------------------------------------------
// Approximate floating-point comparison.
// -----------------------------------------------------------------------------
inline bool approxEqual (float a, float b, float tolerance = 1e-4f) noexcept
{
    return std::fabs (a - b) <= tolerance;
}

inline bool approxEqual (double a, double b, double tolerance = 1e-9) noexcept
{
    return std::fabs (a - b) <= tolerance;
}

} // namespace tests
} // namespace omnivy
