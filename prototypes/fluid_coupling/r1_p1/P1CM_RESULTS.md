# P1-C-M results — PASS

**The compiled Judas prototype passes the specified fixed-grid mass-consistent momentum checks. This does not complete integrated P1.** Pressure, moving solids, cut cells, P1-D/E, buoyancy, swimming and production integration were not run.

## Reproduce

```sh
bash /home/conner/Documents/GitHub/Project-Judas/prototypes/fluid_coupling/r1_p1/run_p1cm.sh
```

The command configures/builds the existing prototype, executes its single compiled implementation, verifies regression/evidence gates, and records sources/hashes. No external Python packages are required for this C++ validation.

## Executed scope and checkpoint preservation

- Before editing: all 25 accepted P1-C runs reran identically except timings; sources and results were copied and fingerprinted in `evidence/p1cm/baseline/`.
- Final execution: **55 runs / 1,655 timesteps**: 25 existing fraction cases with momentum active (1,190 steps), plus 30 momentum cases (465 steps). No requested test family was skipped.
- Every prior fraction summary field except timing is identical. Full per-stage, per-step and nonlinear-oracle CSV files are byte-identical to the pre-edit rerun.
- One actual JSGT phase-flux path supplies all mass/momentum stages. There is no alternate PLIC or phase flux path for momentum.

## Mechanism and changed files

- `main.cpp`: opt-in periodic geometry indexing, actual phase-transfer/state records, momentum attached to all legacy runs, new validation runner in the same executable. No original fixture, timestep policy, normal estimate or fraction arithmetic was retuned.
- New `mass_momentum.hpp`: fine mass/flux/source records, separate staggered partitions, half-duals at physical boundaries, segment-first upwinding, frozen source velocity, both branch updates and conserved combination.
- New `momentum_validation.hpp`: analytical expectations, fixtures, covariance and negative controls; calls the shared implementation.
- New `run_p1cm.sh`, `record_p1cm.py`: reproducible execution and evidence verification.
- New `P1CM_METHOD.md`, `P1CM_RESULTS.md`; updated README/STATUS and scope notices in the three preserved P1-C documents.
- Added evidence under `evidence/p1cm/`, including the unchanged supplied package, reference rerun, preserved baseline, logs, CSVs and snapshots. The build remains `.build-jsgt/`.
- `vof.hpp`, original oracle headers and CMake configuration are unchanged.

## Numerical gate results

| Quantity | Worst measured normalized error | Gate |
|---|---:|---:|
| Fine/dual/combination mass consistency | 6.804545e-16 | 1e-12 |
| Global mass, including boundary transfer | 5.250780e-14 | 1e-11 |
| Component momentum, including boundary transfer | 1.633725e-14 | 1e-11 |
| Constant transported vector, raw | 7.541745e-12 | 1e-8 * Uref |
| Combination energy inequality | 9.666844e-14 | 1e-12 |

Raw fractions over all compiled stages: **[-3.1918911957973251e-16, 1.0000000000000258]**, zero material excursions under the unchanged 256-epsilon tolerance. Stored fractions were not repaired. The existing roundoff endpoint geometry convention remains explicit.

All masses stayed positive and states finite. Local scales use actual dual volumes, masses, absolute boundary transfers and sources. Raw and normalized values are logged. Core global budgets do not subtract the source residual; a second long-double audit separately reports source-adjusted budgets. Source cancellation is checked, not assumed.

## Momentum fixtures

The reference-family random generator is seeded xorshift64*, not NumPy. Its pure/mixed geometry and test categories reproduce the supplied family; invariants are compared, not PLIC/RNG bit patterns. Constant vectors in nonuniform prescribed flow are manufactured transport checks. The dedicated common-co-motion family explicitly tests transported velocity equal to the advector; the open-slab case does likewise, and the uniform reference smooth case degenerates to a constant matching vector.

