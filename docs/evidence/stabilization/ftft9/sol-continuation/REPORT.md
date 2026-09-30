# FTFT9 bounded continuation — 2026-09-30

HEAD/local origin tracking: `d2246cd19ad70065d121ffc5d4a4445ebca5d82b`.
FTFT4–8 are checkpointed. FTFT9 remains uncommitted and acceptance is incomplete.
This continuation follows the operator's request to finish the immediate code
repairs without another repetitive full validation campaign. No commit or push
was made; no readiness claim is made.

## Code completed

- `FluidHydrostatics.cpp/.h`: occupancy and bulk velocity/acceleration now use the
  same geometrically unoccluded exterior columns. Previously particles in a
  body's displaced/contact shadow could feed its own one-way wall acceleration
  back into analytic buoyancy. The force equation and spatial kernel are unchanged.
- `PhysicsWorld.cpp/.h`: optional auxiliary normal acceleration declines moving
  prescribed normal boundaries; ordinary bounded PGS still processes their rows.
  Eligible tentative corrections require finite, cone-valid impulses and an
  actual non-worsening closing residual, otherwise exact rollback. Main rigid
  ContactSolver, impact timing and geometry were not changed.
- `ProductionFluidCoupling.cpp/.h`: expose rejection/decline counters from the
  actual auxiliary solve. `ProductionFluidTests.cpp` adds a case selector and
  passive CSV diagnostics; fixture geometry and assertions are unchanged.
- `FluidHydrostaticsTests.cpp`: five checks cover shared exterior motion sampling,
  common-fall cancellation and rotated equivalence. `ParticleContactBatchTests.cpp`
  adds 25 checks without changing the original 52.

## Actual executions

| Execution | Result | Scope |
|---|---|---|
| Auxiliary particle-contact target | 77/77 checks PASS | Includes stiff 100:1 support and six conflicting-wall cases; caps are not physical acceptance |
| Hydrostatic target | 90/90 checks PASS, about 0.22 s | Component geometry, measured acceleration and transformed equivalents |
| `dense_020` | 727/727 checks PASS; 480 ordinary steps | 240 pool preparation +240 body steps; 432 kg, 752 particles |
| `half_020` | 729/729 checks PASS; 480 ordinary steps | 240 preparation +240 body steps; 108 kg, 752 particles |

Only these changed-path executions were performed. Earlier full-family failures
and earlier container/player evidence remain intact; they were not relabelled.
Commands, raw logs, source/binary hashes, CSVs and observations are in `run1/` and
`../particle-batch/acceleration-safety/`.

### Integrated observations

| Measurement | Dense .20 m | Half-density .20 m |
|---|---:|---:|
| Final centre height (m) | 0.2999998331 | 1.121607780 |
| Final vertical velocity (m/s) | approximately 0 | -0.2800221145 |
| Mean sampled immersion | 0.9968705177 | 0.5075053573 |
| Independent measured immersion | 1.0 | 0.5282806396 |
| Peak body speed (m/s) | 1.492953062 | 1.692776442 |
| Liquid mass change (kg) | 0 | 0 |
| Peak liquid kinetic energy (J) | 3309.215576 | 502.747101 |
| Final liquid kinetic energy (J) | 7.282509 | 57.487476 |
| Contact iteration-cap batches | 19 | 0 |
| Peak normalized closing residual | 0.749295831 | 8.49e-8 |
| Optional moving-boundary declines | 114 | 0 |
| Particle-step median (ms) | 19.344171 | 18.675192 |
| Executed hybrid median (ms) | 22.905852 | 22.182605 |
| Amortized hybrid per rigid frame (ms) | 13.532156 | 12.975909 |

These are the heavier 752-particle test pools, **not shipped workload performance**.
No complete-energy conservation assertion is made for the exterior one-way hybrid.
The earlier dense explosion reached 299,336,064 J at step 20; this execution stays
finite and the body sinks/rests, but the underlying conflicting coarse wall rows
remain unresolved when the solver reports its cap.

## Remaining work, in order

1. Dispose of the coarse particle/moving-wall feasibility limit explicitly within
   the authorized approximate model. The safeguard prevents catastrophic optional
   acceleration; it does not expel all trapped particles or satisfy incompatible
   normal constraints. Do not hide caps or claim exact conservation.
2. Correct the **preparation/oracle** of the .25 m neutral fixture before judging
   its neutral result: the captured settled surface is about .82–.90 m while the
   reference body's top is 1.0 m. It starts partly emerged and cannot be assumed
   neutrally suspended. Keep the .20 m/four-second drift requirement unchanged.
3. Resolve the unchanged strict prepared-pool micro-rest failures. Existing
   30/60 Hz diagnostics both fail the .05 m/s local-speed prerequisite; changing
   cadence alone is not a demonstrated cure. No tolerance, damping or cadence was
   tuned in this continuation.
4. Run the remaining unchanged density/resolution/rotation/light-body cases and
   current player integration after those specific issues are addressed. The
   previous player run predates the column-envelope change. Existing container
   8-case pass is from the prior accelerator; confirm retained guarantees on the
   final source once, rather than repeating all suites during development.
5. Freeze source, run FTFT9 acceptance/normal shipped-fluid performance, then the
   final clean Release engine-wide gate once. That includes persistence, real
   async, gravity, contacts, editor/standalone and classic/terrain near/far paths.
6. Only after acceptance: checkpoint/push FTFT9, complete
   `docs/STABILIZATION_STATUS.md`, checkpoint/push final stabilization, verify a
   clean synchronized repository. No milestone tag.

Sparse envelopes whose occupancy support has no spherical bulk-weight support
remain a documented sampling limitation. Exact exterior momentum/energy and
research-grade pressure coupling are outside the authorized production model.

## Protection and continuation

Protected prototype trees, FTFT1–3 evidence, persistence implementation,
ResourceManager and ContactSolver have no diff against this checkpoint. Scene
fingerprint schema 2 remains the pending FTFT9 authored-metadata update; strict
FTFT1 validation is not replaced or relaxed. Historical failure evidence remains
unchanged. Current source fingerprints and git status are captured in `run1/`.
Resume from this dirty working tree; do not reset to HEAD or discard useful work.
