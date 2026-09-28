# FTFT4A-P implementation and validation result

**Implemented; correctness checks pass. Performance acceptance remains for
operator review.** No commit, push or tag was made. Full FTFT4 is still OPEN.

## Actual engine performance

Seven cyclic-interleaved trials per source version, same machine/compiler/
flags, unchanged `TestScaleStatistics` workload: 1,500 crates plus a floor,
30 fixed steps. This is `PhysicsWorld::Step` time, not a whole rendered frame.

| Source | Median whole step (ms) | Range (ms) |
|---|---:|---:|
| old | 3.984 | 3.813–4.304 |
| slow | 19.794 | 19.144–27.465 |
| optimized | 12.876 | 12.519–14.108 |

`old` is HEAD before FTFT4A; `slow` is the preserved uncommitted geometry
repair; `optimized` is this pass. Whole-step cost fell **34.95%** from slow,
but is still **3.23×** old. The four small rotating/compound/near-parallel/
moving-static workloads improved **1.61–2.10×** relative to slow.

The complete crate test call, including creation, all 30 steps, destruction
and its exhaustive correctness oracle, **regressed 5.78%**:
2,940.352 → 3,110.312 ms median. This composite timer does not isolate the
oracle or creation. It is preserved and is not described as a speedup.

All three versions used 1,500 candidate/colliding pairs and 6,000 points at
the measured crate step. Slow/optimized reported 45,000 predicates, zero
predicate fallbacks and zero unresolved cases. No candidate or contact was
removed to obtain the timing result. Two additional supporting counter runs
used those same compiled objects and unchanged fixtures: 30 crate steps
plus exhaustive queries and 480 mixed steps. Both measured zero numerical-
gap fallbacks, unresolved cases, invalid inputs and stale prepared hints;
48 exact predicate fallbacks occurred in the mixed run. These runs supply
counter evidence and are not additional performance trials.

[Full performance tables, raw record locations, cache counters and limits](PERFORMANCE_RESULTS.md)
include the three-source mixed-workload comparisons. There were **42 timed
invocations**: 21 crate calls / 630 steps and 21 mixed calls / 84 workloads /
10,080 steps. Mixed trace samples are part of those invocations, not extra
performance runs. Setup and ResetBody work are covered by outer timers.

### Remaining measured costs

Optimized crate component medians are 0.914 ms broadphase, **8.108 ms
narrowphase**, and 3.789 ms solver phase. The solver phase now includes
post-integration interval/orientation preparation previously performed
inside proxy refresh; this is not an isolated solver-algorithm comparison.
Component medians are not additive, and their timer intervals omit initial
force/reach work. The whole-step measurement includes that work.

The frame cache still allocates 1,501 hash nodes per measured crate step.
Reported retained cache payload is 2,045,536 bytes; see [measurement scope](README.md)
for excluded allocator/padding/index overhead and process RSS records.
Genuine represented orientation changes still rebuild caches. No epsilon
key, angular clamp, sleeping or small-motion exclusion was introduced.
No new optimized call-graph profile was run, so no finer hotspot attribution
is claimed from the old slow-source profile.

## Executed correctness evidence

| Check | Actual result |
|---|---|
| Independent represented-input geometry | 823 rows, 887 primitive pairs, 1,245 contacts, 1,646 enclosure checks: PASS; original tolerances unchanged |
| Geometry arithmetic | 16,354 predicates, 64 exact predicate fallbacks, zero unresolved supported cases; same fallback count/error maxima as slow |
| Cache suite | 13,327 checks: PASS; 2,894 bitwise bounds comparisons, 546 actual-world cache inspections |
| Prepared geometry equivalence | All 823 rows / 887 pairs match uncached manifolds bitwise |
| Cache lifecycle | Translation, real/tiny rotations, static ResetBody, offset compounds, destroy/reuse, vector growth, integration/correction and shared contacts: PASS |
| Player | 8 new analytical previous/current sweeps, 21 existing independent sweeps and existing 300 broadphase/exhaustive comparisons: PASS |
| Supplied real-tree replay | 400 queries: PASS |
| Source geometry witnesses | 23 checks: PASS; world CSV byte-identical to slow |
| First-step lifecycle and sphere wrappers | 30 fixtures, 90 steps, 6 convenience contacts, 1,050 checks: PASS |
| Current production suites | 39 suites: PASS, including rigid restitution/friction/stacks |
| Real-GL async application path | 12 cases, 246 assertions: PASS |
| FTFT1 runtime/editor persistence | Existing valid/rejected-save smoke and atomicity suites: PASS |
| FTFT3 scene construction | 136 scenes, 272 constructions, 32,640 steps, 12,095 checks: PASS |
| Classic/terrain near/far | Four 900-step walks: PASS; all CSVs byte-identical to slow and near/far counterparts |
| Supporting supplied scalar references | All four programs PASS; component results are not engine speed evidence |
| FTFT4B timing controls | 13 of 45 assertions still FAIL as expected; OPEN, not passing FTFT4A evidence |

Geometry maxima remain: surface error 5.684e-14 m, midpoint-anchor error
1.271e-13 m, projected-witness error 5.684e-14 m, normal-length error
2.220e-16 and sphere signed-gap error 9.770e-15 m. Invalid zero/NaN/infinite
orientations remain explicit uncertainty. The existing finite-FLT_MAX
quaternion range limitation remains unresolved; FLT_MIN diagnostic resolves.

All 21,632 stored mixed-trajectory scalar samples match slow exactly, and
checksums over every step match. Checksums do not prove equality of every
unrecorded value. No trajectory rebaseline or tolerance change was used.

## Changes in this performance pass

- `src/ContactGeometryInternal.h`: supplied factored interval SAT workspace
  and prepared-rotation Pair construction; original exact polynomial kept.
- `src/ContactPreparedGeometry.h` (new), `src/Contacts.h/.cpp`: keyed prepared
  quaternion data, contact reuse and exact numerical-gap fallback.
- `src/Narrowphase.h/.cpp`: prepared bounds/dispatcher APIs; original uncached
  AABB computation retained for comparison.
- `src/PhysicsWorld.h/.cpp`: immutable shape radii, exact-key current/previous
  bounds, orientation reuse, diagnostics and read-only cache inspection.
- `src/ContactSolver.h/.cpp`: shared body calculations within valid solve
  phases, with post-integration orientation refresh.
- `tests/ContactCacheTests.cpp` (new), `CMakeLists.txt`: cache validation.
- `tests/BroadphaseTests.cpp`: one extra diagnostic print only in the scale
  fixture; its setup, stepping and assertions remain unchanged.
- `tests/ContactPerformanceTests.cpp`, `tests/ContactScalePerformance.cpp`,
  `scripts/ftft4p_validation.py` (new): shared-source workloads and reproducible
  comparison/protected regression runner.
- `docs/FTFT.md`, directly affected `docs/ARCHITECTURE.md`, and this evidence
  directory. The earlier FTFT4A source/tests/evidence remain preserved.

## Source identity, protection and limits

Final hashes, exact diffs against slow/HEAD, source snapshot and Git status
are recorded by `final-audit.json` and `final-source.json`. The build/run
records retain compiler commands, executable hashes and per-run source hashes.
All **3,705 protected source/evidence fingerprints** remained unchanged.
Protected fluid prototypes were not rebuilt or rerun. No human visual
validation, fluid research, contact-timing repair or FTFT5 work was performed.

Performance remains materially above the old baseline; this report does
not upgrade milestone claims or close FTFT4. The independently reproduced
zero-gravity stopping, premature bounce and impact-chain defects remain
FTFT4B issues. Terrain remains the existing approximate specialized path.