| Case | Fine/MAC | Density ratio | Steps | Local mass max norm | Global mass max norm | Global momentum max norm | Common-vector error | Runtime s | Result |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| reference_uniform_constant | 24/12 | 1 | 12 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.0401697 | PASS |
| reference_uniform_smooth | 24/12 | 1 | 12 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.038168 | PASS |
| reference_uniform_seeded_random | 24/12 | 1 | 12 | 0.000e+00 | 0.000e+00 | 8.327e-17 | 0.000e+00 | 0.0380342 | PASS |
| reference_uniform_constant | 24/12 | 1000 | 12 | 4.830e-16 | 1.471e-15 | 1.022e-15 | 2.887e-15 | 0.0382468 | PASS |
| reference_uniform_smooth | 24/12 | 1000 | 12 | 4.830e-16 | 1.471e-15 | 1.022e-15 | 0.000e+00 | 0.0381669 | PASS |
| reference_uniform_seeded_random | 24/12 | 1000 | 12 | 4.830e-16 | 1.471e-15 | 2.686e-16 | 0.000e+00 | 0.038901 | PASS |
| reference_uniform_constant | 24/12 | 1e+06 | 12 | 4.547e-16 | 7.591e-16 | 6.025e-16 | 3.676e-12 | 0.0385376 | PASS |
| reference_uniform_smooth | 24/12 | 1e+06 | 12 | 4.547e-16 | 7.591e-16 | 8.549e-16 | 0.000e+00 | 0.0389818 | PASS |
| reference_uniform_seeded_random | 24/12 | 1e+06 | 12 | 4.547e-16 | 7.591e-16 | 3.467e-16 | 0.000e+00 | 0.0388415 | PASS |
| reference_streamfunction_constant | 24/12 | 1 | 12 | 1.249e-16 | 1.464e-18 | 2.656e-18 | 1.110e-16 | 0.0485218 | PASS |
| reference_streamfunction_smooth | 24/12 | 1 | 12 | 1.249e-16 | 1.464e-18 | 9.866e-18 | 0.000e+00 | 0.0493494 | PASS |
| reference_streamfunction_seeded_random | 24/12 | 1 | 12 | 1.249e-16 | 1.464e-18 | 6.939e-17 | 0.000e+00 | 0.048418 | PASS |
| reference_streamfunction_constant | 24/12 | 1000 | 12 | 4.429e-16 | 2.207e-15 | 5.039e-16 | 3.109e-15 | 0.0466367 | PASS |
| reference_streamfunction_smooth | 24/12 | 1000 | 12 | 4.429e-16 | 2.207e-15 | 1.549e-16 | 0.000e+00 | 0.0472504 | PASS |
| reference_streamfunction_seeded_random | 24/12 | 1000 | 12 | 4.429e-16 | 2.207e-15 | 3.358e-16 | 0.000e+00 | 0.0454993 | PASS |
| reference_streamfunction_constant | 24/12 | 1e+06 | 12 | 6.173e-16 | 1.265e-15 | 5.200e-16 | 7.542e-12 | 0.0514541 | PASS |
| reference_streamfunction_smooth | 24/12 | 1e+06 | 12 | 6.173e-16 | 1.265e-15 | 2.396e-16 | 0.000e+00 | 0.0463713 | PASS |
| reference_streamfunction_seeded_random | 24/12 | 1e+06 | 12 | 6.173e-16 | 1.265e-15 | 4.160e-16 | 0.000e+00 | 0.0531312 | PASS |
| genuine_common_comotion | 24/12 | 1 | 12 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 5.551e-17 | 0.0159391 | PASS |
| genuine_common_comotion | 24/12 | 1000 | 12 | 4.217e-16 | 1.251e-15 | 9.305e-16 | 5.829e-15 | 0.0160966 | PASS |
| genuine_common_comotion | 24/12 | 1e+06 | 12 | 6.805e-16 | 1.455e-15 | 1.092e-15 | 5.882e-12 | 0.0162457 | PASS |
| smooth_momentum_refinement | 24/12 | 1 | 18 | 0.000e+00 | 0.000e+00 | 6.510e-16 | 0.000e+00 | 0.0251107 | PASS |
| smooth_momentum_refinement | 48/24 | 1 | 35 | 0.000e+00 | 0.000e+00 | 1.404e-15 | 0.000e+00 | 0.12235 | PASS |
| smooth_momentum_refinement | 96/48 | 1 | 70 | 0.000e+00 | 0.000e+00 | 3.774e-15 | 0.000e+00 | 0.713327 | PASS |
| open_boundary_slab | 48/24 | 1 | 18 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.000e+00 | 0.0532633 | PASS |
| open_boundary_slab | 48/24 | 1000 | 18 | 1.826e-16 | 5.165e-14 | 1.054e-15 | 3.608e-15 | 0.0520482 | PASS |
| open_boundary_slab | 48/24 | 1e+06 | 18 | 1.799e-16 | 1.981e-14 | 7.662e-15 | 4.102e-12 | 0.0476307 | PASS |
| covariance_base | 24/12 | 1000 | 12 | 4.339e-16 | 1.308e-15 | 8.665e-16 | 0.000e+00 | 0.0176042 | PASS |
| covariance_axis_exchange | 24/12 | 1000 | 12 | 4.512e-16 | 1.308e-15 | 1.950e-15 | 0.000e+00 | 0.0185806 | PASS |
| covariance_x_reflection | 24/12 | 1000 | 12 | 4.344e-16 | 1.090e-15 | 8.665e-16 | 0.000e+00 | 0.0203492 | PASS |

