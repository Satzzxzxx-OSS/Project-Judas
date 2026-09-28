# FTFT4A-P — Recover performance without weakening contact geometry

FTFT = FixThisFuckingThing.

Perform this performance pass within FTFT4A only. FTFT4B, FTFT5 and fluid work remain unauthorized. Do not commit, push or tag; report for review when finished.

## 1. Starting state

The reviewed correct-but-slow implementation is the **uncommitted worktree** in `Judas_FTFT4A_Performance_Review.zip`, based on HEAD:

`3404388f1af48419505c802fc11dbf8a0b577f38`.

Do not reset to HEAD: that would delete the geometry fixes. Verify current files against the supplied export manifest and preserve unrelated/newer work. Save a persistent pre-optimization snapshot and source hashes outside disposable build output.

Read `REVIEW.md` and the component code/results in this package. Method selection is supplied; no new contact-model or literature round is requested.

The reviewer compiled the scalar source-template/reference checks but could not compile GLM-dependent Judas here. Therefore engine-level speed and regression acceptance remain your task. The `value_carriers.hpp` file is only a storage adapter for those tests: **never add it as a GLM replacement or engine dependency**.

## 2. Goal and non-negotiable invariants

Reduce the measured ~20.979 ms / 1,500-crate cost materially while retaining:
- represented-input robust signs, genuine positive gaps and exact fallback;
- proper quaternion geometry, precise local witnesses/anchors;
- conservative bounds and full candidate coverage;
- current contact selection, warm-start, friction and iteration policies;
- all FTFT1–3 protections and isolated fluid checkpoints.

Do not restore gamma8 gap collapse, remove exact fallback, drop difficult candidates, add sleep/clamps, skip moving static objects, hide work outside the timer, or specialize on scene/object IDs. Do not implement FTFT4B's impact-time repair during this pass.

## 3. Selected changes

### A. Reuse shape and orientation work

Cache the shape-derived bounding radius rather than recomputing it in StepReach and every candidate pair.

Prepare the interval homogeneous quaternion data, ordinary homogeneous data and normalized binary64 rotation once per actual parent orientation. Reuse it for compound children, pair contexts, warm-start frames and repeated contact points where the pose is unchanged.

Keep exact expansions lazy and tied to the original represented parent pose/child offsets. Do not introduce a world-float child-centre round trip.

For BoxAabb, cache normalized interval matrix entries, individual rotated-offset products, and accumulated extents. Reconstruct the translated bound in the original order:

    center = Iv(position[k]);
    for j = 0..2: center += savedOffsetTerm[k][j];
    lo = center - savedExtent[k];
    hi = center + savedExtent[k];

Keep the existing outward binary32 conversion. Do not reassociate the three terms into a combined offset before adding position.

Maintain separate old/current pose bounds for player sweeps. At step start, identical previous/current poses may share a computed result instead of running ShapeAabb twice.

### Required cache correctness

Keys must cover exact represented values/revisions that affect the result: quaternion, shape type/dimensions, child offsets, and position where applicable. No epsilon-based pose equality. Do not assume a static body never moves: doors use ResetBody.

Handle creation, ResetBody, slot destruction/reuse, all shape changes, position/rotation integration, penetration correction, and previous/current state changes. Do not retain pointers invalidated by body-vector reallocations. A cache is derived data, never authoritative physical state.

Orientation-only data may survive a translation; translated AABBs may not. Solver pre-integration and post-integration phases must not share stale orientations. Keep allocation and memory cost visible.

### B. Factored interval SAT workspace

Implement the expression in `work/code/sat_workspace.hpp` as a per-pair internal filter workspace, adapted to the existing code.

The exact identities are:

    N_i dot N_k = d^2 * delta_ik
    N_k cross N_i = epsilon_kiℓ * d * N_ℓ
    r_ij = N_Ai dot N_Bj

They follow from the source's proper quaternion matrix R=N/d, not a near-unit assumption.

Precompute the nine cross-body dot products and d_A^2/d_B^2 once per pair. Evaluate each face/cross-axis radius using those identities instead of six fresh 3D dot products per axis.

Reuse the same axis vector for length and SAT evaluation. Keep the existing cross-axis validity predicate: a small nonzero axis is not parallel merely because it is small.

