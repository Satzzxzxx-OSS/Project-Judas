> Preserved P1-C fraction checkpoint: the active executable now also runs P1-C-M. See `P1CM_METHOD.md` and `P1CM_RESULTS.md`. Statements below about executable scope describe the earlier fraction-only checkpoint. Its full original source/report is preserved in `evidence/p1cm/baseline/`.

# P1-C frozen-WY JSGT results — PASS

**P1-C passes this transport validation family. Integrated P1 remains incomplete.** No moving solids, triple-line integration, pressure/body coupling, P1-D/E, P2 or production integration was resumed.

## Implementation and scope

`main.cpp` freezes Cn>0.5 once, uses that field in both directional predictors and both complete diagnostic WY orders, averages separately reconstructed geometric face fluxes, and accepts only the shared-face final divergence. The cumulative Courant gate is strict <0.5. No averaged-scalar PLIC, limiter, fraction clamp or state repair was introduced. Tolerances and all original fixture geometries are unchanged.

Added `nonlinear_transport_oracle.hpp` supplies an independent exact nonlinear flow-map oracle. Existing `transport_oracles.hpp` and `vof.hpp` were not changed. Documentation and raw evidence were updated; the failed continuous-C predecessor was archived.

## Refinement

L1 is the domain integral of absolute cell-fraction error against independent analytical geometry. Observed order is log2(previous_error/current_error). Runtime includes reconstruction, all diagnostics, oracle evaluation and CSV output.

| Fixture | Grid | dt | Cx+Cy | L1 | Order | Cumulative budget max | Runtime s |
|---|---:|---:|---:|---:|---:|---:|---:|
| aligned_translation | 24² | 0.0416666667 | 0.28 | 0.001765346104 | — | 4.163e-17 | 0.00691972 |
| aligned_translation | 48² | 0.025 | 0.336 | 0.001006945559 | 0.810 | 4.163e-17 | 0.0299375 |
| aligned_translation | 96² | 0.0125 | 0.336 | 0.0004320017886 | 1.221 | 6.939e-17 | 0.171969 |
| oblique_translation | 24² | 0.1 | 0.288 | 0.001468846322 | — | 2.776e-17 | 0.00341048 |
| oblique_translation | 48² | 0.05 | 0.288 | 0.0006319724915 | 1.217 | 5.551e-17 | 0.0172054 |
| oblique_translation | 96² | 0.025 | 0.288 | 0.0002616053906 | 1.272 | 5.551e-17 | 0.0947148 |
| slotted_translation | 24² | 0.05 | 0.264 | 0.003142326031 | — | 5.551e-17 | 0.00897267 |
| slotted_translation | 48² | 0.0333333333 | 0.352 | 0.00121675153 | 1.369 | 1.110e-16 | 0.0337363 |
| slotted_translation | 96² | 0.0166666667 | 0.352 | 0.0007051958306 | 0.787 | 2.776e-16 | 0.192834 |
| reversal | 24² | 0.1 | 0.432 | 0.002145035265 | — | 2.082e-17 | 0.00285067 |
| reversal | 48² | 0.0333333333 | 0.288 | 0.001096501928 | 0.968 | 2.776e-17 | 0.0223981 |
| reversal | 96² | 0.0166666667 | 0.288 | 0.0004551130567 | 1.269 | 8.327e-17 | 0.131948 |
| nonuniform_strain | 24² | 0.015625 | 0.375 | 0.002214210765 | — | 1.665e-16 | 0.0117665 |
| nonuniform_strain | 48² | 0.0078125 | 0.375 | 0.0008297001348 | 1.416 | 6.106e-16 | 0.0568177 |
| nonuniform_strain | 96² | 0.00390625 | 0.375 | 0.0003920667493 | 1.081 | 1.221e-15 | 0.273867 |
| nonlinear_incompressible | 24² | 0.0208333333 | 0.364583 | 0.002137304429 | — | 1.388e-16 | 0.0399632 |
| nonlinear_incompressible | 48² | 0.0104166667 | 0.369792 | 0.001099365166 | 0.959 | 4.996e-16 | 0.177039 |
| nonlinear_incompressible | 96² | 0.00520833333 | 0.372396 | 0.0005398825274 | 1.026 | 6.106e-16 | 0.962983 |

All six error sequences decrease at both refinements. The nonlinear fixture is approximately first order here. These are empirical convergence results, not a general accuracy theorem. Uniform errors are not all smaller than the prior report: the newly required smaller dt adds reconstruction steps and changes geometric diffusion. No constants were tuned to recover the previous numbers.

## Bounds and identity

| Stage | Minimum fraction | Maximum fraction | Beyond-tolerance cells |
|---|---:|---:|---:|
| predictor_x | -3.0531133177191805e-16 | 1.0000000000000255 | 0 |
| predictor_y | -2.4980018054066022e-16 | 1.0000000000000255 | 0 |
| wy_x_after_y | -2.9143354396410359e-16 | 1.0000000000000255 | 0 |
| wy_y_after_x | -3.1225022567582528e-16 | 1.0000000000000255 | 0 |
| jsgt_final | -2.3939183968479938e-16 | 1.0000000000000258 | 0 |

All 25 runs, 1190 timesteps and 5950 recorded stages completed. The unchanged alpha tolerance is 5.6843418860808015e-14. The largest overshoot, 2.5757e-14, occurs in the complete rotation; it is recorded, not repaired. This is accumulated roundoff within the existing gate, not exact mathematical boundedness of floating-point values.

