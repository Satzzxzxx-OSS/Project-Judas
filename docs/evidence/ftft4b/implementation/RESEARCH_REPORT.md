# Project Judas — FTFT4B final research decision

## Status

This document is the technical-lead decision for the FTFT4B implementation pass.

Current expected production checkpoint:

`c43af4c0b65c597bff7fd03bed55173f5c483139`

That checkpoint records the current-engine FTFT4B diagnostic failures. It does **not** repair production physics.

FTFT4A geometry/cache work is already preserved underneath that checkpoint. Its remaining performance debt is still open and is not silently waived by FTFT4B.

## What is demonstrated in current Judas

The committed diagnostics establish the following current-engine defects:

1. **Premature inelastic stopping**
   - A separated restitution-zero body can have its post-impact velocity applied before it reaches the surface.
   - In zero gravity it can remain stopped at the pre-contact gap rather than landing.

2. **Premature rebound**
   - A separated bouncing body can receive its rebound velocity before the actual contact time.
   - Its rebound speed may look plausible while its final position is physically wrong.

3. **Induced-collision timing**
   - An impact can change another body's velocity and create a second collision inside the same fixed step.
   - Candidate motion based only on the beginning-of-step trajectory is insufficient.

4. **Simultaneous-impact energy creation**
   - Current PhysicsWorld three-sphere witness:
     - masses: 10, 0.125, 10 kg
     - initial vx: 1, -3, 4 m/s
     - radius 0.5 m
     - restitution 1
     - friction 0
     - gravity 0
   - initial kinetic energy: 85.5625 J
   - final kinetic energy: approximately 88.3828808665 J
   - increase: approximately +2.8203808665 J
   - momentum remains approximately conserved.
   - The defect therefore is not merely a missing equal-and-opposite impulse.

The diagnostic checkpoint must remain preserved as before-evidence.

## Research decision

FTFT4B should not attempt to repair these defects by inventing another single "target velocity" for speculative contacts.

The selected architecture is:

1. **Chronological impact timing** for genuinely new impacts:
   - advance to time of impact;
   - resolve the instantaneous impact at that configuration;
   - advance through the remaining part of the fixed step.

2. **A motion-segment ledger**:
   - authoritative sweeps use the path actually travelled;
   - a body that moves out and returns to its starting pose is not treated as stationary.

3. **A PLUS-inspired simultaneous frictional impact handler** for a new impact event:
   - Poisson compression/expansion restitution;
   - impact rounds for induced contacts;
   - simultaneous treatment of currently participating proximal contacts;
   - circular Coulomb friction;
   - sliding / rolling / impending-slip state transitions;
   - least-squares/minimum-2-norm load spreading for redundant rigid contact points;
   - non-attractive normal impulses;
   - low-speed capture into steady support.

4. **Existing Judas accumulated-impulse solver remains the persistent-contact solver.**
   - FTFT4B does not replace resting/support contact with the impact handler.
   - Physical impact impulse is not replayed later as a warm-start support impulse.

This split is deliberate. Mature impact literature distinguishes instantaneous impact response from sustained contact handling.

## Why PLUS was selected

Primary reference:

Thomas K. Uchida, Michael A. Sherman, Scott L. Delp,
“Making a meaningful impact: modelling simultaneous frictional collisions in spatial multibody systems,”
Proceedings of the Royal Society A 471 (2015), 20140859.
DOI: 10.1098/rspa.2014.0859
Open article: https://pmc.ncbi.nlm.nih.gov/articles/PMC4984984/

The PLUS model directly addresses the exact research gap that remained after the Judas-specific counterexamples:

- spatial simultaneous impacts;
- friction coupled to normal response;
- redundant contact points;
- induced impacts;
- compression and expansion;
- sliding direction changes;
- rolling / sliding transitions;
- load spreading;
- numerical tolerances;
- low-speed transition to steady contact.

The paper reports an open-source implementation in Simbody. Simbody is Apache-2.0 licensed:
https://simbody.github.io/3.7.0/simbody_license_page.html

Simbody API documentation also exposes the same impact/contact distinction and classifications:
https://simbody.github.io/simbody-3.6-doxygen/api/classSimTK_1_1ImpulseSolver.html

This handoff does **not** authorize adding Simbody as a dependency. The paper and Simbody are algorithmic references. If implementation code is copied rather than independently reimplemented, Apache-2.0 attribution/license obligations must be preserved and the copied/adapted portions identified explicitly.

## Important findings from the independent Judas research

### A. Correct single-impact timing

For a separated contact with gap `s`, closing speed `q > 0`, restitution `e`, and remaining step time `h`:

