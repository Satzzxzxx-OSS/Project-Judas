# FTFT4A-P comparison procedure

The correctness reference is the preserved pre-optimization FTFT4A source ZIP. The historical HEAD is also timed; its old geometry is **not** the correctness oracle.

## Reproduction

From the repository root, with the reviewed source frozen:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --build-references
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --components
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --build-optimized
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --build-scale-wrappers
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --regressions --jobs 4
PYTHONDONTWRITEBYTECODE=1 python3 scripts/ftft4p_validation.py --benchmarks
```

Each mode refuses to overwrite an existing `results.json`. Preserved source archives, code, execution commands, outputs and hashes are under this directory; compiled programs and object files are disposable `build/ftft4p/` products. Every comparison uses the same compiler and explicit `-O3 -DNDEBUG -std=gnu++17 -Wall -Wextra -ffp-contract=off` flags. No profiling instrumentation, extra engine thread, fast-math, native-ISA option or replacement GLM is used. The supplied scalar-reference `value_carriers.hpp` remains isolated inside the unmodified research package.

## Timers and workloads

Seven trials rotate the execution order OLD/SLOW/OPT, SLOW/OPT/OLD, OPT/OLD/SLOW. Every child completes before another starts. Formal timing must run without simultaneous builds, regression suites or other benchmark work.

The production-scale fixture is the actual `BroadphaseTests.cpp::TestScaleStatistics`: 1,500 dynamic crates, one floor, 30 fixed steps, reporting its existing final-step and component timers. A source comparison permits only additional independent diagnostic printing; setup, body/candidate/contact work, step count and assertions must remain unchanged. The same supplemental wrapper includes the actual source file, renames its original main and calls only that unmodified function. Its whole-function wall timer includes creation/cache preparation, 30 steps, printing, the original exhaustive pair-comparison assertion and destruction; it is not pure simulation time. The full broadphase suite also runs through the complete production regression matrix.

The identical `tests/ContactPerformanceTests.cpp` is linked against all three engine versions. Its four deterministic, 33-body, 120-step workloads exercise rotating boxes, three-box compounds with nonzero offsets, near-parallel/exact-touch geometry and a static support moved and rotated with `ResetBody` every frame. No engine branch selects these fixtures. Each reports:

- setup time, including creation-time derived-cache preparation;
- average external frame time including support updates, force application and `PhysicsWorld::Step`;
- actual full-step and existing component timers, with cache preparation/rebuild inside their normal production execution;
- total candidate/contact work and exact/unresolved predicate counters;
- actual cache-hit/rebuild/allocation counters and peak cache bytes where that engine version exposes them;
- process peak RSS, explicitly a coarse process-level measurement rather than a cache-allocation oracle.

Component labels retain their production timer boundaries, which can contain different preparation work after optimization: in particular, post-integration orientation preparation is now inside the solver-labelled interval. Do not infer an isolated constraint-iteration slowdown from that component alone.

The allocation counter is the engine's declared node/bucket/vector allocation-call diagnostic; the byte count is its retained payload/capacity estimate, not allocator metadata, the new per-reserved-constraint frame indices/alignment, or all process memory. Creation-time cost is visible in setup time and retained bytes; per-step counters count their stated step only.

Every body transform and linear/angular velocity at all 120 steps contributes its represented float bits to a deterministic checksum. Trial one additionally records full body states every tenth step and at the last step. Slow/optimized comparisons report component differences and the first *sampled* divergence. A passing benchmark means the fixture executed with finite states and no unresolved geometry; it does not silently approve trajectory changes or a performance threshold.

## Regressions and protected outputs

The existing FTFT4 regression helpers reuse the unchanged FTFT2 application runner and FTFT1 smoke logic. An explicit gravity-test output argument keeps FTFT3 evidence untouched, including when that target is discovered by the FTFT2 suite. Fresh FTFT3 construction output and the same four 900-step near/far application walks go into `performance/regressions/`. Walk differences are measured against the slow FTFT4 result and reported, not rebaselined.

Every mode verifies all 3,705 protected source/evidence hashes against the preserved source/evidence union. No fluid prototype is built or executed. Pressure timing, restitution, friction, iteration policy and FTFT4B temporal witnesses are outside this performance change.
