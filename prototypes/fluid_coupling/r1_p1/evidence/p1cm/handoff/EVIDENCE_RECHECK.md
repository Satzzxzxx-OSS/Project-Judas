# Evidence recheck for the Astra P1-C-M handoff

## What was executed

The original user-visible research archive was extracted and all four hashes in its
SHA256 manifest were checked: all matched. Its Python program was rerun successfully
in this environment. No Judas repository was available or modified by these checks.

While inspecting the source for this handoff, I found an additional restriction issue:
the original reference summed mass transfers on a staggered face and chose a donor
from the net sign. That loses transport when constituent fine-face segments counterflow.
The revised reference upwinds each fine-face segment first and sums momentum afterwards.
The original archive is retained without changes; use `evidence/rechecked/` for this task.

I also tightened the revised research program's fraction assertions to the existing
P1-C value, 256 double epsilon. The original program asserted 1e-12, although its
recorded fraction excursions were already smaller than the P1-C tolerance. The revised
program rejects materially out-of-range PLIC inputs before reconstruction and records
roundoff endpoints without rewriting the fraction field. Explicit total-mass and final
mass-restriction assertions were added. No physical damping, mass floor, or repair was added.

The revised program completed 18 fixed-grid cases / 216 timesteps plus its separate
analytic counterflow and conserved-quantity averaging checks. It also retains the
four earlier geometric co-motion checks; those are OUTSIDE the assigned Astra task.

Run from this package:

    python evidence/rechecked/compatibility_checks.py

Dependencies and exact executed interpreter/library versions are recorded alongside it.
Running this rewrites the corresponding results JSON; preserve the shipped copy first
when comparing. Runtime and environment fields need not match between machines.

## Results from the revised program

| Quantity | Worst value |
|---|---:|
| `normalized_total_mass_error` | 1.2423870930931906e-16 |
| `normalized_total_momentum_error` | 3.2545970421612077e-16 |
| `normalized_dual_mass_ledger_error` | 3.1990421999623917e-16 |
| `normalized_mass_restriction_error` | 2.9422444931051596e-16 |
| `uniform_velocity_error` | 5.6851190421980391e-12 |
| `sweep_mean_vs_shared_flux_error` | 2.2204460492503131e-16 |
| `max_full_step_relative_energy_increase` | 1.751567874091627e-16 |
| `max_velocity_bound_overshoot` | 5.6851190421980391e-12 |

Raw fraction range: -1.1796119636642288e-16 to 1.0000000000000027.

Counterflow check: +0.25 and -0.25 mass transfers, with donor component velocities 2
and -1, yield exactly 0 net mass transfer and +0.75 momentum transfer. Net-first
donor selection yields 0 and fails that analytic check.

Averaging check: branch masses 1 and 9, velocities 1 and 0 yield combined mass 5,
combined momentum 0.5 and velocity 0.1. Arithmetic velocity averaging yields 0.5,
which would imply incorrect momentum 2.5.

The intentionally wrong OLD-mass normalization produced large spurious velocities
at high contrast; its maximum recorded deviation was approximately 94648.49.

## Limits of this evidence

- This is independent Python research, NOT compiled Judas validation.
- Fractions use an independent low-order normal estimate and PLIC construction; the
  task preserves Judas's existing accepted reconstruction instead.
- The periodic reference has only 24x24 fine / 12x12 staggered cells and twelve
  timesteps per case. It does not establish momentum accuracy under refinement.
- The reference is periodic. The specified open-boundary slab and physical boundary
  half-dual-volume budgets have NOT been numerically exercised here; their expected
  integrals follow directly from translating [0.8,1] by 0.15 over a unit-height domain.
- Smooth/random and constant-vector transport with a distinct nonuniform prescribed
  advector are manufactured transport checks, not pressure-coupled flow solutions.
- Original normalized mass-ledger diagnostics use the normalizations in the script,
  which include max(1, mass magnitude). The task asks additionally for stricter local
  scales and raw residuals so a domain-wide normalization cannot hide a local error.
- Observed full-step energy behaviour on these fixtures is not a general stability
  theorem. The convexity inequality establishes only the branch-combination property.
- There is no moving-body transport, pressure, force, torque, buoyancy, swimming or
  production performance evidence here.

## Sources independently checked for this handoff

1. Pal, Fuster and Zaleski, arXiv:2101.04142v1 (2021).
   https://arxiv.org/pdf/2101.04142
   Checked §§3.2 and 3.4, Eqs.21,25–26,32–33, the statement freezing the central
   velocity to timestep n, and Eq.40/Figs.4–5. PDF pages 12,13,17,18,19 were also
   inspected as rendered pages. Supports the two-grid representation, restriction,
   matching flux/source structure and distinction between stage/current and frozen
   source velocities. It is not the JSGT scheme.

   Notational caution: printed Eq.34 uses C^(n,l), while adjacent discussion invokes
   the direction-independent WY coefficient. The handoff does not silently interpret
   that printed line as our frozen binary field: its S equation is derived explicitly
   from the already-approved Judas fraction update. Do not reopen P1-C on that basis.

2. Huang et al., arXiv:2606.02467v1 (2026 preprint).
   https://arxiv.org/html/2606.02467v1
   Eqs.5–7 distinguish previous-direction advected velocity from timestep-n source
   velocity and normalize momentum by the matching updated mass/density. Its Eq.8
   discusses density-weighted RK2; our branch averaging is separately derived, not
   claimed to be that temporal integrator. We are not implementing its high-order
   SynDRoM reconstruction, viscosity modifications or full flow solver.

3. Basilisk source `vof.h`, accessed in this research pass.
   https://basilisk.fr/src/vof.h
   `cc[] = (c[] > 0.5)` is computed before directional sweeps. This is corroboration
   of the frozen numerical coefficient. Its embedded-boundary approximation and
   fractional/tracer repair behaviours are not authorization for Judas substitutions.

## Evidence classification

Published ingredients: common fine-grid volume/mass/flux representation, staggered
restriction, matching mass/momentum directional updates, frozen correction sequencing.

Our derivations: integrated-mass form of S from Judas's actual fraction update;
JSGT conserved-quantity branch combination; common-vector and global-budget algebra;
combination-energy inequality; exact open-slab and counterflow arithmetic.

Executed evidence: the explicitly limited Python checks above.

Still to be earned: the compiled C++ prototype's integration, unchanged 25-run P1-C
regression, new momentum refinement/open-boundary/axis-exchange checks and all results
required in ASTRA_TASK.md. No result for those may be copied from this package.
