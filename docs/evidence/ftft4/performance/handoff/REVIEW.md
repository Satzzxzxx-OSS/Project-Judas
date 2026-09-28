# FTFT4A performance review — source-specific research and handoff

## Disposition

Keep the repaired geometry. Do not accept the current 5x production regression as resolved. The supplied source reveals avoidable repeated geometry work and an expensive per-axis interval SAT expression. I implemented and checked a more efficient, algebraically equivalent predicate formulation plus prepared-bound calculations in isolation.

**The full Judas engine was not compiled or benchmarked here.** GLM is not installed; an ordinary dependency download failed DNS, and a direct connection attempt also failed. No substitute GLM was used to build an engine. The tests below compile the actual scalar arithmetic template header with storage-only vec3/quaternion carriers; that header only reads their fields. Separately transcribed bounding-box arithmetic reproduces the actual source expression order. These are component reference measurements, not whole-frame measurements or production acceptance.

The source ZIP's 328 manifest entries matched SHA-256 and byte counts. Its declared base HEAD is `3404388f1af48419505c802fc11dbf8a0b577f38`. FTFT4A is an **uncommitted** change relative to that HEAD. Compare a prospective optimization to this export, not just `git show HEAD`.

## Evidence already supplied by the operator

The seven-run same-machine crate medians were 4.196 ms before FTFT4A and 20.979 ms after. Current component medians are 2.877 ms broadphase/proxy, 12.226 ms narrowphase/assembly, 3.172 ms solver. Component timers exclude some initial reach work; total step is authoritative. The crate fixture executes zero exact fallbacks, so removing exact arithmetic cannot cure that measured hot path.

Sources: `input/docs/evidence/ftft4/RESULTS.md`, baseline/final performance JSON and logs. The supplied gprof diagnostic repeats setup plus 150 steps and reports 915,005 BoxAabb calls, 1,365,005 interval quaternion rotations and about 197.6 million interval multiplications. Those are instrumented diagnostic call totals, not isolated final-step counts; symbol folding must not be interpreted as fallback counts.

## 1. Source map: repeated work to remove

### Bounds and shape constants

- `src/PhysicsWorld.cpp:177–188`: `TightBound` rebuilds the current AABB, then a previous-pose AABB for dynamic bodies, even when those poses are identical.
- `:194–197` and `:637–641`: `ShapeBoundingRadius` is recomputed in body reach and again on both sides of each candidate pair. It is shape-derived, not contact-derived.
- `:603–607`: each step first makes the previous pose equal to the current pose and calls `CoverStepReach`; that makes the initial pair of bound computations identical.
- `:712–724`: pose integration/position correction is followed by another proxy refresh. The old-pose bound and orientation data need not be recalculated.
- `src/Narrowphase.cpp:149–166`: each `BoxAabb` reconstructs an interval quaternion matrix and performs nine interval divisions, then redoes local offset/extent products.
- Compound children repeat that parent-orientation preparation.

### Contacts and solver

- `src/Contacts.cpp:42–55`: each `Context` constructs interval and binary64 homogeneous rotations plus normalized binary64 rotations, for both parent bodies. The same body can participate in many contacts.
- `src/ContactGeometryInternal.h:225–232`: quaternion products recur within matrix construction. `operator*` may call `fma` and two `ilogb` queries, so source-level repeated products are not necessarily costless after optimization.
- `src/Contacts.cpp:185–223`: each SAT axis builds its axis/length, and `Sat` builds the axis again and performs six full body-axis dot products.
- `src/ContactGeometryInternal.h:253–256`: body-axis dot products are repeated across all 15 SAT tests although they follow exact rotation identities.
- `src/ContactSolver.cpp:82–115,194–197`: the same body's rotations and inertia are recalculated for multiple contact points at each phase. Phase changes matter: the pre-integration and post-integration orientation are not interchangeable.

No proposed optimization changes step order, impact targets, friction, iteration counts, force integration, collision geometry or uncertainty policy.

## 2. Prepared bounds with exactly preserved arithmetic

For an unchanged quaternion, shape dimensions and child offset, prepare:

1. the interval rotation entries `n[j][k]/d`;
2. the three individual rotated-offset products for each component;
3. the outward interval extent, using the original addition association.

When position changes, start from `Iv(position[k])` and add the three saved offset terms in the **same order as the source**, then add/subtract the saved extent and use the same outward binary32 conversion.