`tau = s / q`

For a stationary plane and constant pre-impact velocity, the post-impact contact-normal speed is `e*q` away from the plane, and the remaining motion occurs for `h - tau`.

This is why one velocity cannot simultaneously represent:
- pre-impact travel,
- impact,
- post-impact travel.

A 1 mm gap, 1 m/s approach, 1/60 s step gives:
- e=0: finish touching at 0 m/s;
- e=0.5: finish 7.833333... mm away at 0.5 m/s.

### B. Impact can create another impact

An independently checked chain contains:
- A->B at t=0.08 s;
- B->C at t=0.16 s.

B was initially stationary. Events/candidates must be updated after the first impulse.

### C. Current orientation integration is partition-dependent

Repeatedly applying Judas's existing normalized first-order quaternion update to arbitrary event substeps changes the trajectory of a body that never collided.

Therefore FTFT4B must not globally "substep the world" through the old integrator.

The selected compatibility policy is an **anchored motion segment**:

For a segment beginning at `(q0, omega, t0)`, evaluate the existing one-step quaternion expression as a function of elapsed segment time from the fixed segment start:

`q(dt) = normalize(q0 + 0.5 * dt * (omegaQuat * q0))`

using the engine's existing multiplication convention and arithmetic order.

- Evaluating it at the full fixed-step duration reproduces the current no-impact endpoint.
- Merely observing an unrelated event does not restart the orientation update.
- If an impact changes angular velocity, close the segment at the event pose and create a new segment.

This is a compatibility choice for FTFT4B, not a claim that the normalized-Euler path is the final ideal rotational integrator.

### D. Frozen Newton restitution targets create energy in coupled impacts

The actual Judas three-sphere diagnostic demonstrates this in production.

Independent reference work showed that reevaluating which contacts are actually compressing after impulses avoids that exact failure in frictionless cases, motivating impact rounds rather than fixed beginning-of-solve rebound targets.

### E. Existing-style friction can also inject energy

An independent off-centre scalar specialization found:
- initial KE = 0.505 J
- after source-style iterative normal/friction solve ~= 0.539333 J

This is not yet claimed as a separate current-production witness. It demonstrates why FTFT4B must not simply attach the old friction iteration to a new normal-impact model and assume energy consistency.

### F. Low-speed capture is a physical/numerical policy, not an event-count fallback

Ideal restitution models can accumulate arbitrarily many low-speed impacts in finite physical time.

The selected policy is velocity-based capture into steady contact:
- below the existing Judas restitution threshold, effective impact COR becomes zero;
- the contact transitions to persistent support.

A high diagnostic work cap remains a safety/error detector only. Hitting it is not an accepted physical solution and must not silently discard remaining step time.

## PLUS-specific policy adopted for Judas

### Impact rounds

At an event configuration, collect the connected proximal contact island.

At the beginning of each impact round:
- **observing**: normal relative velocity is non-closing;
- **expanding**: the contact compressed in the prior round, received nonzero compression impulse, and has nonzero effective COR;
- **compressing**: remaining non-observing contacts.

Impulses can induce observing contacts to begin closing. Those participate in a later round.

### Poisson restitution

For contact k:

`normal_expansion_impulse[k] = e_eff[k] * normal_compression_impulse[k]`

where:
- `e_eff = 0` below the existing Judas low-speed restitution/capture threshold;
- otherwise use the existing contact/material restitution value.

FTFT4B does not introduce a new velocity-dependent material curve beyond that existing threshold policy.

### Redundant constraints and load spreading

For each impact interval:
- begin with all plausible compressing normal constraints active;
- non-sliding non-observing contacts begin with rolling tangential constraints active;
- solve the constrained system using a **minimum-2-norm / least-squares impulse** among redundant constraints;
- reject attractive normal impulses;
- reject rolling solutions outside the circular Coulomb friction cone;
- remove the largest violated active constraint and resolve;
- the active set strictly decreases during this inner correction loop.

This is based on the PLUS active-set strategy and is intended to avoid arbitrary load placement among redundant rigid contact points.

### Friction state

Use the current Judas friction coefficient for both static/dynamic impact friction in FTFT4B; do not add new material coefficients in this repair.

Tangential state:
- sliding;
- rolling;
- impending slip.

Use a named numerical transition-speed setting. If Judas already has an equivalent velocity tolerance, reuse it. Otherwise add an explicit impact setting, documented as numerical state classification rather than a material property.

Sliding friction:
- circular Coulomb cone;
- impulse opposes current slip direction;
- do not use a coordinate-aligned friction pyramid.

The impact interval must be subdivided when the tangential slip direction changes beyond the configured maximum angle or a sliding/rolling transition occurs.

