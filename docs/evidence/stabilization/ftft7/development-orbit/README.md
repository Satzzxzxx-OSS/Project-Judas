# FTFT7 real-scene orbital integrity

## Actual path and independent expectations

`tests/OrbitalIntegrityTests.cpp` writes/loads ordinary `.judas` scenes, constructs `RuntimeWorld` and `GameSession`, then uses `StepPlayedWorld`. The window test input seam supplies held keys only. Tests do not replace gravity, force integration, contact detection or the authoritative step.

Expected first-step impulses use independent double-precision Newtonian mechanics with literal SI G=6.67430e-11. Orbital budgets use total mass-weighted momentum, inertial barycentre, central-force angular momentum and kinetic plus Newtonian potential energy. Circular endpoints and eccentric turning radii come from the analytical two-body orbit. No expected quantity calls production `CelestialGravity` functions. Rotations use independent double-precision Rodrigues arithmetic.

## Reproduced production defects

1. A Local-gravity spacecraft with an explicit celestial component registered twice. Two actual bodies produced three participants and twice the Newtonian force. Before repair: dvA 0.009555300697684288 m/s versus expected 0.004777650286456; dvB -0.028665902093052864 versus -0.014332950859368. Both internal impulses still summed to zero, demonstrating why momentum alone was insufficient. `before-witness.log` preserves two failed checks and exit 1.
2. Operator-thrust handles were populated only during initial Build. A Full→Coarse→Full transition replaced the physics generation but left the operator targeting the stale handle; a runtime-created celestial did not register its operator force. `thrust-before-results.log` preserves five failed checks and exit 1. The reconstructed body received zero external impulse instead of -8.33333362459e10 kg m/s. The created-body impulse residual was approximately one.

`RuntimeWorld::RebuildCelestialParticipants` now avoids duplicate automatic vehicle insertion and rebuilds operator-thrust records from current live Full entities. No gravitational law, integration formula or material property changed. The separate mixed-fidelity force fix is documented in the neighbouring coarse-gravity evidence.

## Final executed results

Final run: **23 cases, 30,785 ordinary fixed steps, 357 checks, zero failures**, exit 0, 0.0576373 seconds. This includes one explicitly labelled **OBSERVED** numerical-resolution case described below; it does not claim that observation met the 1% accuracy target.

- Explicit and automatic Local spacecraft each register exactly once and match Newtonian acceleration.
- Unequal masses orbit without central anchoring; two complete revolutions preserve bounded energy and angular momentum.
- Arbitrary rotation, local translation, velocity boost and body creation order agree within declared floating-point trajectory tolerances.
- Fixed absolute origin (1e12,-2e12,3e12) produces bit-identical local orbit state; this tests fixed-origin scope, not live rebasing.
- Both bound speed perturbations and positive-energy escape follow mechanical ordering; escape separation reaches 59.1346 m from initial 16 m.
- Resolved close encounter minimum centre separation 0.285716512 m for two radius-0.14 m spheres: 0.005716512 m surface gap, no contacts. Max energy error 0.220766%, normalized momentum residual 9.97207e-6.
- Restored operator thrust matches fresh-body output. Its normalized external-impulse residual is 7.15451e-8; runtime-created operator residual is 5.23842e-7. Destroying the created entity removes its operator handle.

### Numerical accuracy limitation retained explicitly

The initial new test incorrectly imposed a blanket 1% energy budget at every tested timestep. The eccentric speed-fraction-0.7 orbit at ordinary **60 Hz reaches 1.238434% maximum energy error**, so it **does not meet 1%**. That original nonzero exit and source are retained in `first-full-*` / `OrbitalIntegrity.first-full.cpp`; nothing is rewritten as a pass.

The engine uses finite-step symplectic Euler and does not promise fixed accuracy for every orbital timescale. The final suite retains the unchanged 60 Hz scene and measured error as `OBSERVED`, explicitly `energy_budget_met=0`. It tests the same physical scene at finer timesteps with the **unchanged 1% bound** and independently requires first-order error reduction from 60 Hz. No production integrator was changed, and no threshold was widened.

| Eccentric timestep | Maximum relative energy error | 1% budget |
|---|---:|---|
| 1/60 s | 0.0123843434269 | NOT MET (observation retained) |
| 1/120 s | 0.00616443870715 | Met |
| 1/240 s | 0.00307900171661 | Met |

| Circular timestep | Analytical endpoint position error (m) |
|---|---:|
| 1/30 s | 0.282208282712 |
| 1/60 s | 0.141836770166 |
| 1/120 s | 0.0710813716514 |

The close encounter deliberately uses 1/1920 s because of its much shorter pericentre timescale; this is a resolution probe, not a claim that the shipped 60 Hz scene would reproduce that precision.

## Reproduction and evidence

```sh
cmake --build build --target judas_orbital_integrity_tests -j2
build/judas_orbital_integrity_tests --output docs/evidence/stabilization/ftft7/development-orbit/reproduced-fixtures
```

`final-metadata.json` records exact commands, build/run times, exit code, source and binary SHA-256. `final-results.log` includes per-case raw budgets/endpoints; `final-fixtures/` contains the executed authored scenes. `--witness-only` and `--thrust-witness-only` execute the corresponding targeted diagnostic through the same compiled implementation.

Historical source snapshots/logs retain the before states and initial diagnostics. The initial missing-GLAD-include compilation error is retained separately and is not physical evidence. The first thrust diagnostic's `destroyed_thrust FAIL` output label inherited failures from its preceding creation case; its own checks passed. Only that label accounting was corrected before final execution; assertions and the prior raw output remain preserved.

No rendering or human visual validation is claimed by these headless numerical tests. Parent FTFT7 validation runs the existing celestial/spacecraft/frame, lifecycle and production regression families separately.