Do not combine the offset terms first and then add position: that reassociates arithmetic. Do not skip actual updates because an object is labelled static: `ResetBody` drives moving doors too.

`work/code/bounds_cache_review.cpp` performs this calculation. **60,000 bound comparisons were bit-identical**, spanning identity, tiny and arbitrary quaternion rotations, compound offsets and translations through 1,048,576 local units. This checks cached arithmetic, not production cache invalidation or a new exact geometry proof. The source's conservative enclosure tests still must run in-engine.

Seven interleaved microbenchmark trials, 24,576 queries each:

| Orientation workload | Uncached median ms | Prepared-query median ms | Query speedup |
|---|---:|---:|---:|
| Identity | 4.874518 | 1.185889 | 4.11x |
| Near identity | 25.294925 | 2.921950 | 8.66x |
| Arbitrary with offsets | 32.926329 | 4.087954 | 8.05x |

**Preparation is excluded from the prepared-query timer.** It must be paid whenever orientation/shape/offset changes. These numbers quantify reuse opportunity; they are not an end-to-end engine speedup. Include cache construction and invalidation costs in production timing, and measure moving/rotating scenes too.

## 3. Faster interval SAT without losing certified signs

For the source's proper quaternion matrix `R=N/d`, its columns satisfy the exact identities:

`N_i dot N_k = d^2 delta_ik`,

`N_k cross N_i = epsilon_kiℓ d N_ℓ`.

These hold for the represented nonzero quaternion before any approximate normalization. Let `r_ij = N_Ai dot N_Bj`, calculated with outward intervals once per pair.

For a face axis belonging to A, the A projections are `d_A^2` on that axis and exactly zero on the other two; the B projections are the precomputed `r_ik`. For a B face axis the symmetric statement holds.

For cross axis `t=N_Ai cross N_Bj`:

- `|N_Ak dot t| = 0` when k=i; otherwise `d_A |r_(3-k-i),j|`;
- `|N_Bk dot t| = 0` when k=j; otherwise `d_B |r_i,(3-k-j)|`.

Substitute these into the **same** homogeneous SAT separation expression. Keep the source's computed cross-axis vector and length test; do not treat small nonzero cross axes as parallel. The template implementation is `work/code/sat_workspace.hpp`.

This is algebraic factorization and shared computation, not an axis-aligned fixture branch or an empirical error bound. The original exact predicate remains the fallback. A zero-containing interval must still go to exact arithmetic; it must never become touching by assumption.

### Executed checks

- 1,200 pair states, 18,000 axis tests.
- 90,000 exact-expression equality, sign and endpoint-enclosure assertions: **zero failures**.
- This comparison uses the source expansion arithmetic as an exact dyadic arithmetic reference, not a long-double approximation. It independently evaluates the original and factored polynomial expressions, then subtracts them exactly.
- The supplied 823 fixture rows / 887 primitive pairs were also evaluated through the original and factored **predicate** wrappers: identical hit/sign decisions, matching independent fixture hit expectations, 35 exact fallbacks in each wrapper, no unresolved supported cases. This is not the full manifold/anchor/PhysicsWorld run; the operator's larger fallback counts include additional paths.

Seven interleaved trials, 6,144 complete 15-axis predicate kernels each:

| Orientation workload | Original median ms | Factored (prepare each call) ms | Factored + reused rotations ms |
|---|---:|---:|---:|
| Identity | 32.144544 | 14.437143 | 14.868963 |
| Near identity | 52.828401 | 30.996974 | 26.588205 |
| Arbitrary with offsets | 165.968562 | 70.313369 | 63.378862 |

The factored query improves **1.70–2.36x** even when pair rotations are rebuilt every call; with saved rotations it improves **1.99–2.62x** in the two rotated workloads. The tiny identity prepared/nonprepared reversal is timing noise/code-path cost, not a reason to special-case fixtures.

These kernels evaluate all axes and lengths, not clipping, contact solving or production early-exit distribution. **Do not multiply these factors to predict a 20.979 ms engine result.** Full-engine timing is still required.

### Gap values are not just signs

Retain the existing binary64 construction/normal/anchor path and exact fallback. A cheaper sign filter does not certify a poorly rounded numerical gap magnitude. Check sign/value consistency; recompute the exact numerator where required rather than swallowing an `ArithmeticFailure` or inventing zero. The supplied fixture wrapper found unchanged fallback decisions, but that is not a universal theorem for all inputs.

