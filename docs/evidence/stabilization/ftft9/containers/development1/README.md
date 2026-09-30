# Container integration — development 1 (failing)

This is a preserved initial FTFT9 integration run, **not acceptance**.
`run_metadata.json` records the source/binary SHA-256 values, command and exit 1;
`source/` preserves the actual tested implementation and probe.

Eight authored scenes ran through serialization, `RuntimeWorld::Build`,
`GameSession::Begin`, and 1,920 ordinary `StepPlayedWorld` calls. The 5,802 checks
reported eight failures. All four supported-container cases passed their load
and retention requirements. Free-fall and zero-gravity cases failed as below.
No probe changes the solver, projects liquid positions, injects a load, or sets
particle velocities. The zero-gravity body receives one declared external
horizontal impulse through `PhysicsWorld::ApplyLinearImpulse`.

The compound tank has five resolved wall boxes, interior x/z ±0.6 m and
0 ≤ local y ≤ 1.2 m. Its cavity metadata matches that geometry. Two practical
particle spacings (0.15/0.20 m) use solver length-scale factors 3/4 and declared
lattices 7×3×7 / 5×3×5. Each particle's mass is the existing authored density ×
spacing³ rule. No engine branch recognizes these fixtures or names.

| Case | Measured load / independent weight (N) | Retained | Relative COM drift | Final momentum residual norm |
|---|---:|---:|---:|---:|
| supported .15 m, 200 kg | 4866.486 / 4866.987 | 100% | 0.159 m | 40892.15 kg·m/s* |
| supported .15 m, 2000 kg | 4866.397 / 4866.987 | 100% | 0.159 m | 146840.86 kg·m/s* |
| supported .20 m, 200 kg | 5886.005 / 5886.000 | 100% | 0.231 m | 46989.90 kg·m/s* |
| supported .20 m, 2000 kg | 5885.998 / 5886.000 | 100% | 0.231 m | 152937.91 kg·m/s* |
| free-fall .15 m, 200 kg | — | 69.4% | 0.766 m | 1.927 kg·m/s |
| free-fall .20 m, 200 kg | — | 66.7% | 0.718 m | 2.704 kg·m/s |
| zero-g .15 m, 200 kg | — | 100% | 0.348 m | 1.830 kg·m/s |
| zero-g .20 m, 200 kg | — | 100% | 0.355 m | 2.704 kg·m/s |

`R = P_body + P_liquid − P_initial − J_external − J_gravity`.
Gravity accounting uses each subsystem's actual advanced time (60 Hz rigid,
30 Hz liquid). **The supported rows include the unmeasured external static-floor
support impulse in R**; those large values are not unexplained production
momentum error. The independent acceptance oracle there is mean contained-fluid
contact load during the last two seconds, against initial liquid mass × g.
Zero-g uses external impulse (20,0,0) kg·m/s. Free fall has no other external
support. PBF does not claim exact fluid momentum/energy conservation; the probe
records its actual residuals and declares numerical gates before execution.

First zero-g step: no contained impulse, exterior response or analytic force,
yet total kinetic energy rises from 1 J to 24.956 / 36.276 J. This first rise
comes from PBF density relaxation, before rigid contact feedback. Independent
Poly6 summation on the initial regular lattices gives peak density
1014.52965 kg/m³ at 25 / 9 particles (rest density 1000). The solver turns the
position correction into velocity without a stored compression-energy budget.
Later density-weighted XSPH smoothing is not pairwise momentum symmetric.

The final free-fall hold frame reports older liquid positions and current body
positions: at the last executed liquid step the drift was already
0.436 / 0.389 m; the final held frame adds about 0.329 m. Thus the failure is not
solely a reporting phase mismatch. The initial relaxation and contained reaction
also cause drift. Production repairs and subsequent runs must be recorded in a
new evidence directory, preserving this run.

Particle-step medians here range 0.925–2.506 ms (small 75/147-particle fixtures),
not a shipped-demo performance acceptance result. Full timings are in CSV/log.
