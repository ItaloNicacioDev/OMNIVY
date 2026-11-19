// -----------------------------------------------------------------------------
// Omnivy DSP test runner.
// -----------------------------------------------------------------------------
#include "tests/TestFramework.h"

#include <cstdio>
#include <vector>

int omnivy::tests::g_testsRun = 0;
int omnivy::tests::g_testsPassed = 0;
int omnivy::tests::g_testsFailed = 0;
int omnivy::tests::g_checksRun = 0;
int omnivy::tests::g_checksFailed = 0;

int main ()
{
    const auto& entries = omnivy::tests::Registry::instance().getEntries();

    std::printf ("Omnivy DSP unit tests (Phase 3.4)\n");
    std::printf ("==================================\n");
    std::printf ("Test cases registered: %zu\n\n", entries.size());

    for (const auto& entry : entries)
    {
        ++omnivy::tests::g_testsRun;
        const std::size_t checksBefore = omnivy::tests::g_checksRun;
        const std::size_t failuresBefore = omnivy::tests::g_checksFailed;

        entry.fn();

        const std::size_t checksInCase = omnivy::tests::g_checksRun - checksBefore;
        const std::size_t failuresInCase = omnivy::tests::g_checksFailed - failuresBefore;

        if (failuresInCase == 0)
        {
            ++omnivy::tests::g_testsPassed;
            std::printf ("  PASS %-55s (%zu checks)\n", entry.name.c_str(), checksInCase);
        }
        else
        {
            ++omnivy::tests::g_testsFailed;
            std::printf ("  FAIL %-55s (%zu checks, %zu failed)\n", entry.name.c_str(),
                         checksInCase, failuresInCase);
        }
    }

    std::printf ("\n----------------------------------\n");
    std::printf ("Tests run   : %d\n", omnivy::tests::g_testsRun);
    std::printf ("Tests passed: %d\n", omnivy::tests::g_testsPassed);
    std::printf ("Tests failed: %d\n", omnivy::tests::g_testsFailed);
    std::printf ("Checks run  : %zu\n", omnivy::tests::g_checksRun);
    std::printf ("Checks failed: %zu\n", omnivy::tests::g_checksFailed);

    return omnivy::tests::g_testsFailed == 0 ? 0 : 1;
}
