# M55 human drain/refill correction

Starting HEAD remains `d9ebc8c7a987047b1d4175ed5d5de7da8dea472d`.
Narrow corrected candidate, **awaiting human re-test**. Nothing committed/pushed/tagged.
The earlier 103-suite gate is preserved; it was not repeated.

## Measured cause

The defect is M55 surface state/iteration handling, **not M54 ownership accounting**.
All reproduced failures retained the 161 m³ water total within roundoff. See
[original human report and failure sequence](HUMAN_FAILURE.md). No ledger code or
project content was changed in this follow-up.

1. An accepted withdrawal reduced cell quantities but retained the complete old
   face discharge. Near empty, the donor had only roundoff water (~4.9e-14 m³ in
   the partition), yet the predictor still demanded ~0.1314 m³/s transport. The
   pressure equations were asked to support motion without donor water.
2. A dry receiver legitimately accepted one cell's capacity (1.3624999868 m³),
   not a disconnected 20 m³ deposit. Its head was exactly at the cell's upper
   capacity boundary. A strict interior test assigned a 1e-9 regularized
   Jacobian instead of its actual one-sided storage slope (~0.5 m²). This
   unnecessarily ill-conditioned the Newton correction as wet cells activated.
   The momentum-only intermediate reproduced the exact reported bounded-iteration
   failure on the first refill release: 124 cumulative PCG iterations across
   retries, residual 1.71505e-9 m³ against a 1.3625e-9 target.
3. After fixing those transitions, repeated cycles exposed paired-transfer
   rounding: proportional outgoing face products could overspend a tiny donor
   by one ulp. Example: 8.864449e-14 m³ old storage became -1.262177e-29 m³.
   Rejecting this correctly rolled the step back, but repeated re-wetting must
   not generate that invalid candidate in the first place.

## Small generic correction

- Withdrawal takes the outgoing donor fraction of its existing transport momentum.
  Unaffected faces are unchanged; retained nonzero waves survive. Incoming quantity
  starts at rest; explicit arrival impulses remain separate.
- Exact empty/full boundaries use the physical interior capacity derivative,
  with the existing positive Jacobian floor. Outside physical storage remains
  regularized. The capacity function itself is unchanged.
- Proportionally limited shared transfers consume remaining donor/receiver budgets
  **before** each paired debit/credit. Incoming amounts accumulate separately and
  cannot be spent again in that step. This bounds rounded transfers, rather than
  clamping negative final cell volumes or repairing a failed solve.

The solver now starts full newly wet cells with their real storage derivative,
without orphan donor flow, and commits representable nonnegative paired transfers.
The existing 12 Newton iterations, configured PCG budget, 16 line-search attempts,
three-level half-step retry policy, tolerances, friction, dimensions and **60 Hz**
liquid cadence are unchanged. No reset to equilibrium, damping increase, extra
iteration budget, topology bypass, global rescale or demo-control exception.

## Focused proof

[Core](core.log): **61 checks / zero failures**, including all previous 52 checks:
calm fully wet and dry shoreline, localized disturbances/reflection/half-step,
1000→200→1000 L with nonzero retained waves, donor momentum accounting, full local
re-wetting at the physical capacity coordinate, separated islands, raised dry-sill
isolation and actual wet-face reconnection, nonnegative cells, true radial and
oblique gravity, and exact failed-solve partition/discharge rollback.

[M54 conservation](m54-conservation.log): **67 / zero failures**. Its implementation
and transfer ledger are unchanged, but surface transaction state changed, so the
existing focused conservation checks were rerun.

[Flat five cycles](flat-five-cycles.log), [radial five cycles](radial-five-cycles.log):
**17 / zero failures each**. Ordinary registered scenes, public authored Z/X logical
input, real JS callbacks, physical bodies/cups and normal fixed simulation. Each
cycle fully drains, refills through the accepted connected allocation, then settles
120 fixed steps. No fixture writes quantities. Every step checks owner/partition,
M54 total, finite/nonnegative storage and absence of solve/script errors. Final
flat pool quantity can be slightly below 160 m³ because legitimate splash parcels
own the difference; it is not repaired. Observed total-ledger error remains on the
order of 1e-13 m³; the final radial error is 0.

[Flat application](flat-application.log), [radial application](radial-application.log):
**20 / zero failures each**: actual opening scoop/carry/pour, waves, JS swimming,
render/query state, reload/Stop and stale-handle cleanup.

Affected Release targets build with zero warnings. Source flat/radial normal
Application loops and moved-package normal loop pass using native X11 graphics.
The refreshed actual exported executable also starts from unrelated `/tmp` and
runs 120 fixed steps. The fixed-only executable harness is startup evidence,
not visual acceptance. No source/API/asset format or packaging change was needed.

## Solver measurements

Sequential Release fixed-step runs, no solver tracing, same machine. These are
surface solve times, not whole-frame time. Iterations count cumulative PCG work
per step, including retries, rather than nonlinear iterations.

| Scene/phase | Steps | Mean/max iterations | Mean/max solve ms | Max final residual m³ | Retry splits |
|---|---:|---:|---:|---:|---:|
| Flat ordinary | 30 | 6.40 / 13 | 0.435 / 0.783 | 1.593e-7 | 0 |
| Flat refill | 112 | 23.32 / 91 | 0.640 / 2.639 | 1.454e-7 | 2 |
| Radial ordinary | 30 | 6.67 / 8 | 0.864 / 1.389 | 5.758e-8 | 0 |
| Radial refill | 110 | 32.74 / 82 | 0.704 / 1.463 | 5.013e-8 | 0 |

Residual targets remain 1e-9 × max(1, owned m³); different quantities imply different
absolute targets. All final measured steps passed their existing bounded policy.
Flat refill used two existing half-step retry splits across all five cycles; radial
used none. Two seconds of post-refill settling per cycle also passed.

Original first-refill failure: **22 iterations, 3 retries, residual 7.842e-4 m³,
0.556 ms**, rejected. Repaired first refill: **16 iterations, no retries,
residual 4.959e-10 m³, 0.436 ms**, accepted. Original ordinary sample was 9
iterations / 4.508e-10 m³ / 0.370 ms; final ordinary samples above include normal
body motion. This is a convergence correction, not a timing optimization claim.
The intermediate exact-budget failure took 2.239 ms **with diagnostic tracing**;
that time is preserved but is not directly comparable to untraced benchmarks.

## Scope, preservation and handoff

Existing candidate changes are restricted to `CMakeLists.txt`,
`src/LiquidSurface.cpp`, `tests/LiquidSurfaceTests.cpp`,
`docs/LIQUID_SURFACES.md`, `docs/judasjs/liquid.md`, plus new
`tests/LiquidSurfaceRefillTests.cpp` and this evidence/current evidence indexes.
Temporary narrow tracing was removed. No changes to M54 ledger, solver cadence,
project scripts/scenes, baked assets, gravity selection, UI or rendering.

Initial source fingerprints and original failures remain here; the root final
fingerprint manifest is refreshed for this correction. Exact full M55 scope is
listed in `../CHANGED_FILES.txt`. Protected FTFT/P1, historical M33–M54 work,
prototypes, prior projects and legacy PBF remain unchanged. No later milestone work.

Re-test the clean flat demo: press **Z** repeatedly until empty, then **X** until
refilled; repeat, disturb with **V**, and inspect surface/error/TOTAL. **2** selects
the true radial scene for the equivalent operation; **R** reloads authored state.
Human visual/interactive acceptance remains outstanding.