## Momentum refinement

Exact sine/cosine averages over actual dual rectangles initialize and evaluate the translated solution. rho_l=rho_g=1, advector=(0.37,-0.21), final time=0.5.

| Fine/MAC | dt | L1 x | Order x | L1 y | Order y |
|---|---:|---:|---:|---:|---:|
| 24/12 | 0.0277777778 | 0.03010827302 | — | 0.02240727706 | — |
| 48/24 | 0.0142857143 | 0.0161869557 | 0.895 | 0.01197845145 | 0.904 |
| 96/48 | 0.00714285714 | 0.008369630422 | 0.952 | 0.006196249925 | 0.951 |

Both component errors decrease, approaching first-order behavior. No second-order claim is made.

## Open boundaries, controls and covariance

The open slab uses periodic y and physical x boundaries. It explicitly checks retained liquid 0.05 and net exports 0.15*(rho_l-rho_g) in mass and 0.3 times that amount in x-momentum. Gas inflow is included; physical boundary half-duals contain no ghost mass.

| Check | Actual | Expected / diagnostic | Status |
|---|---:|---:|---|
| counterflow: momentum | 0.75 | 0.75 (diagnostic 0) | PASS |
| negative_net_first: momentum | 0 | 0.75 (diagnostic 0.75) | DETECTED |
| unequal_branch_mass: velocity | 0.1 | 0.1 (diagnostic 0.5) | PASS |
| frozen_source_cancellation: momentum_source | 0 | 0 (diagnostic 0.0825) | PASS |
| open_boundary_slab_1: retained_liquid | 0.05 | 0.05 (diagnostic 1.249e-16) | PASS |
| open_boundary_slab_1: net_mass_export | 0 | 0 (diagnostic 0) | PASS |
| open_boundary_slab_1: net_px_export | 0 | 0 (diagnostic 0) | PASS |
| open_boundary_slab_1000: retained_liquid | 0.05 | 0.05 (diagnostic 1.249e-16) | PASS |
| open_boundary_slab_1000: net_mass_export | 149.85 | 149.84999999999999 (diagnostic 1.132e-15) | PASS |
| open_boundary_slab_1000: net_px_export | 44.955 | 44.954999999999998 (diagnostic 1.769e-16) | PASS |
| open_boundary_slab_1000000: retained_liquid | 0.05 | 0.05 (diagnostic 1.249e-16) | PASS |
| open_boundary_slab_1000000: net_mass_export | 149999.85 | 149999.85000000001 (diagnostic 7.276e-16) | PASS |
| open_boundary_slab_1000000: net_px_export | 44999.955 | 44999.955000000002 (diagnostic 2.183e-16) | PASS |
| covariance_axis_exchange: fraction_covariance | 4.09394740331e-16 | 0 (diagnostic 5.684e-14) | PASS |
| covariance_axis_exchange: velocity_covariance | 9.21485110439e-15 | 0 (diagnostic 1.023e-08) | PASS |
| covariance_x_reflection: fraction_covariance | 5.55111512313e-16 | 0 (diagnostic 5.684e-14) | PASS |
| covariance_x_reflection: velocity_covariance | 4.77395900589e-15 | 0 (diagnostic 1.023e-08) | PASS |
| negative_old_mass: actual_transport_error | 114665.593724 | 0 (diagnostic 1e-08) | DETECTED |
| negative_velocity_mean: actual_transport_error | 0.613406538703 | 0 (diagnostic 1e-08) | DETECTED |

