# P1-PF results — PASS

**42 frozen-geometry projection cases, five detected negative controls, and six actual compiled-transport adapters passed.** This is a component gate. No body poses were advanced and P1-D/E remains unimplemented.

## Reproduce

```sh
bash prototypes/fluid_coupling/r1_p1/run_p1pf.sh
```

Requires CMake, a C++17 compiler, LP64 LAPACK, Bash and Python 3 standard library. See `P1PF_METHOD.md` for exact equations and dependency notices. No Python numerical package is called by the compiled projection or this verification script.

## Executed counts and preserved checkpoint

- Reference component fixtures: **42**; negative controls: **5**.
- Component idempotence calls: **6**; total component factorization calls including negative controls: **51**.
- Actual C++ transport adapters: **6**, each after **12** timesteps; **6** primary projections plus **6** separate idempotence calls.
- Original transport gate: **55 runs / 1,655 timesteps**, unchanged numerical summaries. Original fraction stage/step/oracle records and four momentum/controls records are byte-identical to the pre-edit rerun.
- No specified fixture was skipped. The 72 adapter transport steps are additional to the preserved 1,655, not additional body timesteps.

## Worst normalized component residuals

| Measure | Worst | Gate |
|---|---:|---:|
| constraint_residual | 1.398000e-13 | 1e-9 |
| impulse_residual | 3.185352e-14 | 1e-9 |
| energy_identity_residual | 1.100628e-14 | 1e-9 |
| momentum_budget_residual | 2.198394e-13 | 1e-9 |
| mass_partition_error | 6.082246e-14 | 1e-9 |

## Physical comparisons

| Case / quantity | Actual | Expected | Absolute error | Gate |
|---|---:|---:|---:|---:|
| sharp_density_jump_1.0 / hydro_pressure_relative_error | 1.15258893449e-14 | 0 | 1.153e-14 | 1e-09 |
| sharp_density_jump_1000.0 / hydro_pressure_relative_error | 6.86997445329e-15 | 0 | 6.870e-15 | 1e-09 |
| sharp_density_jump_1000000.0 / hydro_pressure_relative_error | 1.24279774296e-14 | 0 | 1.243e-14 | 1e-09 |
| submerged_free_density_ratio_1 / neutral_body_speed | 5.22335298345e-16 | 0 | 5.223e-16 | 1e-09 |
| asymmetric_partial_immersion_hydrostatic_torque / hydro_force_y | 340.76125 | 340.76125 | 1.393e-11 | 1e-07 |
| asymmetric_partial_immersion_hydrostatic_torque / hydro_torque | 9.72241071428 | 9.72241071429 | 9.344e-13 | 1e-07 |
| supported_resolved_cup_load / fluid_force_y | -815.45625 | -815.45625 | 0.000e+00 | 1e-07 |
| supported_resolved_cup_load / support_force_y | 1428.58125 | 1428.58125 | 2.274e-13 | 1e-07 |
| open_piston_mass_0.01 / piston_velocity | 0.00239754491395 | 0.00239754491401 | 6.100e-14 | 1e-09 |
| open_piston_mass_0.1 / piston_velocity | 0.00239702768566 | 0.00239702768567 | 7.801e-15 | 1e-09 |
| open_piston_mass_1 / piston_velocity | 0.00239186764999 | 0.00239186764999 | 8.127e-16 | 1e-09 |
| open_piston_mass_80 / piston_velocity | 0.00201173512154 | 0.00201173512154 | 7.373e-18 | 1e-09 |
| open_piston_mass_1000 / piston_velocity | 0.000705674801529 | 0.000705674801529 | 6.505e-19 | 1e-09 |
| open_piston_mass_1e+06 / piston_velocity | 9.99583090553e-07 | 9.99583090553e-07 | 2.118e-22 | 1e-09 |
| sealed_piston_pressure_difference / pressure_difference | 100 | 100 | 1.990e-13 | 1e-08 |
| constant_ambient_pressure / constant_pressure_error | 2.56932253251e-11 | 0 | 2.569e-11 | 1e-07 |
| common_free_fall / free_fall_correction | 0 | 0 | 0.000e+00 | 1e-10 |
| arbitrary_gravity_[3.1, -9.81] / pressure_force_0 | -86.1111111111 | -86.1111111111 | 2.288e-12 | 1e-07 |
| arbitrary_gravity_[3.1, -9.81] / pressure_force_1 | 272.5 | 272.5 | 1.683e-11 | 1e-07 |
| arbitrary_gravity_[-7.0, 2.0] / pressure_force_0 | 194.444444444 | 194.444444444 | 6.963e-12 | 1e-07 |
| arbitrary_gravity_[-7.0, 2.0] / pressure_force_1 | -55.5555555556 | -55.5555555556 | 2.557e-11 | 1e-07 |
| arbitrary_gravity_[9.81, 0.0] / pressure_force_0 | -272.5 | -272.5 | 2.728e-12 | 1e-07 |
| arbitrary_gravity_[9.81, 0.0] / pressure_force_1 | -2.68300937023e-11 | -0 | 2.683e-11 | 1e-07 |