- Maximum raw JSGT/sweep-mean error: 2.2204460492503131e-16.
- Maximum divergence-adjusted identity error: 2.688821387764051e-16.
- Maximum per-stage budget residual: 1.7446981359636737e-15.
- Maximum cumulative accepted-state volume budget residual: 2.2343238370581275e-15.
- Maximum cumulative Courant number: 0.43200000000000005 (<0.5).
- Maximum total MAC divergence residual: 1.5987211554602254e-14 per unit time.

Pure gas/liquid remain bit-exact under both uniform velocity (24²) and the existing dyadic affine strain check (32²). Both full WY orders use the original frozen coefficient; neither feeds the accepted state.

## Original witness: new arithmetic

The original 24² strain geometry is unchanged. The sufficient gate reduced dt from 1/32 to 1/64. Both directional Courant numbers are 0.1875; their sum is 0.375. Therefore the new donor strip is shorter; claiming that this new step still empties the witness would be incorrect.

For cell (6,18), step 0:

| Quantity | Value |
|---|---:|
| Cn | 1/8 = 0.125 |
| Frozen coefficient | 0 |
| Cell area | 1/576 |
| Bottom signed phase-area flux | -0.00015597873263888888 = -23/147456 |
| Top phase-area flux | 0 |
| Normalized outward phase flux | 23/256 |
| Conservative part | 1/8 - 23/256 = 9/256 |
| Directional divergence contribution | approximately -1/64 |
| Frozen correction | 0*(-1/64) = 0 |
| Y predictor | **9/256 = 0.03515625** |
| X predictor | 0.12835693359375 |

The adjacent horizontal-interface cell (7,18) has bottom flux -0.00016276041666666666, conservative remainder 1/32, correction zero, and Y=0.03125. Both are genuine geometric results. No witness-cell branch affects transport. The original dt=1/32 would give binary-corrected Y=0 by the previously derived arithmetic, but was not used as acceptance evidence because it violates the global sufficient timestep gate.

## Nonlinear hostile field

The additional field is u=(x-0.5)^2, v=-2(x-0.5)(y-0.5). Directional divergences are nonzero and spatially varying; MAC samples are exact face averages. The independent area-preserving flow map transforms an initial rectangle into curved interfaces. Its closed-form cell-area oracle uses no prototype PLIC, clipping or flux code. At t=0.5 the x support is [5/18,11/14] and total area is exactly 0.2.

Across independently checked oracle snapshots t=0,0.25,0.5 at 24/48/96, maximum summed-area error is 3.334e-18 and fractions stay in [0,1]. The t=0 identity checks initialization only. Nonzero-time map derivation, integration and summed-area checks provide the independent evidence.

## Full rotation and boundary export

- **slotted_rotation**, 32²: 720 steps, dt=0.001388888889, cumulative Courant=0.270526034, L1=0.005952009721, runtime=2.8471 s; boundary export=0, retained area=0.11816370615017721.
- **boundary_exit_translation**, 48²: 48 steps, dt=0.01041666667, cumulative Courant=0.2, L1=0.0003878445322, runtime=0.064365 s; boundary export=0.030112209916657806, retained area=0.0098877900833422017.
- **boundary_exit_slab**, 48²: 48 steps, dt=0.01041666667, cumulative Courant=0.2, L1=6.823245672e-17, runtime=0.11835 s; boundary export=0.15000000000000005, retained area=0.049999999999999899.

The finite-height outflow rectangle has reconstruction error: exact export 0.03 versus measured 0.030112209916657806. Its shared-face budget still closes. The separate full-height slab removes that geometric ambiguity and explicitly asserts exact retained/exported areas 0.05/0.15. Measured errors are 1.041e-16 and 5.551e-17 respectively. The boundary conservation check is therefore exercised with nonzero export, not only closed liquid support.

## Limitations and result boundary

- Centered-gradient PLIC normal estimation and the existing axis-biased degenerate-normal fallback remain unchanged.
- Roundoff-outside fractions retain their stored values; the existing PLIC helper uses limiting empty/full geometry only within the unchanged representability tolerance.
- Pure-phase constancy is tested; no arbitrary fractional-tracer constancy claim is made.
- Smaller dt can increase reconstruction diffusion; no energy/momentum conservation claim follows from prescribed-velocity VOF transport.
- No pressure, body force, buoyancy, body mass threshold, or coupling is present in the active path.

Total measured fixture runtime: **5.279387 s**. This includes heavy diagnostic/oracle work and is not a production benchmark.

## Evidence and reproduction

- `results/jsgt_stdout.txt`: complete output and exact witness arithmetic.
- `results/jsgt_steps.csv`: per-timestep CFL, divergence, bounds, volume/boundary budgets and identity.
- `results/jsgt_trace.csv`: all actual predictor/order/final stages, including strict excursions.
- `results/jsgt_summary.csv`: per-fixture metrics.
- `results/jsgt_nonlinear_oracle.csv`: independent nonlinear oracle area/bounds audit.
- `results/jsgt_run_metadata.json`: source/evidence fingerprints, compiler, reference HEAD and result.
- `results/continuous_predictor_archive/`: failed predecessor source and evidence.

**Decision: P1-C PASS for frozen-WY JSGT. Integrated P1 remains incomplete. Work stops here before moving-solid/pressure/body integration. No production modifications, commits or tags.**
