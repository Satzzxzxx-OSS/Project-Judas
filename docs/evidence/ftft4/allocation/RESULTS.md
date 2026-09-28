# FTFT4A-P: bounded cache allocation/lifetime pass

**Correctness passed. Allocation objective met. Overall speed benefit is small/mixed; performance acceptance remains for operator review. Stop here.**

HEAD remains `3404388f1af48419505c802fc11dbf8a0b577f38`. No commit, push, tag, reset, numerical/contact-law change or FTFT4B work. The prior optimized candidate and all 928 prior performance evidence files are preserved; `before-source.zip` is its exact source snapshot.

## What changed and why storage is reusable

- `ContactSolver::Clear/RegisterFrame` formerly destroyed and heap-allocated one `unordered_map` node per contacting body each step. The map is now a `pmr::unordered_map` backed by a solver-owned `unsynchronized_pool_resource`. **Keys, pointers and numerical frame values still clear/rebuild every step.** Only raw node storage survives. The pool is declared before the map and after its counting upstream; reverse destruction order is safe. No pool release occurs while a map consumer is live. Vector growth uses indices and the solver never retains a pointer to an element across growth. Body pointers live only within one solve, including current-position correction.
- `PhysicsWorld::Orientation/CurrentBound` formerly reset the entire optional prepared-bound value on each quaternion change; rotating compounds allocated the same child array again. An explicit validity bit invalidates contents while preserving capacity. `PrepareShapeBounds(..., output)` rebuilds into owned storage with the exact previous arithmetic. Slot replacement destroys the old Body and all its storage. No child-vector pointers are retained. Exact quaternion and pose keys, previous/current bounds and proxy refreshing are unchanged.
- Immutable shape radius, inline orientation preparation, phase-local solver frames, compound child bounds, per-pair manifold scratch and lazy exact expansion storage are distinct. This pass changes only frame-map backing storage and prepared-child capacity lifetime. Constraint/pending/frame vectors already retained capacity. Exact fallback and clipping scratch are unchanged.

Retention follows peak simultaneous body/child demand, not elapsed steps. Pool options cap chunks at 256 blocks and pool blocks up to 64 bytes; larger bucket storage uses the counted upstream. The standard library may round pool settings; counters report actual upstream requests. There is no fixed-byte universal memory cap or automatic shrink below the world’s high-water demand. Destruction releases all retained storage. Stable-population tests verify retained bytes stop growing.

## Measurements: seven interleaved runs

The unchanged 1,500-crate fixture still constructs 1,501 bodies, simulates **all 30 steps**, runs its actual exhaustive oracle, and destroys the world. The generated wrapper removes back to the original fixture exactly when measurement insertions are stripped. No engine calls, body data, assertions or oracle workload are removed. Uniform `-O3 -DNDEBUG -std=gnu++17 -ffp-contract=off` builds; one child at a time; no concurrent compilation or regression work during trials. SLOW is the preserved correct-but-slow source, BEFORE is the user’s current 12.876-ms candidate, AFTER is this storage pass. The earlier 12.876-ms result is historical; compare this run’s paired BEFORE and AFTER.

All values below are milliseconds, with each reported column its own seven-run median (medians need not add).

| Measured scope | SLOW reference | BEFORE candidate | AFTER storage pass |
|---|---|---|---|
| Whole fixture, including oracle | 2936.700 | 3070.935 | 3070.294 |
| Construction/cache preparation | 1.884 | 2.720 | 2.717 |
| First step (forces + Step) | 19.890 | 16.599 | 16.195 |
| All 30 steps (forces + Step) | 604.472 | 405.040 | 399.117 |
| Steady step: median of steps 2–30 | 19.731 | 13.320 | 12.678 |
| Final Step internal timer | 19.772 | 12.569 | 12.465 |
| Complete oracle comparison | 2327.042 | 2662.802 | 2667.812 |
| Exhaustive OraclePairs alone | 2315.319 | 2655.269 | 2660.194 |
| BroadphasePairs comparison query | 11.189 | 7.536 | 7.475 |
| Printing/assertions/instrumentation | 0.031 | 0.036 | 0.030 |
| Actual implicit teardown | 0.260 | 0.350 | 0.372 |

Steady-state excludes only the separately reported first step; **all-step and whole totals include it**. Across every steady step, ranges were before: 11.927–24.324 ms, after: 11.970–16.835 ms. Whole-fixture ranges were before: 3041.644–3146.340 ms, after: 3034.207–3124.503 ms. Every per-step/per-trial value is retained in `measure/results.json`.

### Why the previous complete-test regression occurred

The old report’s 5.78% wall-time increase included **1,125,750 exhaustive pair comparisons**, overwhelmingly separated, in `BroadphaseTests::OraclePairs`. They are correctness instrumentation, not production broadphase candidates. The unchanged engine’s broadphase sends only 1,500 contacting candidates per step. Here the historical pattern repeats at **4.57% by whole-fixture medians** (SLOW→BEFORE); it is not asserted to reproduce precisely 5.78% on another run.