Supported-body component force records for all five densities, all physical oracle checks, free-body linear/angular responses, per-cell pressures/divergences and per-interface normal mismatches are in the CSV files. The piston oracle uses whole-system analytical inertia; the solver contains no such formula.

## Smooth pressure refinement

| MAC resolution | Pressure L1 (Pa) | Ratio to next grid |
|---|---:|---:|
| 6 | 2.38750899481 | 4.093907 |
| 12 | 0.583185966934 | 4.023217 |
| 24 | 0.144955124658 | — |

Both ratios satisfy the unchanged 3.7–4.4 gate. This smooth, solid-free pressure result does not establish second-order momentum transport or cut-cell accuracy.

## Case inventory and conditioning

| Case | Matrix rows × columns | Rank / nullity | H min / max | Whitened condition | Constraint residual | Solve seconds |
|---|---:|---:|---:|---:|---:|---:|
| random_fluid_1_openFalse | 96 × 144 | 95 / 1 | 0.007812 / 0.01562 | 9.1126 | 2.349e-15 | 0.00487493 |
| random_fluid_1_openTrue | 88 × 144 | 88 / 0 | 0.007812 / 0.01562 | 16.942 | 2.518e-15 | 0.00428879 |
| random_fluid_1000_openFalse | 96 × 144 | 95 / 1 | 0.8053 / 14.28 | 9.8612 | 2.648e-15 | 0.00649581 |
| random_fluid_1000_openTrue | 88 × 144 | 88 / 0 | 0.8053 / 14.28 | 18.01 | 3.800e-15 | 0.00485166 |
| random_fluid_1e+06_openFalse | 96 × 144 | 95 / 1 | 798.3 / 1.428e+04 | 9.8681 | 1.956e-15 | 0.00683285 |
| random_fluid_1e+06_openTrue | 88 × 144 | 88 / 0 | 798.3 / 1.428e+04 | 18.017 | 3.092e-15 | 0.00381787 |
| sharp_density_jump_1.0 | 180 × 312 | 180 / 0 | 0.003472 / 0.006944 | 24.2 | 5.716e-16 | 0.0265508 |
| sharp_density_jump_1000.0 | 180 × 312 | 180 / 0 | 0.003472 / 6.944 | 12.773 | 4.175e-15 | 0.0256357 |
| sharp_density_jump_1000000.0 | 180 × 312 | 180 / 0 | 0.003472 / 6944 | 12.733 | 1.398e-13 | 0.0284152 |
| submerged_free_density_ratio_0.001 | 184 × 311 | 184 / 0 | 0.0001286 / 6.944 | 43.528 | 5.709e-15 | 0.0307507 |
| submerged_supported_density_ratio_0.001 | 187 × 311 | 187 / 0 | 0.0001286 / 6.944 | 93.349 | 1.737e-14 | 0.0301597 |
| submerged_free_density_ratio_0.5 | 184 × 311 | 184 / 0 | 0.003472 / 13.89 | 12.593 | 4.595e-15 | 0.0288578 |
| submerged_supported_density_ratio_0.5 | 187 × 311 | 187 / 0 | 0.003472 / 13.89 | 15.128 | 4.836e-15 | 0.031351 |
| submerged_free_density_ratio_1 | 184 × 311 | 184 / 0 | 0.003472 / 27.78 | 12.865 | 4.836e-15 | 0.0294128 |
| submerged_supported_density_ratio_1 | 187 × 311 | 187 / 0 | 0.003472 / 27.78 | 14.149 | 5.355e-15 | 0.0293805 |
| submerged_free_density_ratio_2 | 184 × 311 | 184 / 0 | 0.003472 / 55.56 | 13.188 | 5.017e-15 | 0.0277054 |
| submerged_supported_density_ratio_2 | 187 × 311 | 187 / 0 | 0.003472 / 55.56 | 14.022 | 5.808e-15 | 0.0304427 |
| submerged_free_density_ratio_1000 | 184 × 311 | 184 / 0 | 0.003472 / 2.778e+04 | 13.998 | 4.740e-15 | 0.0289007 |
| submerged_supported_density_ratio_1000 | 187 × 311 | 187 / 0 | 0.003472 / 2.778e+04 | 14.001 | 5.987e-15 | 0.0293486 |
| asymmetric_partial_immersion_hydrostatic_torque | 192 × 309 | 192 / 0 | 0.003472 / 38.89 | 17.228 | 6.142e-15 | 0.037753 |
| supported_resolved_cup_load | 203 × 298 | 203 / 0 | 0.003472 / 62.5 | 30.502 | 9.105e-15 | 0.0432459 |
| open_piston_mass_0.01 | 94 × 145 | 94 / 0 | 0.0008565 / 13.89 | 468.12 | 4.468e-15 | 0.00567811 |
| open_piston_mass_0.1 | 94 × 145 | 94 / 0 | 0.006944 / 13.89 | 126.08 | 2.507e-15 | 0.00482243 |
| open_piston_mass_1 | 94 × 145 | 94 / 0 | 0.006944 / 13.89 | 37.684 | 3.384e-15 | 0.00568409 |
| open_piston_mass_80 | 94 × 145 | 94 / 0 | 0.006944 / 80 | 9.2195 | 2.643e-16 | 0.00459297 |
| open_piston_mass_1000 | 94 × 145 | 94 / 0 | 0.006944 / 1000 | 9.0013 | 3.283e-17 | 0.00492941 |
| open_piston_mass_1e+06 | 94 × 145 | 94 / 0 | 0.006944 / 1e+06 | 9.0013 | 1.818e-20 | 0.00386934 |
| sealed_piston_pressure_difference | 106 × 145 | 105 / 1 | 6.852 / 80 | 13.269 | 2.113e-16 | 0.00673216 |
| constant_ambient_pressure | 148 × 311 | 148 / 0 | 0.09002 / 19.44 | 7.7698 | 7.503e-16 | 0.0175843 |
| common_free_fall | 196 × 311 | 195 / 1 | 0.003472 / 19.44 | 12.721 | 0.000e+00 | 0.0419625 |
| covariance_reference | 196 × 311 | 195 / 1 | 0.09002 / 19.44 | 14.652 | 3.818e-15 | 0.045117 |
| covariance_angle0.731_origin0 | 196 × 311 | 195 / 1 | 0.09002 / 19.44 | 14.652 | 3.660e-15 | 0.0425491 |
| covariance_angle0.731_origin1e+09 | 196 × 311 | 195 / 1 | 0.09002 / 19.44 | 14.652 | 3.660e-15 | 0.0420446 |
| galilean_covariance | 196 × 311 | 195 / 1 | 0.09002 / 19.44 | 14.652 | 3.383e-15 | 0.0448766 |
| normal_only_free_slip | 64 × 110 | 64 / 0 | 10.42 / 20.83 | 6.9603 | 0.000e+00 | 0.00170595 |
| two_bodies_mixed_mass | 148 × 218 | 147 / 1 | 0.0002667 / 400 | 34.521 | 4.345e-15 | 0.0158577 |
| arbitrary_gravity_[3.1, -9.81] | 148 × 311 | 148 / 0 | 0.1286 / 27.78 | 7.769 | 3.461e-15 | 0.0176237 |
| arbitrary_gravity_[-7.0, 2.0] | 148 × 311 | 148 / 0 | 0.1286 / 27.78 | 7.769 | 3.494e-15 | 0.0174867 |
| arbitrary_gravity_[9.81, 0.0] | 148 × 311 | 148 / 0 | 0.1286 / 27.78 | 7.769 | 3.306e-15 | 0.0178366 |
| smooth_pressure_refinement_6 | 36 × 84 | 36 / 0 | 13.89 / 27.78 | 3.7787 | 2.289e-17 | 0.000429429 |
| smooth_pressure_refinement_12 | 144 × 312 | 144 / 0 | 3.472 / 6.944 | 7.6069 | 6.052e-17 | 0.0151073 |
| smooth_pressure_refinement_24 | 576 × 1200 | 576 / 0 | 0.8681 / 1.736 | 15.26 | 1.008e-16 | 0.907166 |

