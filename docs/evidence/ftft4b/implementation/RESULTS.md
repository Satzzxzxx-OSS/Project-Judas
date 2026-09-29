# FTFT4B implementation stop: selected-policy energy contradiction

**BLOCKED by the brief's concrete method-contradiction rule. FTFT4B is not passing or repaired.**
Starting HEAD and origin/main were `c43af4c0b65c597bff7fd03bed55173f5c483139`, with a clean tree. No commit, push or tag. Protected/historical evidence remains unchanged.

## Work completed, and the exact boundary

Stage A adds `RigidMotion.h`: a step-local, full-handle-owned segment record with fixed start pose, start/end times, linear/angular velocities and evaluated endpoint. Evaluation calls the unchanged `IntegrateRigidBodyPosition` arithmetic from the segment anchor. `PhysicsWorld::Step` currently records **one segment at its existing drift location**; no event scheduler, force ordering, collision law or contact solver change has been installed. ResetBody clears the ledger. The zero-inverse-mass behaviour is retained.

The utility supports closing/restarting a segment, and its unit test preserves an out-and-back path. **Authoritative player queries still use the old endpoint path; Stage F is NOT implemented.** The existing world endpoint compatibility tests pass, including a separate collision elsewhere; this is not a claim that a new event loop has passed.

Stage A checks: **54 PASS** (initial version 52; two additional immovable-state checks close a guard omission before final validation). No-impact endpoints are compared bit-for-bit to the existing integrator. The original `RigidBody.cpp`, geometry, ContactSolver and numerical constants are unchanged.

While preparing the event response, a two-normal algebra check exposed an energy contradiction in the selected compression/Poisson-expansion equations. Integration stopped before Stages B–F. A compiled restricted policy witness and an independent exact-rational calculation confirm it. **The new policy witness uses actual Judas geometry/masses/inertia, but is NOT a call to a completed production PLUS handler or repaired PhysicsWorld::Step.** This distinction is deliberate.

## Minimal physical counterexample

Three freely moving bodies, two simultaneous normal contacts, no friction, force, gravity, constraints to the world or external work. Support bodies are massive but **finite and dynamic**, so their reaction velocities and kinetic energy are included.

- Body 0: mass 100 kg, one ordinary authored compound-box shape. A central box has half-extents (0.125,0.125,0.125) m. Four small boxes have half-extents (1/256,1/256,1/256) m and centres (2,-31/256,0), (-2,31/256,0), (-1/4,-31/256,0), (1/4,31/256,0) m. Symmetric pairs keep the mass centre at the body origin; engine volume-weighted compound inertia is used.
- Bodies 1/2: dynamic spheres, radius 1/32 m, mass 1,000,000 kg each; centres (2,-5/32,0), (-1/4,-5/32,0). Their materials have e=1 and e=0.5. Body 0 has e=0. The unchanged Judas pair rule (maximum restitution) gives those two contact CORs. All friction coefficients are zero.
- Initial body-0 velocity: (0,-19/9,0) m/s; spin (0,0,-4/9) rad/s, stored as binary32. Both spheres start at rest. Maximum mass ratio is 10,000, within the required family.
- Actual current `PrimitiveAt`/`ComputeContacts` finds **exactly two contacts**, each with **signed separation exactly 0** and normal (0,1,0). Body-0 lever arms are (2,-1/8,0) and (-1/4,-1/8,0). Sphere lever arms are (0,1/32,0), parallel to the impulse, so their spin stays zero.

`GetInertiaWorld` supplies body-0 Izz = 1.0665110349655151 kg m². Its full tensor and all pre/post velocities are in `validation/policy-witness-final.jsonl`. There is no geometric skin, gap collapse, feature classifier, mass cutoff or dropped reaction.

### Selected equations, not a new law

Let r=[2,-1/4] be the contact x arms, m=100, support masses M=1e6 and I=Izz. In normal coordinates:

```
u_i = v_y + r_i*omega_z - support_velocity_i
A_ij = 1/m + r_i*r_j/I + (i==j ? 1/M_i : 0)
A = [[ 3.7605482225417126, -0.45881840281771408],
     [-0.45881840281771408, 0.068603300352214264]]
```

The matrix is SPD, rank 2, determinant 0.047471692435816421. Thus the unique solution is also the minimum-2-norm solution; no redundant constraint choice, SVD threshold or active-set ambiguity is involved. Every computed normal impulse is strictly repulsive. Tangential impulse is exactly zero because mu=0; circular Coulomb compliance is automatic.

Each compression round solves `A_active * p_active = -u_active - A_active,exp * p_exp`. The next expansion budget is `e_eff * accumulated_compression_impulse`. Expansion applies that prescribed budget. A previously observing contact that becomes closing compresses in the next round. Remaining expansion budget has priority over labelling a zero-speed contact observing; otherwise even isolated restitution would be suppressed at the end of compression. This implements the brief's required Poisson restitution, not a new target-velocity rule.

All actual compression approach speeds exceed the existing 0.5 m/s capture threshold. No choice at the threshold changes the result. The normal block is linear with zero friction, so subdividing a phase for a tangential-state observation does not change its total normal impulse. Neither maxSlipDirectionChange=0.15 nor 0.01 can change these zero-friction equations.

The operations correspond to the selected PLUS normal compression equation (2.3), Poisson expansion (2.4), and Part II impact rounds/table 1. No Simbody source was copied. The earlier supplied energetic-work restitution candidate was **not** substituted.

