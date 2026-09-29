# FTFT7 source and coverage map

Baseline inspected: `a0ba6d1dbaf60954a08b165e09e60741469ac0f3`.
This map describes production paths and focused evidence; the final runner's
results/fingerprints establish the tested final source. Historical logs are
not rewritten.

## Actual paths

| Capability | Production path | Mechanism / limit |
|---|---|---|
| Authored celestial registration | `RuntimeWorld::Build` → `RebuildCelestialParticipants` (`src/RuntimeWorld.cpp`) | Live dynamic celestial entities plus a Local-mode vehicle; static celestial objects are separate point-mass sources. A vehicle with both components must occur once. |
| Full pair attraction | `StepPlayedWorld` → `CelestialGravity::ApplyForces` (`src/Simulation.cpp`, `src/CelestialGravity.cpp`) | One Newton force per unordered valid Full pair, applied with opposite signs. Positions are authoritative simulation positions. No prescribed orbit. |
| Cross-fidelity attraction | `StepPlayedWorld` → `ApplyCoarseCelestialForces` (`src/CoarseSimulation.cpp`) | One start-of-step force per pair containing an Active Coarse celestial. Full receiver accumulates force; Coarse receiver receives the reciprocal velocity kick. The runtime participant set includes Local vehicles without celestial metadata. No scratch allocation when no coarse celestial exists. |
| Coarse drift | `StepCoarseEntities` (`src/CoarseSimulation.cpp`) | Local authored acceleration and semi-implicit drift after the mutual-force kick. Ordinary settled props retain the existing stationary approximation; a nonzero mutual celestial force wakes a settled celestial. Coarse has no contact response. |
| Gravity selection | `StepPlayedWorld` (`src/Simulation.cpp`) | Ordinary and Local vehicles sample local gravity; Local vehicles also participate in mutual celestial attraction. Celestial-mode vehicles select static point-mass acceleration and omit local gravity. Static sources are external prescribed reservoirs, not closed-system dynamic pairs. |
| Thrust and SAS | `GameSession::HandleFrameInput` → `ApplyFlyingPrimitiveControl` (`src/FlyingPrimitiveControl.cpp`) | Thrust adds force along current body axes; rotation input adds torque. SAS computes shortest attitude error and damped angular acceleration, multiplies by current inertia, and adds torque. It does not assign orientation or linear velocity. |
| Velocity/pose integration | `PhysicsWorld::Step`, `IntegrateRigidBodyVelocity/Position` (`src/RigidBody.cpp`) | Force kick once, then drift; finite-step orbital energy error is expected. Quaternion drift is normalized first-order integration with FTFT4 anchored segments. Constant world angular velocity is an approximation: this is not exact torque-free asymmetric-body gyroscopic motion. |
| Frame reporting | `src/ReferenceFrame.cpp` | Position/direction transforms and velocity subtraction include `V + omega cross r`. Choosing a frame does not apply forces or rewrite the world's authoritative states. |
| Attachment/release | `HandlePilotToggleRequest` (`src/PilotControl.cpp`), `src/PilotAttachment.cpp` | Store body-local offset/orientation; follow the live body pose. Release inherits the actual attachment point's linear plus angular velocity. Gravity-facing egress is an explicit positional clearance correction; it does not recalculate inherited velocity at the corrected point. |

## Operator-thrust handle lifecycle

The orbital fixture also reproduced a separate stale-generation defect:
`RuntimeWorld::Build` populated operator-thrust handles only once. A celestial
entity demoted and reconstructed at identical state received no authored
operator impulse, and runtime-created celestial thrusters were omitted.
`development-orbit/thrust-before-results.log` preserves the four-case run
(41 checks, five failures, exit 1); the fresh-body control passed. The
pre-fix RuntimeWorld source is preserved alongside it. `RebuildCelestialParticipants`
now refreshes the operator inventory from live Full entity definitions as
well, retaining the participant deduplication repair. Force magnitudes and
input-direction calculations are unchanged. Final orbital execution records
verify this correction; no new control or force law is introduced.

