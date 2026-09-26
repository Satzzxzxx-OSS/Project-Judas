# P1-C JSGT results — FAILED / INCOMPLETE

Judas Symmetric Geometric Transport (JSGT) was implemented and tested in the isolated prototype. The uniform/zero-directional-divergence gate passed. The first nonconstant, nonuniform divergence-free fixture failed at its first y predictor. **Stop: no moving-solid, cut-cell, pressure/body, P1-D/E, or P2 work was resumed.**

## Actual algorithm and changes

- `main.cpp::jsgtStep()` integrates R(Cn), R(X(Cn)), and R(Y(Cn)) separately; it averages geometric face fluxes and applies one shared-face divergence. It never reconstructs averaged scalar fractions.
- `reconstruct()` and `geometricFaceFlux()` use unit-cell coordinates to remove global-origin cancellation. The alpha tolerance remains 256*double epsilon, or 5.6843418860808015e-14. No alpha clamp or post-update repair was added.
- `transport_oracles.hpp` supplies independent analytical cell fractions: rectangle overlaps; piecewise-linear polygon integration; circle-chord integral minus a rectangular internal slot. It never calls transport/PLIC geometry.
- Uniform-translation, oblique, slotted and reversal errors are tested at 24/48/96. One complete rotational revolution runs at 32 squared. Each stage reports raw fraction bounds, strict and material excursions, flux magnitude, and update residual.
- `METHOD.md` and `P1C_METHOD.md` declare the method change honestly. Earlier scalar-average evidence is retained separately; it is not renamed as JSGT evidence.
- The existing normal estimate is centered axial differences, not the weighted Youngs stencil. The existing degenerate-gradient normal fallback remains a reconstruction limitation. No reconstruction tuning was performed.

## Uniform refinement

L1 = sum(cell area * abs(numerical fraction - independent analytical fraction)). The reported order is log2(error on the previous grid / current error).

| Fixture | Grid | dt | CFL | L1 | Observed order | Volume error | Runtime (s) |
|---|---:|---:|---:|---:|---:|---:|---:|
| aligned_translation | 24² | 0.08333333 | 0.34 | 0.00155282471 | — | 2.082e-17 | 0.003588 |
| aligned_translation | 48² | 0.05 | 0.408 | 0.00076505762 | 1.021 | 1.388e-17 | 0.012775 |
| aligned_translation | 96² | 0.025 | 0.408 | 0.000353510298 | 1.114 | 7.633e-17 | 0.075391 |
| oblique_translation | 24² | 0.2 | 0.336 | 0.0013328298 | — | 2.776e-17 | 0.002142 |
| oblique_translation | 48² | 0.1 | 0.336 | 0.000575155717 | 1.212 | 2.776e-17 | 0.007183 |
| oblique_translation | 96² | 0.05 | 0.336 | 0.000222963155 | 1.367 | 2.776e-17 | 0.039066 |
| slotted_translation | 24² | 0.1 | 0.312 | 0.00281522097 | — | 1.388e-17 | 0.004498 |
| slotted_translation | 48² | 0.06666667 | 0.416 | 0.00109806934 | 1.358 | 1.249e-16 | 0.016299 |
| slotted_translation | 96² | 0.03333333 | 0.416 | 0.000591766986 | 0.892 | 1.943e-16 | 0.084135 |
| reversal | 24² | 0.1 | 0.264 | 0.00214503526 | — | 0.000e+00 | 0.002942 |
| reversal | 48² | 0.06666667 | 0.352 | 0.000968365555 | 1.147 | 6.939e-18 | 0.012095 |
| reversal | 96² | 0.03333333 | 0.352 | 0.000402551373 | 1.266 | 6.939e-18 | 0.070771 |

All four error sequences decrease on both refinements. This is evidence for these fixtures, not a general convergence theorem.

## Bounds, conservation, and composition identity

| Fixture | Grid | Min alpha, all stages | Max alpha, all stages | Max update residual | Sweep-order identity error | Result |
|---|---:|---:|---:|---:|---:|---|
| constant_gas | 24² | 0 | 0 | 0.000e+00 | 0.000e+00 | PASS |
| constant_liquid | 24² | 1 | 1 | 0.000e+00 | 0.000e+00 | PASS |
| aligned_translation | 24² | -1.6653345369377348e-16 | 1 | 3.469e-17 | 2.220e-16 | PASS |
| aligned_translation | 48² | -2.2204460492503131e-16 | 1.0000000000000002 | 3.469e-17 | 2.220e-16 | PASS |
| aligned_translation | 96² | -2.2204460492503131e-16 | 1.0000000000000004 | 1.041e-16 | 2.220e-16 | PASS |
| oblique_translation | 24² | -6.2450045135165055e-17 | 1 | 2.776e-17 | 1.110e-16 | PASS |
| oblique_translation | 48² | -1.8045189908305614e-16 | 1.0000000000000002 | 2.776e-17 | 2.220e-16 | PASS |
| oblique_translation | 96² | -1.6642503347652493e-16 | 1.0000000000000002 | 4.163e-17 | 2.220e-16 | PASS |
| slotted_translation | 24² | -2.7755575615628914e-17 | 1.0000000000000002 | 9.714e-17 | 1.110e-16 | PASS |
| slotted_translation | 48² | -1.9428902930940239e-16 | 1.0000000000000002 | 1.110e-16 | 2.220e-16 | PASS |
| slotted_translation | 96² | -2.2204460492503131e-16 | 1.0000000000000002 | 3.331e-16 | 2.220e-16 | PASS |
| reversal | 24² | -1.3877787807814457e-17 | 1 | 2.082e-17 | 1.110e-16 | PASS |
| reversal | 48² | -2.2204052979462887e-16 | 1.0000000000000002 | 4.857e-17 | 2.220e-16 | PASS |
| reversal | 96² | -2.2204451366396977e-16 | 1.0000000000000002 | 9.021e-17 | 2.220e-16 | PASS |
| slotted_rotation | 32² | -5.377642775528102e-17 | 1.0000000000000002 | 1.249e-16 | 2.220e-16 | PASS |
| strain_constant_gas | 32² | 0 | 0 | 0.000e+00 | 0.000e+00 | PASS |
| strain_constant_liquid | 32² | 1 | 1 | 0.000e+00 | 0.000e+00 | PASS |
| nonuniform_strain | 24² | -0.00390625 | 1 | 2.498e-16 | 0.000e+00 | FAIL |

