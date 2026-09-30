# FTFT9 actual-brief resumption: blocked boundary update

**Paused for operator review at a demonstrated boundary defect. No numerical
repair, production source edit, commit or push was made in this resumption.**

## Scope correction

The stabilization brief accepts PBF visual motion plus approximate analytic
hydrostatics/drag, finite-mass declared cavities and controller swimming. Exact
exterior momentum conservation and continuum CFD are not requirements.

Individual-particle mean speed below 0.05 m/s and the 0.864 kg, 0.12 m-wide body
at 0.20 m spacing are additional diagnostics, not the stated FTFT9 acceptance
requirements. Their historical failures remain preserved; none is relabelled a
pass. No assertion or source fixture was changed during this scope audit.

Required coverage still includes independent half-density immersion, neutral
drift, sinking, gravity/frame consistency, container retention/load, light-body
stability, swimming and shipped performance. An actual particle left in solid
geometry cannot be excused as a harmless micro-rest diagnostic.

## Current restored-code result

The existing production body target was rebuilt once, then its unchanged
`half_025` case was executed once: **731 checks / 480 ordinary fixed steps / zero
failures**, exit 0. Independent mean immersion was **0.561912537**, sampled
immersion **0.484137118**, final vertical velocity **+0.388659954 m/s**. Even its
extra micro-rest prerequisite passed (0.0499571 m/s). This qualifies the earlier
rejected-candidate miss; the restored coarse half-density case is not currently
a demonstrated acceptance failure. The remaining family was not rerun.

## Current production FluidWorld closing-gap witness

The small probe links the current Release engine archive and calls the existing
FluidWorld step. It does not implement a separate collision algorithm.

- Zero gravity; one finite-mass particle, unchanged density/collision machinery.
- Particle radius 0.072 m, matching the production 0.20 m-spacing configuration.
- Floor top y=0; body underside translates from y=0.18 to y=0 during 1/60 s.
- Initial particle centre y=0.075 m: clear of both expanded solids initially.
- Final centre **y=-0.071999997 m**, vertical velocity **-10.80000019 m/s**.
- **Inside the physical floor: true.** Particle count remains one.
- Diagnostic physical-witness exit: **1**, preserved unchanged.

At the closed gap, the currently enforced collision skins require y>=+0.072
for the floor and y<=-0.072 for the body's underside. There is no local feasible
intersection. `ApplySolidCollisions()` resolves the floor, then overwrites that
position with a later body projection; repeating two passes repeats the conflict.
Sweeps are already reapplied after PBF corrections. More iterations alone cannot
supply lateral fluid displacement or satisfy those incompatible conditions.

This component witness proves the current boundary-update defect. It is not a
new whole-application acceptance run. Commands, source hashes, build/compiler
flags, raw outputs and binary hash are in [results.json](results.json). Existing
source hashes are unchanged across execution.

## Proposed fix state — not implemented

Two explicit repairs should be evaluated together:

1. Use resolved authoritative rigid motion for liquid boundaries rather than an
   unconstrained velocity-predicted endpoint. Apply analytic hydrostatic/drag
   before the rigid step, then advance the 30 Hz particle field against actual
   completed rigid motion. This removes fictitious body/floor overlap from a
   predictor which ignores rigid contact resolution. Preserve finite-mass cavity
   exchange and the existing force ownership; do not reapply gravity.
2. Separate **hard physical solid exclusion** from a **soft numerical collision
   skin**. A sampling radius is not an incompressible grain size. Where geometric
   skins overlap, physical-solid nonpenetration takes priority; desired skin
   clearance may be relaxed geometrically. A floor-only final pass is insufficient
   because it can leave particles in the body. The repair must validate exclusion
   from the solid union and preserve particle mass, not discard contacts or matter.

These are proposed approximation changes, not claims of an implemented solver.
The exact soft-skin update and motion-history seam still require a concrete
implementation specification before code changes. The closing-gap witness,
dense-pool spatial containment, cavity load/retention and shipped performance
must establish the result. Prior endpoint-escape attempts remain rejected.

## Other findings, not silently repaired

`FluidHydrostaticField::Query()` uses larger column-envelope occupancy support
than its spherical velocity/acceleration support. A fully wet query can therefore
lose a constant acceleration field. Same admitted-column motion interpolation
is a small defensible correction, but was not implemented after the boundary
blocker was established.

The old rotated-player endpoint failure is not explained by a zero bulk weight.
Its recorded local PBF acceleration accounts for the measured force deviation;
the player also starts with identity orientation, so its initial sampling geometry
is not a strict rotation of the reference fixture. Neither observation is grounds
to widen its assertion or claim an unexecuted fix.

## Stop state

Main/local origin-main remain `d2246cd19ad70065d121ffc5d4a4445ebca5d82b`.
Previously uncommitted FTFT9 work is preserved. FTFT1–3 evidence and fluid research
prototypes are unchanged. No full repeated regression, new fluid model or source
checkpoint was attempted. Await the operator's disposition of the proposed
boundary approximation before continuing.
