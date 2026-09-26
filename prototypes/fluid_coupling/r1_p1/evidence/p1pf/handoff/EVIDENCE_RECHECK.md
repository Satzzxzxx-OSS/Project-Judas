# P1-PF handoff evidence recheck

## What was actually checked here

The supplied Judas_Frozen_Geometry_Projection_Research.zip was extracted and all
8 entries in its SHA256.json manifest verified before any execution. The original
archive is included unchanged under evidence/original/.

The supplied projection_checks.py was then run unchanged in a separate directory:

    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python projection_checks.py

Result: 42 named positive cases and five negative controls passed. Every JSON
result other than elapsed time matched the original exactly. Elapsed time in this
rerun was about 1.169 seconds; this is not a production benchmark.

Worst normalized results:

| Measure | Value |
|---|---:|
| Constraints | 3.07279448724669e-13 |
| Impulse equation | 3.802419644014676e-14 |
| Projection energy identity | 9.509832717951329e-15 |
| Discrete momentum budget including external reactions | 1.4100336891884045e-13 |
| Fluid mass partition | 2.1960310406850083e-15 |

The reference code, results, stdout and this rerun's metadata are in reference/.
The original note is copied there unchanged and still describes its original
42-case scope. ASTRA_TASK.md is the implementation instruction.

## Added bridge evidence, separate from the original 42

To avoid specifying a completely unexercised bridge from P1-C-M to projection,
a separate adapter_reference_checks.py was implemented and executed here.

It extracts unchanged fixed-grid helper function definitions from the corrected
P1-C-M Python reference compatibility_checks.py, preserves that source in full,
and does not execute its unrelated Shapely imports or moving-geometry tests.
NumPy and SciPy are sufficient. The actual frozen-WY two-order mass/momentum
updates produce the test snapshots; prescribed advectors are not overwritten by
pressure output.

Six cases were executed, each with 12 transport timesteps and one all-fluid
periodic projection, followed by an idempotence check:
- density ratios 1, 1,000 and 1,000,000;
- uniform common co-motion;
- nonuniform prescribed transport, with the resulting mass/momentum snapshot
  supplied directly to the periodic pressure operator.

Result: six cases / 72 transport timesteps PASS. Approximately 0.580 seconds here.
The periodic operator uses one face DOF at each wrapped seam. This bridge has no
body geometry, so it is not proof of advection through solid masks.

| Bridge measure | Worst value |
|---|---:|
| Constraints | 1.5054628137499247e-13 |
| Impulse equation | 5.191450217091551e-13 |
| Projection energy identity | 2.878164541147833e-16 |
| Periodic component-momentum budget | 2.1782393790973986e-16 |
| Second-projection velocity change | 1.6940615576999107e-13 |
| Common-velocity error | 1.2262413306984854e-13 |

These are independent Python reference calculations, NOT the newly committed
Judas C++ source or a rerun of the user's 55-case executable. The C++ handoff
explicitly requires the six adapter checks to consume its actual transport output.

## Source-versus-derivation boundary

The papers support monolithic interface-impulse coupling, massive fluid duals,
transpose reactions and the variational projection principle. Our B/H notation,
resolved geometry, SVD reference and fixture contract are a derived specialization.
No source supplies this exact full Judas implementation unchanged.

The implementation brief does not rely on the earlier, unverified claim of a
universal PBF limitation. It specifies and tests this pressure component directly.

## Limits that remain

- No user repository or production files were accessed here.
- No arbitrary moving cut-cell method was developed or validated in this recheck.
- No body trajectory, long-term neutral drift, final floating immersion, thin
  subgrid shell, 3D, swimming or production frame-rate claim is established.
- Full-step energy and angular momentum across transport and moving geometry
  remain separate from the fixed-projection identities.
- Random inputs and analytic oracles test selected cases, not every possible
  geometry or condition number. Algebraic residuals alone do not validate physics.
- The SVD rank threshold and tolerance scales are explicit engineering choices.
- Whole-grid covariance is not oblique-cut-cell accuracy.

No research gap blocks the specified frozen, resolved-geometry component. The
remaining moving-geometry research blocks later integration, not this assignment.
