// Supporting FTFT4A-P counter diagnostic. Not timed acceptance evidence.
// Includes each unchanged fixture in one translation unit; production objects
// are the already-built optimized engine objects, not a second implementation.
#include <cstdio>
#include "Contacts.h"
#define main unchangedFixtureMain
#ifdef FTFT4P_GAP_MIXED
#include "tests/ContactPerformanceTests.cpp"
#else
#include "tests/BroadphaseTests.cpp"
#endif
#undef main
int main(int, char** argv) {
    ResetContactGeometryDiagnostics();
#ifdef FTFT4P_GAP_MIXED
    const int status=unchangedFixtureMain(1,argv);
    constexpr const char* scope="four unchanged mixed workloads, 480 fixed steps";
#else
    (void)argv;
    TestScaleStatistics();
    const int status=g_failures==0?0:1;
    constexpr const char* scope="unchanged 1500-crate fixture, 30 fixed steps plus its final exhaustive correctness queries";
#endif
    const auto d=GetContactGeometryDiagnostics();
    std::printf("{\"kind\":\"supporting_gap_diagnostics\",\"scope\":\"%s\",\"predicates\":%llu,\"intervalResolved\":%llu,\"exactFallbacks\":%llu,\"numericGapFallbacks\":%llu,\"unresolved\":%llu,\"invalidInputs\":%llu,\"preparedOrientationMisses\":%llu,\"fixtureExitCode\":%d}\n",
        scope,static_cast<unsigned long long>(d.predicates),
        static_cast<unsigned long long>(d.intervalResolved),
        static_cast<unsigned long long>(d.exactFallbacks),
        static_cast<unsigned long long>(d.numericGapFallbacks),
        static_cast<unsigned long long>(d.unresolved),
        static_cast<unsigned long long>(d.invalidInputs),
        static_cast<unsigned long long>(d.preparedOrientationMisses),status);
    return status;
}