## Existing coverage reused

The named tests call production implementations; no test count is treated as
a proof by itself. Final execution records belong to the stabilization runner.

| Target | Existing scope |
|---|---|
| `judas_celestial_gravity_tests` | Analytical inverse-square force, five-revolution equal/unequal mass orbits, barycentre/momentum/angular momentum/energy, half timestep, whole-universe rotation, impulse perturbation and escape. |
| `judas_spacecraft_control_tests` | 12 sections: thrust/mass response, coast, orientation versus existing velocity, counter/perpendicular thrust, torque/inertia, local gravity composition, uncontrolled no-op, rotation covariance. |
| `judas_spacecraft_flight_tests` | Four sections: celestial orbit, thrust/escape, SAS torque and translation independence, one-shot input routing. |
| `judas_reference_frame_tests` | Six sections: translating/rotating transforms, angular point velocity, live body/pilot frame telemetry, reset and rigid rotation covariance. |
| `judas_pilot_attachment_tests` | Eight sections: local capture, translation/roll/repeated rotation, universe rotation, release linear/angular/combined velocity. |
| `judas_pilot_dismount_tests` | Four sections: actual shape support, gravity-facing clearance, zero gravity, arbitrary rotated equivalent. |

Existing orbit engineering budgets are retained: period 2%, radius spread
2.5%, five-orbit energy and angular-momentum relative errors 2%, normalized
momentum `1e-5`, barycentre drift 1 mm; halving the timestep must lower energy
error. These are finite-step scenario bounds, not exact conservation claims.
Near collisions require timestep resolution; coincident point masses have
undefined Newton direction and production returns zero. Large absolute
origins use M23's fixed-origin local-coordinate contract, not live rebasing.

## Reproduced coarse defects and narrow correction

`coarse-pre-fix/run.log` executes the actual `StepPlayedWorld` against the
pre-fix engine. It returns **1: 125 checks, 14 failures, 86 steps**:

- Coarse→Full force used the old pose, but late Full→Coarse force sampled the
  already advanced Full body. Maximum normalized momentum residual: 0.00110095.
- A Local vehicle without celestial metadata received attraction from Coarse
  celestial bodies but was omitted from their reciprocal source list. Residual:
  0.0360697 (also reproduced after rotation/translation and velocity boost).
- A celestial demoted at rest stayed frozen under a nonzero mutual force.
  One-Coarse residual: 0.0891484. Both-Coarse had zero momentum error only
  because neither body accelerated; the independent Newton acceleration
  checks expose this false comfort.
- Ordinary Coarse entities erroneously received every static celestial source
  in addition to authored local gravity. In the same scene, Full velocity was
  `(1,1.95000005,3)`, Coarse `(0.900749266,1.79119885,3)`; independent local-field
  expectation was `(1,1.9499999973922968,3)`.

The repair computes shared mutual forces before either pose advances,
includes actual live participants, wakes only celestial receivers of nonzero
mutual force, and removes the unselected static-source force from Coarse.
Ordinary settled, inertial and dormant semantics otherwise remain unchanged.

`coarse-post-fix/run.log` records **125 checks, zero failures, 86 steps**.
Worst normalized momentum residual was `9.524962961546131e-8`, below the
unchanged `2e-6` engineering gate. First-step Newton velocity tolerance is
`5e-6 m/s`; the authored-local-gravity comparison uses `5e-7 m/s`. Both Full
and Coarse now produce `(1,1.95000005,3)` for that local-gravity witness.
The final runner rebuilds the subsequent equivalent scratch-reserve/wake
predicate cleanup and records final source fingerprints.

## Reproduction

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target judas_coarse_celestial_tests -j4
./build/judas_coarse_celestial_tests
```

The pre-fix witness used the same `-ffp-contract=off -O3 -DNDEBUG
-std=gnu++17 -Wall -Wextra` flags and linked the existing `build/libjudas_engine.a`,
SDL2 and `build/libjudas_glad.a`. Its source snapshots and source/binary hashes
are preserved beside the failing output. The binary is not evidence content.