Every rank threshold and singular extreme is recorded. The sealed piston has one coupled pressure gauge. Per-body mass/inertia and impulse mismatch are recorded separately. Global budget gates include external walls and supports.

## Six actual transport adapters

| Case | Constraint | Impulse | Energy | Momentum budget | Idempotence | Common-vector error | Time s |
|---|---:|---:|---:|---:|---:|---:|---:|
| common_comotion_ratio_1 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 2.735e-17 | 0.000e+00 | 5.551e-17 | 0.0505784 |
| nonuniform_transport_snapshot_ratio_1 | 3.544e-16 | 4.585e-17 | 3.123e-17 | 3.723e-18 | 4.996e-16 | 0.000e+00 | 0.0530326 |
| common_comotion_ratio_1000 | 3.142e-17 | 5.843e-16 | 1.057e-32 | 4.773e-17 | 5.551e-17 | 2.776e-16 | 0.0648049 |
| nonuniform_transport_snapshot_ratio_1000 | 5.596e-15 | 3.759e-15 | 8.695e-16 | 8.314e-17 | 9.548e-15 | 0.000e+00 | 0.0591091 |
| common_comotion_ratio_1000000 | 3.704e-17 | 6.021e-13 | 1.112e-31 | 9.679e-18 | 5.551e-17 | 7.208e-14 | 0.0579603 |
| nonuniform_transport_snapshot_ratio_1000000 | 1.372e-13 | 3.162e-15 | 5.908e-16 | 1.988e-17 | 2.407e-13 | 0.000e+00 | 0.0582982 |