The counterflow primitive is the same segment donor function used in active transport. The wrong-old-mass and independent-velocity-average diagnostics use actual branch states but never feed the accepted result. Source-only algebra is an auxiliary negative control; source cancellation is also checked on actual nonuniform runs.

The maximum prolongation fine-minus-parent divergence defect is 1.110223e-15. Existing P1-C velocities were never prolonged.

## Energy and velocity diagnostics

Energy is sum_q sum_K Pq^2/(2Mq), treating the two component partitions separately. Combination convexity is checked against the mean branch energy. Full-step energy is independently recorded; it is not inferred from that inequality. Donor-cell diffusion is expected.

| Case | Ratio | Initial E | Final E | Max step growth norm | Minimum component velocity | Maximum component velocity |
|---|---:|---:|---:|---:|---:|---:|
| reference_uniform_constant | 1 | 0.35465 | 0.35465 | 0.000e+00 | -0.42 | 0.73 |
| reference_uniform_smooth | 1 | 0.0905 | 0.0905 | 0.000e+00 | -0.21 | 0.37 |
| reference_uniform_seeded_random | 1 | 0.345756311 | 0.0363726912 | 0.000e+00 | -0.988244954 | 0.993867172 |
| reference_uniform_constant | 1000 | 41.1030756 | 41.1030756 | 8.643e-16 | -0.42 | 0.73 |
| reference_uniform_smooth | 1000 | 10.4887307 | 10.4887307 | 1.016e-15 | -0.21 | 0.37 |
| reference_uniform_seeded_random | 1000 | 40.903553 | 7.8973544 | 0.000e+00 | -0.984096064 | 0.993867172 |
| reference_uniform_constant | 1e+06 | 40789.5287 | 40789.5287 | 7.135e-16 | -0.42 | 0.73 |
| reference_uniform_smooth | 1e+06 | 10408.7194 | 10408.7194 | 5.243e-16 | -0.21 | 0.37 |
| reference_uniform_seeded_random | 1e+06 | 40598.7003 | 7885.02243 | 0.000e+00 | -0.984086534 | 0.993867172 |
| reference_streamfunction_constant | 1 | 0.35465 | 0.35465 | 0.000e+00 | -0.42 | 0.73 |
| reference_streamfunction_smooth | 1 | 0.0472662375 | 0.0370830644 | 0.000e+00 | -0.419246134 | 0.419246134 |
| reference_streamfunction_seeded_random | 1 | 0.345756311 | 0.0999958964 | 0.000e+00 | -0.992445896 | 0.991510299 |
| reference_streamfunction_constant | 1000 | 41.1030756 | 41.1030756 | 3.457e-16 | -0.42 | 0.73 |
| reference_streamfunction_smooth | 1000 | 6.26195983 | 5.55595195 | 0.000e+00 | -0.419998494 | 0.419997033 |
| reference_streamfunction_seeded_random | 1000 | 40.903553 | 16.0409411 | 0.000e+00 | -0.991802771 | 0.991510299 |
| reference_streamfunction_constant | 1e+06 | 40789.5287 | 40789.5287 | 3.568e-16 | -0.42 | 0.73 |
| reference_streamfunction_smooth | 1e+06 | 6220.95555 | 5525.91877 | 0.000e+00 | -0.419999998 | 0.419999997 |
| reference_streamfunction_seeded_random | 1e+06 | 40598.7003 | 15990.0813 | 0.000e+00 | -0.991802771 | 0.991510299 |
| genuine_common_comotion | 1 | 0.0905 | 0.0905 | 0.000e+00 | -0.21 | 0.37 |
| genuine_common_comotion | 1000 | 8.227355 | 8.227355 | 8.636e-16 | -0.21 | 0.37 |
| genuine_common_comotion | 1e+06 | 8145.08235 | 8145.08235 | 4.466e-16 | -0.21 | 0.37 |
| smooth_momentum_refinement | 1 | 0.372975527 | 0.365871124 | 0.000e+00 | -0.612670245 | 1.02321615 |
| smooth_momentum_refinement | 1 | 0.373293152 | 0.36921542 | 0.000e+00 | -0.618149722 | 1.02829092 |
| smooth_momentum_refinement | 1 | 0.373373242 | 0.371192895 | 0.000e+00 | -0.619536309 | 1.02957191 |
| open_boundary_slab | 1 | 0.045 | 0.045 | 0.000e+00 | 0 | 0.3 |
| open_boundary_slab | 1000 | 9.036 | 2.29275 | 0.000e+00 | 0 | 0.3 |
| open_boundary_slab | 1e+06 | 9000.036 | 2250.04275 | 0.000e+00 | 0 | 0.3 |
| covariance_base | 1000 | 56.2893958 | 55.8861711 | 0.000e+00 | -0.612670245 | 1.02321615 |
| covariance_axis_exchange | 1000 | 56.2893958 | 55.8861711 | 0.000e+00 | -0.612670245 | 1.02321615 |
| covariance_x_reflection | 1000 | 56.2893958 | 55.8861711 | 0.000e+00 | -1.02321615 | -0.227329755 |

