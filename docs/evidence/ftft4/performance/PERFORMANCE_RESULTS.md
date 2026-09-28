# FTFT4A-P executed performance and regression evidence

Performance is improved, with material remaining cost. This report does not close FTFT4B and does not turn the historical geometry into a correctness oracle.

## Seven interleaved engine trials

All three versions used the same compiler, `-O3 -DNDEBUG -std=gnu++17 -Wall -Wextra -ffp-contract=off`, the same machine, and sequential cyclic execution. All builds and regression jobs had finished before the formal timing batch. No concurrent benchmark workers were used.

Values are median [minimum, maximum] milliseconds across seven runs. The crate workload/function and actual final workload were unchanged: **1,501 bodies, 1,125,750 possible pairs, 1,500 candidates, 1,500 colliding pairs, 6,000 contact points, tree height 11** in every version/trial.

| Crate metric | Historical HEAD | Correct-but-slow FTFT4A | Optimized FTFT4A |
|---|---:|---:|---:|
| total_ms | 3.984 [3.813, 4.304] | 19.794 [19.144, 27.465] | 12.876 [12.519, 14.108] |
| broadphase_ms | 0.383 [0.379, 0.459] | 2.582 [2.541, 3.504] | 0.914 [0.777, 1.494] |
| narrowphase_ms | 1.17 [1.141, 1.373] | 11.637 [11.145, 16.531] | 8.108 [8.036, 8.936] |
| solver_ms | 2.237 [2.199, 2.433] | 2.933 [2.859, 4.719] | 3.789 [3.637, 4.585] |
| whole_fixture_ms | 230.842 [228.633, 235.668] | 2940.35 [2866.75, 2996.94] | 3110.31 [3004.06, 3257.58] |

The actual final-step median decreased **34.95% (1.537× faster)** versus the preserved slow source, and remains **3.232×** the historical cost. Narrowphase/assembly remains the largest measured component. The solver-labelled timer increased, but now includes post-integration interval/normalized-orientation preparation that previously occurred during bounds/proxy work. These component boundaries moved: this does **not** establish that the constraint iterations themselves became slower. The new cache diagnostics expose 1,501 allocation calls in the final crate step. These costs are not hidden or treated as solved.

**Whole-fixture wall time increased.** This additional timer wraps the unchanged complete `TestScaleStatistics`, including creation/cache preparation, all 30 steps, its existing exhaustive all-pairs correctness comparison, printing and destruction. It is not pure simulation time. The existing exhaustive comparison traverses 1,125,750 potential pairs using uncached geometry calls. The aggregate timer does not separately attribute its increase to that check versus other work, so no more specific causal claim is made.

## Mixed actual-engine workloads

Each workload has 32 dynamic bodies plus one support and runs 120 steps. Full-step values include normal cache work; external-frame values additionally include actual `ResetBody` support motion and force calls. The rotating, compound and moving-support cases use genuine angular evolution and nonidentity orientations. The near-parallel/exact-touch case actually executes exact fallbacks.

| Workload, full-step ms | Historical HEAD | Slow | Optimized | Slow/optimized |
|---|---:|---:|---:|---:|
| rotating_boxes | 0.0840884 [0.0825819, 0.104466] | 0.483948 [0.466494, 0.507658] | 0.288271 [0.273807, 0.297922] | 1.679× |
| offset_compounds | 0.178976 [0.172696, 0.192166] | 2.18371 [2.16251, 2.31758] | 1.15817 [1.11468, 1.22505] | 1.885× |
| near_parallel_and_exact_touch | 0.0674344 [0.0652493, 0.070695] | 0.332431 [0.325707, 0.348637] | 0.206188 [0.203073, 0.217923] | 1.612× |
| moving_static_support | 0.143908 [0.13717, 0.151617] | 1.04412 [1.02782, 1.10484] | 0.497836 [0.491104, 0.519902] | 2.097× |

| Workload | Slow setup ms | Optimized setup ms | Slow external-frame ms | Optimized external-frame ms |
|---|---:|---:|---:|---:|
| rotating_boxes | 0.144186 [0.120129, 0.20131] | 0.151401 [0.132394, 0.21111] | 0.484176 [0.466693, 0.50788] | 0.288491 [0.274093, 0.298162] |
| offset_compounds | 0.267313 [0.260932, 0.337387] | 0.151018 [0.142542, 0.27297] | 2.18395 [2.16283, 2.31786] | 1.15848 [1.1149, 1.2253] |
| near_parallel_and_exact_touch | 0.058455 [0.058126, 0.099082] | 0.053844 [0.052952, 0.064861] | 0.332632 [0.325897, 0.348845] | 0.206377 [0.203263, 0.218133] |
| moving_static_support | 0.155858 [0.153755, 0.175533] | 0.102968 [0.101614, 0.148669] | 1.04656 [1.0303, 1.1077] | 0.499476 [0.492522, 0.521493] |

Creation-time preparation is included in setup, not hidden outside all measurements. The optimized rotating-box setup median is slightly higher with overlapping timing ranges; performance improvement is not universal for every component.

## Counters, memory and trajectories

Crate final-step counters, every optimized trial: 45,000 geometric predicates, zero exact fallbacks, zero unresolved cases; 4,500 orientation hits / 1,500 rebuilds; 6,000 bound hits / 1,500 rebuilds; zero shape rebuilds; 3,002 solver-frame builds; **2,045,536 retained-cache bytes estimated**; **1,501 cache allocation calls**. The original slow case has the same predicate/fallback/unresolved counts and does not expose new cache counters.