## 4. Small arithmetic opportunities and rejected experiments

The current singleton interval-product guard calls `ilogb(abs(a))+ilogb(abs(b)) >= -968`. For finite normal operands, exponent bits give exactly:

`(ea-1023)+(eb-1023) >= -968` iff `ea+eb >= 1078`.

A bit-field implementation retains the original path for subnormals/nonfinite operands and does not change the threshold or FMA residual calculation. It passed all **4,186,116 normal exponent-field pairs**, 10,000 subnormal comparisons and interval endpoint checks. Quaternion-product hoisting also preserved the tested interval/double/expansion results.

Across the combined arithmetic harness there were **5,393,305 assertions with zero final failures**. Raw records are included. Isolated performance was workload/compiler-sensitive: these local changes are optional secondary optimizations, not the main repair.

I also tried sign-selected interval multiply/divide endpoint formulas. The first form differed on 2,114 overflow-range checks under GCC 14 -O3; the reproducer passed at -O0/-O2 and with tree-VRP disabled. That indicates an optimizer-sensitive interaction but does not establish a complete compiler root cause. A finite-result guard preserved the original fallback and removed the discrepancies. The guarded version passed, but did not offer a compelling universal speedup. **It is not recommended for this handoff.** The rejected/guarded evidence is preserved instead of silently discarded.

A separate hardware-FMA-target build attempt timed out during compilation and produced no executable/results. It supplies no performance or correctness evidence. No hardware-specific flag is required by the selected route.

## 5. Compiler contract

Measurements used GCC 14.2.0, `-O3 -std=c++17 -ffp-contract=off`, default x86-64 target, on this shared container host. Explicit `std::fma` remains available; unintended contraction/reassociation is not permitted. No `-ffast-math`, `-Ofast`, blanket `-march=native`, thread-count change or sleeping was used.

GCC documents the difference between explicit/automatic contraction and the assumptions introduced by fast math. Error-free transforms must retain their intended operation sequence. This compiler contract is part of the comparison, not a way to speed up the code by dropping safeguards.

## 6. Implementation recommendation

Astra should perform ONE FTFT4A performance pass:

1. Save and measure the current correct-but-slow worktree as the reference.
2. Cache shape radii, interval/ordinary parent rotation data and prepared child-bound terms with exact invalidation.
3. Use the factored interval SAT workspace, keeping original exact predicates and the robust construction path.
4. Reuse body rotations/inertia within each solver phase without crossing pose changes.
5. Include small equivalent arithmetic substitutions only when measured worthwhile.
6. Re-run independent geometry, bounds, actual-tree, lifecycle, player, rigid, async, persistence, gravity and near/far checks.
7. Compare seven-run full-engine medians and varied moving/rotating/compound workloads under the same compiler/build conditions.

No FTFT4B timing fix, no other FTFT item, and no fluid changes. A source-local experiment is not production acceptance. The requested result is a measured reduction without weakened geometry, not a promised 4 ms before the engine is actually run.

## 7. Remaining obligations

- Engine-level cache invalidation across ResetBody, slot reuse, compounds, integration, penetration correction and previous/current player sweep poses.
- Full manifold/anchor and numerical-range behaviour with the cheaper filter.
- Actual engine and whole-step performance, including memory/allocation cost and fallback-heavy workloads.
- The known FTFT4B timing defects and FTFT4A extreme-quaternion range limitation remain open, unchanged.

No new source export is needed. The included implementation brief tells Astra precisely which work to perform and which evidence is still required.

## Primary references used for general principles

- Shewchuk, adaptive exact geometric predicates: https://www.cs.cmu.edu/~quake/robust.html
  Supports adaptive filtering as a principle, not the specific quaternion SAT factorization here.
- GCC optimization options: https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
  Floating-point contraction/fast-math behaviour.

All source-specific findings come from the uploaded export. All measurements explicitly labelled as local are from the included component runners. Operator engine measurements remain attributed to the supplied report.

## Fresh extraction verification

After packaging, I extracted into a separate directory and rebuilt/reran all four component programs. All validation counts, verdicts and non-timing benchmark outputs matched. The first aggregate tool invocation was interrupted after the arithmetic runner; the other three were then executed separately successfully. This is recorded in `work/results/FRESH_EXTRACTION_VERIFICATION.json`. No complete Judas engine run is implied.