For the initial implementation:
- expose `maxSlipDirectionChange`;
- default to 0.15 rad, a value explicitly explored in the PLUS paper;
- validation also runs a higher-accuracy 0.01 rad reference and checks convergence rather than pretending 0.15 is exact.

### Persistent contact handoff

After instantaneous impact resolution:
- touching, nonseparating contacts enter/return to the ordinary persistent-contact system;
- a newly impacted contact begins persistent support with **zero warm-start support impulse**;
- do not copy the physical impact impulse into the support cache;
- if an existing persistent island participates in a material impact, invalidate affected cached support impulses for subsequent frames rather than replaying stale load data;
- the existing accumulated normal/friction solver remains responsible for gravity support, resting stacks, moving platforms, slopes and sustained friction.

## Time integration selected for FTFT4B

Within one fixed physics step:

1. Apply gravity/external force and torque velocity updates **once**.
2. Resolve already-persistent support using the existing support solver, with restitution disabled for persistent contacts.
3. Create motion segments from the post-force/post-support velocities.
4. Search for the earliest new impact in remaining time.
5. Drift authoritative poses to that event time using the segment evaluators.
6. Build the connected proximal impact group.
7. Run the PLUS-inspired impact handler while time/configuration remain fixed.
8. Close/restart motion segments only for bodies whose velocities changed.
9. Refresh affected proxies/candidates and continue through remaining physical time.
10. Drift to the end of the fixed step.
11. Apply position-only overlap cleanup for residual numerical penetration.
12. Store the motion ledger for authoritative swept queries/presentation consumers that need the body's actual within-step path.

Never:
- reapply a full gravity/force kick at each event;
- apply the impact impulse again as a support warm start;
- use one start/end chord for a body that had multiple motion segments.

## Collision event search

Use the repaired FTFT4A geometry as authoritative.

Broadphase:
- conservative swept bounds for the current motion segment and remaining interval;
- after an impact changes a trajectory, refresh the affected bodies' proxies/candidates.

Narrowphase event search:
- use the robust signed-separation witness supplied by FTFT4A;
- use conservative advancement for separated candidates;
- a safe speed bound includes relative linear speed plus angular reach:
  `|vA-vB| + |omegaA|*RA + |omegaB|*RB`;
- for the anchored quaternion path, instantaneous angular speed does not exceed the segment's `|omega|`, making this a conservative rotational reach bound;
- use outward/conservative arithmetic consistent with FTFT4A;
- if the robust sign is numerically unresolved, use the existing exact fallback rather than a new distance skin;
- genuine positive gaps remain positive.

Do not use an arbitrary safety factor or restore the removed gamma8 "collapse to touching" rule.

A high diagnostic iteration/work budget is allowed only as a correctness guard:
- expose a failure/stat if exceeded;
- do not silently declare a collision, drop the remaining timestep, or force restitution to zero because a counter ran out.

## Motion ledger requirements

For every dynamic body that moves during a step, retain an ordered list of segments:
- segment start/end physical time;
- start/end position;
- start/end orientation;
- segment linear/angular velocity.

Authoritative dynamic-body sweeps must inspect all relevant segments.

A body may leave and return to the same final pose; its path must not collapse to a zero-length chord.

The ledger is step-local and does not become persistent scene state.

## Linear algebra / implementation guidance

PLUS requires least-squares/minimum-norm solves for small impact islands.

Do not add Simbody as a whole-engine dependency for FTFT4B.

Preferred order:
1. reuse an existing production-quality rank-revealing dense least-squares/SVD backend already available in Judas, if one exists and is portable;
2. otherwise add one small isolated numerical utility with explicit tests against independently computed matrices;
3. if adapting Apache-2.0 Simbody code, retain required notices and clearly document provenance.

Do not substitute ordinary normal equations without conditioning evidence.

## Performance principle

The high-fidelity impact handler runs only for **new instantaneous impacts**.

The ordinary 1,500-resting-crate workload should remain on the persistent-contact path and must not execute PLUS intervals every frame.

FTFT4A's existing performance debt remains separately OPEN; FTFT4B must not hide it with sleeping, skipped contacts or lower geometry fidelity.

## Acceptance requirements

### 1. Preserve all existing repaired geometry and FTFT checkpoints

Run:
- FTFT1;
- FTFT2;
- FTFT3;
- FTFT4A geometry/cache/storage;
- current production suites;
- protected fluid prototype fingerprints.

Do not silently rewrite historical diagnostic evidence.

### 2. Current-engine FTFT4B diagnostic witnesses must turn green