| Mixed workload (120 steps) | Candidates / colliding / points | Predicates | Exact fallback slow / optimized | Cache hits orientation / bounds | Rebuilds orientation / bounds | Allocations | Peak estimated cache bytes |
|---|---:|---:|---:|---:|---:|---:|---:|
| rotating_boxes | 3959 / 3401 / 12072 | 105573 | 0 / 0 | 11759 / 16841 | 2358 / 2359 | 3522 | 63472 |
| offset_compounds | 9308 / 3371 / 17059 | 384936 | 0 / 0 | 22456 / 16255 | 2945 / 2945 | 6437 | 81904 |
| near_parallel_and_exact_touch | 3840 / 3840 / 12480 | 87304 | 48 / 48 | 11520 / 16320 | 2880 / 2880 | 3970 | 63472 |
| moving_static_support | 4992 / 3376 / 14551 | 158760 | 0 / 0 | 14140 / 16058 | 2826 / 3142 | 4390 | 69808 |

The timed comparison drivers record `geometryExactFallbacks`, not the separate
`geometryNumericGapFallbacks` field. A subsequent **supporting diagnostic**
using the same compiled optimized objects and unchanged fixtures measured
that field: zero numerical-gap fallbacks in 30 crate steps plus the exhaustive
queries (3,771,000 predicates), and zero in the four mixed workloads / 480
steps (736,573 predicates, 48 exact predicate fallbacks). Both runs had zero
unresolved/invalid/stale-hint cases. These two additional invocations are
not part of the 42-run performance sample. See `gap-diagnostics/metadata.json`
and its raw logs; no source or algorithm changed.

Every mixed optimized/slow candidate, colliding-pair, point, predicate and fallback count matches; all unresolved counts are zero. Cache allocations count the declared production node/bucket/vector allocation calls, and bytes estimate retained payload/capacity, excluding allocator bookkeeping and the new per-reserved-constraint frame indices/alignment. Raw process peak RSS is logged but is a coarse process/launch high-water diagnostic, not an attribution of cache memory.

For slow versus optimized, **21,632 sampled body-state scalar values compare identically** (four workloads × 13 recorded steps × 32 bodies × 13 position/quaternion/linear/angular components). All 120-step state checksums also match, and each version repeats deterministically across seven trials. A rolling hash is supporting evidence, not proof against hash collisions or a substitute for raw all-step equality. No trajectory difference was observed in the samples.

All four original classic/terrain near/far application CSV traces are **byte-identical to the preserved slow result**. These traces retain their existing logging precision; this is not a universal exact-trajectory theorem.

## Executed checks

- Formal performance batch: **42 processes**, comprising 21 unchanged crate-fixture executions (630 steps) and 21 mixed-driver executions (84 workloads, 10,080 steps), all PASS (the three trial-one mixed runs also emit sampled traces; their 1,440 steps are included in the 10,080, not additional executions); elapsed 50.805 seconds.
- Complete production regression discovery: **39 suites**, all PASS; actual async integration **12 cases / 246 checks**, zero failures. Existing M31 stress remains supporting diagnostic evidence, not deterministic acceptance.
- FTFT1 runtime/editor persistence smoke: six processes, PASS. FTFT3: 136 scenes, 272 runtime constructions, 32,640 fixed steps, zero failures.
- Independent player sweeps: 21/21 PASS. Four unchanged 900-step near/far runtime walks PASS and remain pairwise identical.
- Full regression elapsed 216.976 seconds, including the full rebuild under the explicit contraction flag.
- Supplied unchanged scalar references: 5,393,305 arithmetic assertions; 90,000 exact SAT equality/enclosure assertions; 823 fixture rows / 887 primitive pairs with unchanged 35 fallback counts; 60,000 bit-identical prepared-bound checks. All PASS. These are supporting component checks; no scalar storage carrier is an engine dependency.
- All **3,705 protected source/evidence hashes unchanged**. No protected fluid prototype was built or run. No human visual validation claimed.
- Cache-specific independent geometry, range rejection and actual source-adapter/tree tests are recorded separately by their owning validator; this report does not duplicate their execution counts.

## Sources and reproduction

New executable source/runner files from this evidence work: `scripts/ftft4p_validation.py`, `tests/ContactPerformanceTests.cpp`, and `tests/ContactScalePerformance.cpp`. They do not modify production physics; production optimization changes are documented in the parent FTFT4A-P report.

`PERFORMANCE_METHOD.md` gives commands and timing scope. `reference-builds/results.json`, `optimized-build/results.json` and `scale-builds/results.json` record exact source, compiler, flags and executable hashes. `pre-optimization-source.zip` preserves the full slow source and `old-production-source.zip` preserves historical source. `benchmarks/results.json` and per-execution logs contain all timings, work/counter diagnostics and raw sampled trajectories. `regressions/results.json` links every actual acceptance result. Binaries/objects remain under ignored `build/ftft4p/`.

No algorithm, fixture, reconstruction tolerance, timing law or assertion was changed during these measurements. The new runner initially encountered a protection-manifest schema mismatch before compiling; its union mapping was corrected, with no actual protected-file difference. No physical/test failure was repaired by changing acceptance.

FTFT4B impact timing remains OPEN. This measured optimization recovers part of the cost while preserving the validated geometry in these checks; the remaining 12.876ms crate cost, increased solver-labelled timer/allocations and increased complete diagnostic-function wall time remain review issues.