The reference verifies exact equivalence to the original polynomial and interval enclosures, and matches the supplied 823-row/887-pair classification fixtures. This is not permission to bypass any of the independent production geometry checks.

Keep the original exact predicate as fallback. A sign result is not a gap-magnitude proof: preserve robust gap construction and explicitly fall back if ordinary numerical construction contradicts a certified sign. Do not suppress unresolved diagnostics or substitute zero.

### C. Optional equivalent arithmetic housekeeping

The source singleton-product guard may replace its normal-operand ilogb calls with the exact exponent-bit identity documented in REVIEW.md. Preserve the original exceptional/subnormal path and -968 threshold; retain explicit FMA residuals.

Hoisting identical quaternion products is allowed while preserving expression association. These small changes have mixed microbenchmark payoff. Keep them only when measured useful; they are not the primary speedup claim.

Do NOT import the exploratory sign-selected interval multiplication/division rewrite from this package. Its final guarded version passed but it is not selected for integration.

## 4. Compiler and arithmetic contract

No fast-math, Ofast, unvalidated reassociation or blanket native-ISA requirement. Error-free transforms require the documented operation order. If explicit contraction controls are needed, apply them consistently to the arithmetic translation units and to both comparison builds. Explicit std::fma remains valid.

Do not compare an instrumented build with an uninstrumented one. Do not use extra threads or concurrent benchmark runs to manufacture a comparison.

## 5. Validation

Run component references once as supporting checks. Then use the ACTUAL engine:

- all existing 823 independent geometry fixtures and 1,646 enclosure checks;
- exact fallback/range diagnostics, including original difficult cases;
- all 400 real-tree replay queries;
- analytical/exhaustive player sweeps;
- first-step lifecycle/persistence and sphere wrapper anchor cases;
- complete current production suites, rigid friction/restitution/stacks;
- FTFT1/FTFT2/FTFT3 validation and relevant editor/near-far harnesses.

Add cache-focused checks that compare against uncached calculations after:
- translation with unchanged rotation;
- genuine rotation, including tiny representable changes;
- ResetBody on a static door-like body;
- compound children with nonzero offsets;
- destruction/reuse of the same slot;
- integration then penetration correction;
- previous/current player-sweep queries;
- multiple contacts sharing a body.

Compare bounds bitwise where the expression order is preserved. Compare geometry/anchors with the existing independent oracle tolerances; no assertions may be widened. Keep all uncertain cases explicit.

The new factored filter may alter fallback frequency. Report it rather than assuming that fewer fallbacks means less accuracy. The independent answers must remain valid.

## 6. Performance evidence

Keep three labelled comparison points:
1. old pre-FTFT4A production baseline;
2. current correct-but-slow export;
3. optimized FTFT4A candidate.

Use the same machine/compiler/flags and unchanged TestScaleStatistics. Run seven interleaved trials where practical, record median/range, full-step cost and all available component timers. Include cache preparation/rebuild cost inside actual engine timings.

On the original fixture retain its actual body/candidate/contact workload. Report exact fallback and unresolved counts, cache rebuilds/hits and allocation/memory costs.

Also measure meaningful small/mixed scenes containing rotating bodies, nontrivial compounds, near-parallel/fallback-heavy geometry and moving static supports. This must not be a fast path that only helps 1,500 identical aligned crates.

Local research results of ~1.7–2.6x faster predicate kernels and ~4–9x faster reused bound queries are NOT a promised engine speedup. Do not multiply them or quote them as production performance.

Performance remains a review decision. If full-step cost remains materially high, report the remaining measured bottleneck without weakening geometry or starting an unrelated solver redesign.

## 7. Delivery

Update the FTFT4 report/ledger with the exact changes, preserved guarantees, measurements and remaining limitations. Preserve pre-optimization evidence.

Report:
- source fingerprints and precise diff;
- executed tests, cache invalidation evidence and independent geometry results;
- seven-run production comparisons and mixed-workload results;
- fallback/unresolved counts and any trajectory differences;
- FTFT1–3/prototype protection checks;
- limitations and final git state.

Stop for review. No commit, push, tag, FTFT4B or another FTFT item.

This task is an implementation of specified equivalent computations and caches, not a new contact algorithm.