#### Inelastic separated impact
A zero-gravity, restitution-zero body starting with a real positive gap:
- reaches contact at the correct physical time;
- does not stop short;
- consumes remaining step time appropriately;
- stays touching/nonpenetrating thereafter.

#### Restitution 0.5 timing
The existing 1 mm / 1 m/s / 1/60 s type witness must match the independent analytical position and velocity:
- no early rebound;
- no penetration.

#### Induced collision chain
The existing chain must resolve both impacts at the correct order/times rather than using only original trajectories.

#### Three-sphere energy witness
Use the committed c43af4c0 fixture unchanged:
- initial KE 85.5625 J;
- no unexplained kinetic-energy gain;
- linear momentum conserved to the declared numerical tolerance;
- outcome invariant under equivalent body creation ordering within numerical tolerance.

Do not require one particular final velocity vector merely because a simplified research model produced it; simultaneous rigid impact can be model-dependent. The selected PLUS policy defines the accepted result.

### 3. Single-impact physical checks

Cover:
- e = 0, 0.5, 1;
- mass ratios at least 1e-2 through 1e4;
- off-centre contacts;
- translated/rotated equivalents;
- frictionless impact momentum and angular-momentum budgets;
- energy behavior consistent with restitution policy.

### 4. Frictional impact checks

Must include:
- the off-centre friction/rotation counterexample from the supplied research;
- sliding impact;
- sliding->rolling transition;
- impending slip;
- tangent-basis rotation covariance;
- circular Coulomb cone;
- no material kinetic-energy creation not accounted for by external work;
- convergence of a representative result when `maxSlipDirectionChange` is tightened from 0.15 toward 0.01 rad.

### 5. Redundant/simultaneous contacts

Include:
- flat box face with redundant contact points;
- symmetric four-point impact;
- creation/order permutations;
- equal load symmetry where geometry/mass are symmetric;
- ball-chain induced-impact case;
- mixed contact restitution values where supported.

Verify minimum-norm load spreading does not select an effectively arbitrary contact subset.

### 6. Persistent support transition

Existing:
- resting box/sphere;
- 5-box stack;
- slope friction;
- moving platform;
- rotated-universe support;
- lifecycle first-step behavior.

New checks:
- low-speed impact captured into support without micro-bounce;
- no physical impact impulse replayed as next-frame support warm start;
- support remains stable after a preceding high-speed impact;
- an impact into a resting stack transfers through the impact group and then returns to persistent support.

### 7. Motion-history / query checks

- out-and-back body whose final pose equals initial pose;
- player/dynamic sweep still observes the travelled segments;
- broadphase/event refresh after an impulse;
- no missed second impact created by the first.

### 8. Rotation compatibility

For bodies with no impact:
- final orientation must reproduce the pre-FTFT4B current endpoint regardless of unrelated impact events elsewhere.

For impacted bodies:
- collision prediction and actual pose advancement must use the same anchored segment equation.

Run high-angular-speed hostile cases and the supplied between-endpoints rotation witnesses.

### 9. Progress / safety

Record:
- impact events per step;
- impact rounds per event;
- PLUS intervals;
- active-set changes;
- conservative-advancement iterations;
- any capture-to-support events.

No required case may hit a safety budget.

Budget exhaustion is a reported correctness failure, not a successful fallback.

### 10. Performance

Use paired/interleaved Release measurements.

Required:
- current 1,500 resting crates;
- collision-heavy falling/throwing workload;
- symmetric redundant-impact workload;
- chain-impact workload;
- frictional oblique-impact workload.

For the resting-crate workload, FTFT4B should add no material impact-handler cost because those contacts are persistent. Use a 10% median whole-step regression gate versus the c43af4c0 production baseline for this pass. If exceeded, report before starting unrelated optimization.

Do not claim recovery of the older ~4 ms pre-FTFT4A number; FTFT4A's narrowphase performance debt remains separate.

## Scope exclusions

FTFT4B does not:
- alter FTFT4A geometric fidelity to recover speed;
- solve deformable-body/crumple physics;
- add compliant contact to production;
- modify fluids;
- implement live origin rebasing;
- change authored material schemas beyond what is necessary to preserve existing restitution/friction semantics;
- add sleeping as a performance fix;
- replace the persistent solver.

## Success condition

FTFT4B is successful only if:
- the committed production timing/energy defects are actually corrected;
- simultaneous/frictional impact behavior follows the declared PLUS-inspired policy;
- persistent contact regressions remain green;
- motion history and impact-chain queries are correct;
- no unexplained energy creation appears in the required closed-system fixtures;
- progress and performance gates pass.

A green test count alone is not enough.