The sweep-order identity is checked only where each directional velocity divergence is exactly zero. A zero diagnostic in nonuniform cases means not evaluated, not a proof of that identity.

Uniform constant gas/liquid remain bit-exact. Across successful uniform/rotation fixtures, worst volume drift is 1.943e-16, and worst stage residual is 3.331e-16. Raw excursions are roundoff scale and are recorded rather than repaired.

The rotation uses u=-2*pi*(y-0.5), v=2*pi*(x-0.5), 360 timesteps, dt=1/360, max directional CFL 0.270526. Its one-revolution return L1 is 0.005782348524867277; runtime is 1.02566947 s. Directional divergences vanish for this field, so rotation does not substitute for the separate nonuniform strain gate.

## First material failure: nonuniform divergence-free strain

The exact prescribed field is u=x-1/2, v=-(y-1/2). Its directional divergences are +1 and -1. The analytical flow map is x(t)=1/2+(x0-1/2)exp(t), y(t)=1/2+(y0-1/2)exp(-t), with unit determinant and exactly conserved liquid area.

The liquid initially occupies x=[1/4,3/4], y=[1/4,145/192]. The failed run uses 24², dt=1/32, max directional CFL=0.375. Maximum measured total MAC divergence is 2.6645352591003757e-15, against directional magnitudes approximately one.

At step 0, **`predictor_y`, cell (6,18)** is the first material invalid operation:

| Quantity | Measured / analytical value |
|---|---:|
| Cell area | 1/576 |
| Cn | 1/8 = 0.125 |
| Bottom face velocity | -1/4 |
| Top face velocity | approximately -7/24 |
| Bottom signed phase-area flux | -1/4608 = -0.00021701388888888888 |
| Top signed phase-area flux | 0 |
| Conservative part Cn-(Ftop-Fbottom)/V | 0 |
| dt * div_y(v) | -0.031249999999999972 |
| Added continuous-C dilatation contribution | -0.0039062499999999965 |
| Actual predictor Cy | **-0.00390625** |

The same failure mechanism has a horizontal-interface witness away from the block corner, e.g. cell (7,18), which is printed in the trace. Its conservative part is 6.94e-17 and its predictor is -0.0039062499999999167.

The independent mechanics/geometry calculation is particularly simple. Initial liquid thickness in the top partial cell is dy/8 = 1/192. The downward bottom donor strip is (1/4)*(1/32) = 1/128, so it exports the entire liquid area. No liquid enters from the empty cell above. The explicit continuous-C divergence term then subtracts (1/8)*(1/32)=1/256 from a cell already emptied by the geometric flux. The exact advected upper surface falls below y=3/4, so the analytical liquid fraction in that row is zero.

**This is not the previous scalar-average corrector failure.** It precedes reconstructing Y and precedes JSGT final flux averaging. The retained geometric donor flux plus explicit continuous-C directional divergence correction is not bounded under this nonuniform compressing sweep. No limiter or changed predictor was attempted.

The y-predictor balance closes with a residual of about 2.50e-16 when its directional dilation is included. Predictors are advective intermediate states and are not separately mass-conservative. No full nonuniform step was accepted: the reported accepted-state volume error of zero reflects that rejection. It is **not** evidence that nonuniform JSGT conserves a completed trajectory. No final-time L1 or nonuniform refinement claim is made. The 48/96 nonuniform runs were not started.

## Manufactured constant-field fixture correction

The first harness used the 24² strain field for a bit-exact constant test. Face sampling on that non-dyadic grid produced a 2.66e-15 total-divergence residual; the liquid remained within about 6.7e-16 of one but failed the exact-equality assertion. This is retained in `results/jsgt_initial_constant_fixture_*`.

Only that constant test was moved to 32², where the affine velocities, cell spacing and timestep are exactly representable. Its divergence is then exactly zero, and both constant fields remain bit-exact. No assertion or alpha tolerance was relaxed. The hostile 24² nonconstant strain fixture was left unchanged and produced the material failure above.

## Runtime, scope, and evidence

Total measured fixture runtime in the final run: 1.360215 s (includes reconstruction, analytical error evaluation and CSV instrumentation). These are prototype measurements, not production benchmarks.

- `results/jsgt_stdout.txt`: complete run and first-failure neighborhood.
- `results/jsgt_summary.csv`: per-fixture resolutions, dt/CFL, ranges, residuals, errors, timings and completion status.
- `results/jsgt_trace.csv`: each actual predictor/final stage; absent final rows mean the failure stopped before that operation.
- `results/jsgt_run_metadata.json`: source hashes, reference HEAD, compiler and build information.
- Earlier scalar-average experiment remains in `results/p1c_trace.csv` and `results/scalar_average_P1C_RESULTS.md`.

This executable transports volume fractions under prescribed velocities. It does not evolve momentum, energy, pressure, or rigid bodies; no claims about their conservation or coupling are earned here.

**Decision: P1-C remains FAILED/INCOMPLETE. Uniform JSGT evidence survived; nonuniform admissibility did not. No production files, commits, or tags were changed.**