All six preserve fractions, fine/dual masses, input momentum snapshots and prescribed advectors byte-for-byte through projection. Output momentum is returned separately. Every periodic seam has one shared DOF. Nonuniform cases are snapshots of prescribed transport, not a validated Navier–Stokes time integrator.

## Negative controls and rejected inputs

| Deliberately wrong case | Error / detection measure | Required detection | Result |
|---|---:|---:|---|
| lagged_partitioned_piston | 4170733.33573 | > 1 | DETECTED |
| halve_interface_half_dual_masses | 0.0437901920469 | > 0.001 | DETECTED |
| pin_each_disconnected_chamber | 0.000366668812549 | > 9.9999999999999995e-07 | DETECTED |
| incompatible_closed_wall_flux | 1 | > 0.5 | DETECTED |
| drop_rotational_reaction | 0.860215663136 | > 1.0000000000000001e-05 | DETECTED |

The incompatible sealed-wall input throws an explicit incompatibility error (its exact input and diagnostic are in `incompatible_input.txt`). Nine additional malformed-input checks reject zero/negative/nonfinite masses, nonfinite velocities/RHS/operator, a zero row, mismatched dimensions and zero body inertia. They are input-validation checks, not additional physical projection cases. No mean-RHS removal, pressure pin repair, mass floor or damping is used in the accepted solver. Wrong-mass, delayed-reaction, two-pin and missing-rotation calculations exist only in negative controls.