No material full-step energy growth appeared in the closed cases; the small positive values are numerical roundoff. This is observed behavior for these fixtures, not a full transport or Navier-Stokes stability theorem. Per-component stage extrema, raw energy changes and budgets are available in `momentum_steps.csv`; legacy-case component extrema and energies are in `fraction_regression_momentum.csv`.

## Legacy fraction regression with momentum active

| Case | Fine grid | Steps | Fraction L1 | Runtime s | Result |
|---|---:|---:|---:|---:|---|
| constant_gas | 24 | 3 | 0 | 0.00103286 | PASS |
| constant_liquid | 24 | 3 | 0 | 0.00113556 | PASS |
| aligned_translation | 24 | 6 | 0.0017653461 | 0.00706219 | PASS |
| aligned_translation | 48 | 10 | 0.00100694556 | 0.0342749 | PASS |
| aligned_translation | 96 | 20 | 0.000432001789 | 0.215031 | PASS |
| oblique_translation | 24 | 2 | 0.00146884632 | 0.00388667 | PASS |
| oblique_translation | 48 | 4 | 0.000631972491 | 0.018823 | PASS |
| oblique_translation | 96 | 8 | 0.000261605391 | 0.113663 | PASS |
| slotted_translation | 24 | 4 | 0.00314232603 | 0.00909535 | PASS |
| slotted_translation | 48 | 6 | 0.00121675153 | 0.0355055 | PASS |
| slotted_translation | 96 | 12 | 0.000705195831 | 0.212666 | PASS |
| reversal | 24 | 4 | 0.00214503526 | 0.00333157 | PASS |
| reversal | 48 | 12 | 0.00109650193 | 0.0277968 | PASS |
| reversal | 96 | 24 | 0.000455113057 | 0.188232 | PASS |
| slotted_rotation | 32 | 720 | 0.00595200972 | 3.05794 | PASS |
| strain_constant_gas | 32 | 16 | 0 | 0.00736289 | PASS |
| strain_constant_liquid | 32 | 16 | 0 | 0.00878221 | PASS |
| nonuniform_strain | 24 | 8 | 0.00221421076 | 0.0132061 | PASS |
| nonuniform_strain | 48 | 16 | 0.000829700135 | 0.0632574 | PASS |
| nonuniform_strain | 96 | 32 | 0.000392066749 | 0.352145 | PASS |
| nonlinear_incompressible | 24 | 24 | 0.00213730443 | 0.0491764 | PASS |
| nonlinear_incompressible | 48 | 48 | 0.00109936517 | 0.21045 | PASS |
| nonlinear_incompressible | 96 | 96 | 0.000539882527 | 1.17305 | PASS |
| boundary_exit_translation | 48 | 48 | 0.000387844532 | 0.0980208 | PASS |
| boundary_exit_slab | 48 | 48 | 6.82324567e-17 | 0.144856 | PASS |