For an additive accounting, use seven-run **means**: SLOW→BEFORE exhaustive/comparison oracle work rises 356.504 ms, all 30 physics steps fall 199.332 ms, construction rises 0.833 ms, teardown rises 0.093 ms and other instrumentation rises 0.004 ms. Net whole-fixture increase: **158.102 ms**. No work was moved outside timing.

The source-specific mechanism is the cold separated-pair path: `Contacts.cpp::BoxBox` builds `SatWorkspace<Iv>` with all nine cross-body dot products before testing the first separation axis, and public uncached oracle calls reconstruct orientation data instead of using persistent world caches. The slow version evaluated projections as axes were visited and often returned on the first axis. Contact-heavy production pairs amortize the workspace; a mostly-separated exhaustive query does not. Phase timings establish where the cost lives; this is a source-supported explanation, not a new call-stack profile attributing every cycle to an individual instruction. No predicate/workspace changes are made in this pass.

AFTER’s oracle remains about 87% of the complete test. Median all-step time improves 1.46%, but whole-fixture median changes only −0.021%. This does not establish a meaningful whole-workload speedup. Final-step narrowphase remains **8.128 ms**, about 65% of the 12.465-ms final Step. Frame allocations were avoidable but were not the dominant cost.

## Heap allocations and retained storage

Separate diagnostic executables override C++ new/delete (including aligned allocation) using a nonallocating counter/header. Timed acceptance executables do **not** link this override. Counts are requested payload bytes, excluding allocator headers, libc malloc and operating-system reservation; all relevant engine containers use the observed C++ path. Diagnostic timings remain in raw logs but are not used for performance acceptance. No allocation inside physics is hidden from these C++ counters.

| Phase | Before calls | Before bytes requested | After calls | After bytes requested |
|---|---|---|---|---|
| Construction | 50 | 4783748 | 51 | 4784052 |
| First step | 1595 | 8241348 | 105 | 8243604 |
| Final steady step | 1502 | 276024 | 1 | 240000 |
| Oracle | 3013 | 175144 | 3013 | 175144 |
| Whole fixture | 48234 | 22521722 | 3216 | 21479586 |

Cache-specific first-step growth: **31 calls / 631,136 bytes** after the change. Every subsequent crate step registers the same 1,501 logical body frames with **0 cache heap calls / 0 cache bytes allocated**. The previous 1,501 per-step node allocations requested 36,024 bytes. The one remaining steady C++ allocation requests 240,000 bytes, consistent with the unchanged `std::stable_sort` temporary for the 6,000-entry contact-cache ordering (`PhysicsWorld.cpp::Step`); that separate per-step scratch path is not pooled or rewritten.

After the final step, measured live whole-world heap payload is **7,074,040 → 7,076,384 bytes**, an increase of 2,344 bytes. Peak heap payload is **7,390,320 → 7,404,064 bytes**. It returns to **zero** after teardown in both diagnostic processes.

AFTER reports **2,180,840 bytes** of retained geometry/frame cache payload, stable across all 30 crate steps. Do not directly compare this with the old 2,045,536-byte estimate: this pass reports actual outstanding pool bytes and includes the two frame indices per retained constraint, which the previous cache estimate omitted. Whole-world live-heap counters above provide the consistent before/after memory comparison. Padding/internal allocator overhead remain excluded from the cache estimate.

The crate workload has zero exact fallbacks. The mixed exact-touch workload retains its **48 exact fallback predicates**, with no unresolved result; its remaining allocations include exact expansion work. These diagnostic counts are not a claim that every allocation in that workload belongs to fallback. Lazy exact storage is unchanged and exceptional work has not been replaced with a filter shortcut.

## Unchanged varied workloads

Each workload still runs 120 steps with the original rotations, compound offsets and moving-support ResetBody calls. Seven interleaved runs per source. Below: independently aggregated medians in milliseconds.

| Fixture | Version | Setup | First | All 120 steps | Steady median | Teardown | Other diagnostics | Whole |
|---|---|---|---|---|---|---|---|---|
| rotating_boxes | before | 0.158 | 0.064 | 33.343 | 0.287 | 0.014 | 0.261 | 34.431 |
| rotating_boxes | after | 0.188 | 0.074 | 33.574 | 0.287 | 0.020 | 0.269 | 34.062 |
| offset_compounds | before | 0.166 | 0.081 | 136.295 | 1.236 | 0.028 | 0.258 | 136.746 |
| offset_compounds | after | 0.158 | 0.079 | 140.201 | 1.228 | 0.038 | 0.282 | 140.679 |
| near_parallel_and_exact_touch | before | 0.075 | 0.403 | 24.876 | 0.195 | 0.019 | 0.247 | 26.364 |
| near_parallel_and_exact_touch | after | 0.074 | 0.373 | 25.821 | 0.194 | 0.022 | 0.268 | 26.257 |
| moving_static_support | before | 0.120 | 0.056 | 59.752 | 0.547 | 0.027 | 0.245 | 61.842 |
| moving_static_support | after | 0.129 | 0.056 | 61.797 | 0.546 | 0.032 | 0.247 | 62.188 |