## Energy, extrema, timing and scope

Component velocities range from -3.02150114348 to 8.30715795428 m/s (angular components are rad/s). Cell pressures range from -8817727.48882 to 18517564.4324 Pa; values in sealed cases depend on the recorded SVD gauge. No pressure clamps were applied.

Adapter fraction extrema: [-1.8041124150158794e-16, 1.0000000000000016], within the unchanged 256-epsilon gate, without repair.

Kinetic energies before/after, constraint work, dissipation and raw energy residuals are in every projection summary. Homogeneous cases satisfy the non-increase gate. Moving prescribed walls may supply work. This proves only the tested fixed-H projection identity, not full-timestep energy conservation.

Measured elapsed times: component fixtures 1.915160 s; six adapters 0.343784 s; original transport fixtures 8.011390 s. Dense SVD plus heavy diagnostics is a correctness implementation, with no real-time performance claim.

## Source and evidence provenance

The supplied handoff and original archive remain unchanged. `prepare_p1pf_reference.py` records the independent Python reruns and exports seeded inputs only; C++ answers come from LAPACK and the reusable assembled operator. See `evidence/p1pf/reference_rerun/` for exact versions, input hashes and cross-platform differences.

Changes: prototype-only `frozen_projection.hpp`, `p1pf_validation.hpp`, `p1pf_adapters.hpp`, optional target dispatch in `main.cpp`/`CMakeLists.txt`, runner/recorder, method/evidence docs, seeded inputs and dependency notices. `record_p1cm.py` changes only generated scope wording. Fraction, mass and momentum algorithms and original fixtures/oracles are unchanged.

Current matching source snapshots and SHA-256 fingerprints are in `evidence/p1pf/current/source_snapshot/` and `RUN_METADATA.json`. Baseline sources/evidence remain under `evidence/p1pf/baseline/`. Full matrix entries, row/DOF/body records and all analytical comparisons accompany the summaries.

Git HEAD remains `0d73f47206cc34674bb9fbbce2cb438f38ce9f22`. No commit, push or tag performed. All tracked changes are confined to the prototype. Captured status:

```text
 M prototypes/fluid_coupling/r1_p1/.gitignore
 M prototypes/fluid_coupling/r1_p1/CMakeLists.txt
 M prototypes/fluid_coupling/r1_p1/P1CM_RESULTS.md
 M prototypes/fluid_coupling/r1_p1/README.md
 M prototypes/fluid_coupling/r1_p1/STATUS.md
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/RUN_METADATA.json
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/jsgt_summary.csv
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/momentum_summary.csv
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/run_output.txt
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/source_snapshot/CMakeLists.txt
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/source_snapshot/main.cpp
 M prototypes/fluid_coupling/r1_p1/evidence/p1cm/current/source_snapshot/record_p1cm.py
 M prototypes/fluid_coupling/r1_p1/main.cpp
 M prototypes/fluid_coupling/r1_p1/record_p1cm.py
 M prototypes/fluid_coupling/r1_p1/results/jsgt_summary.csv
?? prototypes/fluid_coupling/r1_p1/P1PF_METHOD.md
?? prototypes/fluid_coupling/r1_p1/P1PF_RESULTS.md
?? prototypes/fluid_coupling/r1_p1/dependencies/
?? prototypes/fluid_coupling/r1_p1/evidence/p1pf/
?? prototypes/fluid_coupling/r1_p1/fixtures/
?? prototypes/fluid_coupling/r1_p1/frozen_projection.hpp
?? prototypes/fluid_coupling/r1_p1/p1pf_adapters.hpp
?? prototypes/fluid_coupling/r1_p1/p1pf_validation.hpp
?? prototypes/fluid_coupling/r1_p1/prepare_p1pf_reference.py
?? prototypes/fluid_coupling/r1_p1/record_p1pf.py
?? prototypes/fluid_coupling/r1_p1/run_p1pf.sh
```

**Stop for operator review.** Frozen projection does not earn moving-solid transport, integrated buoyancy, P1-D/E, swimming, 3D or full R1.