### Raw four-round result

| Round | Contact states | Normal impulse increments (kg m/s) | Outgoing normal speeds (m/s) | Total KE (J) |
|---|---|---|---|---:|
| Initial | Both approaching | — | -3.0000000596, -2.0000000522 | 222.9448517827 |
| 0 | Compress, compress | 23.6656136966, 187.4285794433 | approximately 0, 0 | 0.0178462016 |
| 1 | Expand, expand | 23.6656136966, 93.7142897216 | 45.9978407909, -4.4291095129 | 336.7659856682 |
| 2 | Observe, compress | 0, 64.5611725700 | 16.3759867083, approximately 0 | 193.7917338709 |
| 3 | Observe, expand | 0, 32.2805862850 | 1.5650596669, 2.2145547565 | **229.5352968202** |

Final normal velocities are both separating. All expansion budgets are consumed; there is no pending impulse or unresolved closing contact. No event/round budget is exhausted. Four rounds, one full-rank normal block per round, zero active-set eliminations. Maximum recorded compression-equation residual is 8.881784197001252e-16 m/s.

**Final gain: 6.590445037525484 J.** This violates the unchanged +1e-4 J closed-system energy allowance by a large margin. Linear momentum residual is 0 in the double evaluation; angular momentum residual is 3.552713678800501e-15 kg m²/s. All bodies' translational and rotational kinetic energies are included. Conserving impulse does not establish energy consistency.

The independent `exact_poisson_check.py` treats the represented input values as exact fractions, solves the same unique blocks with no floating-point state tolerance, and computes budgets from body mechanics. It confirms:

- exact gain = **6.590445037524927 J**;
- exact linear and angular momentum residuals = **0**;
- reversed contact ordering produces exactly identical physical velocities;
- maximum C++ energy comparison discrepancy = **5.684341886080801e-13 J**;
- no attractive impulse; all compression equations close exactly; both final contact velocities nonnegative.

This is not an assertion about a unique compliant material answer. The failed independent oracle is simply that an initially unstrained, unforced, closed rigid system with e<=1 must not create unexplained kinetic energy. The selected per-contact mixed-COR round rule contradicts that requirement for an engine-supported configuration. A global energy clamp, another restitution law, or excluding mixed materials would change the approved method and was not attempted.

## Setup and evidence honesty

The first algebraic preflight used random SPD two-normal mobility matrices (seed 27); its third case raised the contradiction. That check did not establish current-engine geometry. The physical fixture above then supplied actual engine geometry/inertia and finite reaction bodies; the exact calculation removes roundoff and ordering as explanations.

The first physical-fixture construction used decimal coordinates. The engine did not return the intended exact touching pair, and the probe aborted with code 134 before producing a physical result. `policy-fixture-first.cpp` and `policy-first-run.json` preserve that setup failure. Coordinates were changed to exact binary fractions to establish touching; no physics assertion was weakened. The final compiled policy witness and exact checker both exit **1** for the demonstrated energy failure.

This is a stop-condition witness, not a general rank-aware impact implementation. The two-row specialization is sufficient because its matrix is nonsingular and every active impulse is admissible. The general SVD utility, frictional interval/Newton solve, event timing, persistent-contact handoff and ledger consumers remain **unimplemented**.

## Current-engine witnesses and regression scope

The reproducible runner builds current source and preserves the committed diagnostics unchanged. The original three-sphere world witness and its 36 variants still report energy failure, and the unchanged temporal checker still reports 13/45 failures. This is expected: no new world response or timing rule was installed. These failures are not relabelled passes.

`validation/commands.json` records commands, exit codes and wall times. `validation/summary.json` lists executed versus not-run work. Eight existing rigid/geometry/lifecycle/broadphase/cache/storage/physics executables, plus player/range geometry checks, are run to verify the small Stage A integration did not change those results. The motion test has 54 checks. The runner's zero exit means measurements completed; it does not mean FTFT4B is physically accepted.

The full application/editor/FTFT1–3 validation runs, full FTFT4A independent oracle, full impact/friction/TOI acceptance families and paired performance campaign were **not run** after the policy contradiction. Protected files and historical evidence are verified byte-for-byte. Broadphase-test wall time includes its oracle/instrumentation and is not a production-physics performance result. FTFT4A performance debt and the FTFT4B 1.10x gate remain open/unassessed.

## Changed files and next boundary

- `src/RigidMotion.h`: anchored segment utility; preserves existing integrator arithmetic and immovable-state semantics.
- `src/PhysicsWorld.cpp`: owns a ledger per body, records the existing single drift, clears it on reset. No event integration or new impact solver.
- `tests/RigidMotionTests.cpp`: compatibility/segment tests.
- `tests/PoissonPolicyWitness.cpp`: actual geometry/inertia plus restricted selected-policy contradiction.
- `CMakeLists.txt`: two test/probe targets.
- `scripts/ftft4b_policy_validation.py`: reproducible blocked-state validation.
- `docs/FTFT.md`, `docs/ARCHITECTURE.md`: implementation/contradiction status only.
- `docs/evidence/ftft4b/implementation/`: handoff provenance, immutable starting fingerprints, raw failure and regression evidence, exact reference and this report.

Do not advance to FTFT5/fluid work. Further response implementation requires operator review of this concrete contradiction. No alternative model is proposed or selected here.