Total final compiled-fixture runtime: **7.953008 s**, including fraction reconstruction, analytical expectations and heavy CSV diagnostics. The 30 new momentum runs account for 1.903224 s. These are not production performance claims.

## Supporting Python evidence

The supplied manifest and nested original-archive manifest passed all 13 hashes. The corrected fixed-grid functions reran as 18 cases / 216 timesteps plus two analytical witnesses and exactly matched the shipped numerical records. Bundled Python 3.12.14 and NumPy 2.3.5 were used. Shapely was unavailable, so a recorded AST loader executed unchanged selected functions while omitting only unused Shapely imports and excluded moving-geometry functions. No moving-geometry checks were run or claimed. See `evidence/p1cm/reference_rerun/`. Its PASS is supporting evidence only.

## Source fingerprints and repository state

| File | SHA-256 |
|---|---|
| main.cpp | `0948d0730cc633960cc1467f545b68e476406cd2f413aa5ba01206d3372d0e3a` |
| mass_momentum.hpp | `f8998879468c050cff82217c161a97cd7e89a98dde3da782dc1de16539e87332` |
| momentum_validation.hpp | `5177a03fe25acd96a8416e308fe54b2931b0bb20428527b9fcd1a240b114d9d3` |
| vof.hpp | `4d584ecb1bc97121acc215b20c3f88be83493b65c3b83637f6c38bcb46ba4fa6` |
| transport_oracles.hpp | `f91097764f16587d4317c942ea73a8da6ae593ec0335ab1c67182101ca190714` |
| nonlinear_transport_oracle.hpp | `140f553aa19eaa6e6f015a2c3e9a2a0f3930827e8a92eb3111f00f5bda07f74a` |
| CMakeLists.txt | `9f2d6421b596fc91bc77f74a80e82b7640b4a56fc7ce08ecb5dc0af23c3d5e9c` |
| run_p1cm.sh | `82003c96ba24acb90ede244f206e25939acd198ca8fe0d0de7bdca597159d3c0` |
| record_p1cm.py | `cf067b9cbca1ab54da502f999f11596d9410ef2312e2c5b24bea4bdf98d307ce` |

Repository HEAD at execution: `48385a78afb2930744f2b87ad6aa55452f18ae18`. Tracked and staged changes were checked to be confined to the isolated prototype. Git status captured by this run, before the checkpoint commit:

```text
?? prototypes/
```

Current raw evidence and matching source snapshots are in `evidence/p1cm/current/`. `RUN_METADATA.json` fingerprints both sources and executed evidence. The preserved baseline and earlier complete-run logs remain separate. The build/evidence runner does not commit, tag or push. Its metadata records the execution state; the Git checkpoint is a separate operator-authorized action. Machine-specific `build_output.txt` is excluded from version control and the evidence manifest.

**Decision: P1-C-M PASS under the specified gates. Stop here for operator review. Integrated P1, pressure/body coupling and P1-D/E remain incomplete.**
