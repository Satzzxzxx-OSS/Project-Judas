// The original workload is included unchanged. Timing this complete call exposes
// creation-time cache preparation as well as all 30 steps, the existing exhaustive
// comparison assertion and destruction. It is NOT a pure simulation timer.
#ifndef FTFT4P_BROADPHASE_SOURCE
#error "Compile with the selected snapshot's actual BroadphaseTests.cpp path"
#endif
#define main JudasOriginalBroadphaseMain
#include FTFT4P_BROADPHASE_SOURCE
#undef main
int main() {
    const auto begin=std::chrono::steady_clock::now();
    TestScaleStatistics();
    const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    std::printf("scale fixture wall %.9f ms (creation + 30 steps + original exhaustive assertion + destruction)\n",elapsed);
    std::printf("Scale fixture: %s (%d failures)\n",g_failures==0?"PASS":"FAIL",g_failures);
    return g_failures==0?0:1;
}