Mixed steady per-step min/max and every raw measurement are in `SUMMARY.json` / `measure/results.json`. The offset-compound whole median is **2.88% worse**; the other mixed whole medians vary by roughly −1.07% to +0.56%. All-steps medians are modestly worse in these mixed trials despite lower allocation counts. No broad speedup or statistical significance is claimed. We stop rather than tune or change another mechanism.

| Whole mixed fixture | Before calls | After calls | Before bytes | After bytes | After final cache calls | After retained cache bytes |
|---|---|---|---|---|---|---|
| rotating_boxes | 3738 | 231 | 872281 | 789617 | 0 | 66192 |
| offset_compounds | 6823 | 369 | 2949084 | 1152388 | 0 | 86672 |
| near_parallel_and_exact_touch | 7289 | 3334 | 964152 | 870736 | 0 | 66192 |
| moving_static_support | 4672 | 286 | 1638939 | 1035067 | 0 | 74576 |

All four mixed workloads release all observed heap payload on teardown. Capacity-growth allocations are recorded, including late growth as additional bodies contact; they are not mislabelled steady per-body churn. Full 120-step state checksums match all three sources in all seven repetitions. **21,632 raw sampled body-state components** also match BEFORE/AFTER exactly. This is supporting trajectory evidence alongside physical/geometry assertions, not proof based only on a hash.

## Executed correctness

- New storage suite: **921 checks**, PASS; 100 frame-map lifetimes, smaller-after-larger demand, 100 rotating compound-bound rebuilds, shape-type replacement, 80 real-world steps, moving static support, slot generation reuse, body-array growth. Same suite passes AddressSanitizer, UndefinedBehaviorSanitizer and leak detection with halt-on-error.
- Existing cache suite: **13,327 checks**, **2,894 bitwise bound comparisons**, **546 world cache inspections**, **8 analytical previous/current player sweeps**, PASS.
- Independent represented-input oracle: **823 cases / 887 primitive pairs / 1,245 contacts / 1,646 outward bounds / 823 world pairs / 1,646 queries**, PASS. **16,354 predicates / 64 exact fallbacks / 0 unresolved**. Tolerances and inputs unchanged; errors identical to the prior candidate.
- Supplied adapters: **400 tree queries**, PASS; **23 geometry witness checks**, PASS. Full temporal probe remains **13 failures among 45 checks**, exactly the existing OPEN FTFT4B witnesses. No timing repair.
- Contact lifecycle: **30 fixtures / 90 steps / 6 convenience contacts / 1,050 checks**, PASS.
- Six invalid-quaternion cases and two finite-range diagnostics preserve outcomes. Extreme finite FLT_MAX quaternion arithmetic remains explicitly unresolved; this is a retained limitation, not newly solved geometry.
- **40 production suites**, PASS (39 previous + storage); real GL async **12 cases / 246 checks**, PASS. FTFT1 valid/rejected standalone/editor smoke paths PASS. FTFT3 **136 scenes / 272 runtime constructions / 32,640 steps / 12,095 checks**, PASS.
- **21** independent player sweeps PASS, plus the existing broadphase suite’s **300** exhaustive-comparison player sweeps. Four **900-step** classic/terrain near/far app traces PASS and remain byte-identical to the prior accepted FTFT4A traces.
- Timed comparisons: **21 crate executions / 630 steps**, **21 mixed executions / 84 workloads / 10,080 steps**. Separate allocation diagnostics: **3 crate executions / 90 steps** and **3 mixed executions / 12 workloads / 1,440 steps**. These exclude other correctness-suite executions.

The first storage-test build exposed wrong API argument counts/naming; only test calls were corrected. The sanitizer build succeeded but its first metadata write used a relative path incompatible with the runner; the test then ran with corrected metadata handling. Failed build/log evidence is retained. No assertion/tolerance or numerical calculation was weakened.

Protected FTFT1–3 source/evidence and all fluid prototypes remain unchanged. Fluid prototype tests and human visual validation were **not run**. No hardware profiler or new numerical-method experiment was run in this bounded pass.

## Files and source identity

Production changes in this pass are restricted to `src/ContactSolver.{h,cpp}`, `src/Narrowphase.{h,cpp}`, `src/PhysicsWorld.{h,cpp}`. `Contacts.cpp`, `Contacts.h`, `ContactGeometryInternal.h`, `ContactPreparedGeometry.h`, rigid force/integration and existing tests are unchanged from the preserved candidate. No signed-gap, interval/exact predicate, local-anchor, friction or impact-time expression changes.

Added `tests/ContactStorageTests.cpp` and its CMake target; added test-only measurement helpers `ContactAllocationInstrumentation.h`, `ContactAllocationTracking.cpp`, `ContactMixedLifetime.h`, and `scripts/ftft4alloc_validation.py`. Updated this evidence tree, `docs/FTFT.md` and the relevant architecture paragraph. No protected files changed.

`final-source.json` records exact fingerprints and the tested source mapping. Production regression fingerprints match every final engine source and existing test; the subsequent mixed-lifetime measurement header/runner extension affects only benchmark instrumentation and was compiled/executed for every measured version. `final-source.zip`, `storage-only.patch`, `final-audit.json` and `SHA256.json` preserve the reviewable state.
