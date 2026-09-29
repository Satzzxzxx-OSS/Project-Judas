# Astra implementation brief — FTFT4B impact timing and simultaneous frictional response

## Authorization

Implement **FTFT4B only**.

Expected clean starting checkpoint:

`c43af4c0b65c597bff7fd03bed55173f5c483139`

Verify `HEAD`, `origin/main` and `git status` before editing.

If the tree is not clean or the expected checkpoint differs, report and stop. Do not reset, stash, rewrite history or guess.

Read:
- `RESEARCH_REPORT.md` in this handoff;
- the attached prior FTFT4B timing and impact-response research packages;
- the repository's committed FTFT4B diagnostic evidence.

The model decision is already made. **Do not start another literature/model-selection exercise.**

The selected architecture is:
- chronological time-of-impact handling;
- step-local motion-segment ledger;
- PLUS-inspired simultaneous frictional impact response for new impact events;
- existing Judas accumulated-impulse solver retained for persistent/resting contact.

The current committed diagnostics are acceptance witnesses, not tests to rewrite.

## Source references

Primary impact model:
Uchida, Sherman, Delp (2015),
“Making a meaningful impact: modelling simultaneous frictional collisions in spatial multibody systems”
DOI 10.1098/rspa.2014.0859
https://pmc.ncbi.nlm.nih.gov/articles/PMC4984984/

Simbody ImpulseSolver documentation:
https://simbody.github.io/simbody-3.6-doxygen/api/classSimTK_1_1ImpulseSolver.html

Simbody is Apache-2.0 licensed. It is an algorithm/source reference, **not authorization to add the complete Simbody library as a Judas dependency**.

If you directly adapt any Simbody implementation code, preserve required Apache-2.0 notices, identify those portions in the report and repository, and do not misrepresent adapted code as original Judas work.

## Non-negotiable constraints

- Preserve FTFT4A robust pair-local geometry, exact fallback, genuine signed gaps and local anchors.
- Preserve FTFT1–3 and protected fluid prototypes.
- Do not weaken tests or widen tolerances after seeing a failure.
- Do not restore gamma8 gap collapse.
- Do not use the rejected `-gap/dt + e*closingSpeed` target formula.
- Do not reapply gravity/forces at every impact event.
- Do not silently discard remaining fixed-step time.
- Do not convert event-budget exhaustion into a restitution/material rule.
- Do not replay a physical impact impulse as persistent-contact warm start.
- No scene/object/fixture-specific branches.
- No sleeping/damping as a correctness or performance fix.

## Implementation order

### Stage A — step-local motion segments

Add a step-local motion representation for authoritative rigid-body movement.

A segment stores at minimum:
- body handle/generation;
- physical start/end time within the fixed step;
- segment-start position/orientation;
- segment linear/angular velocity;
- evaluated end pose.

For a segment with unchanged velocities:

`p(t) = p0 + v * t`

For orientation, preserve Judas's current no-impact integration behavior using an anchored evaluation of its existing normalized first-order update from the **fixed segment start**, with elapsed segment time `t`.

Use the existing multiplication convention/order from `RigidBody.cpp`; do not rewrite it from this prose if the source convention differs.

Do **not** repeatedly apply the old quaternion update at every unrelated event boundary.

If an impact changes angular or linear velocity:
- evaluate the pose at the event;
- close the old segment;
- create a new segment from that event pose and the new velocity.

Required immediate tests:
- no-impact body endpoint is identical to pre-FTFT4B behavior;
- unrelated impact elsewhere does not change it;
- out-and-back path remains visible in the ledger.

### Stage B — conservative new-impact timing

Apply external force/torque velocity integration **once** at the fixed-step start.

Handle already-persistent support through the existing persistent solver with restitution disabled for those contacts.

For separated candidates, find the earliest new impact inside remaining physical time.

Use FTFT4A robust pair-local signed separation and conservative advancement.

A conservative relative speed bound must cover:
- relative linear speed;
- `|omegaA| * radiusA`;
- `|omegaB| * radiusB`.

Use conservative/outward arithmetic and existing exact fallback where the robust sign is unresolved.

Do not add an arbitrary contact skin or safety multiplier.

After every material impact:
- refresh affected motion segments;
- refresh affected proxy/candidate information;
- search the remaining time again.

Drift to the event before changing velocities.
After the final event, consume all remaining fixed-step time.

### Stage C — impact group construction

At the event pose:
- build the connected island of geometrically proximal contact points;
- include relevant existing persistent contacts in the connected island because an impact can transmit through them;
- classify participation from current relative velocities, not beginning-of-step velocities.

"Proximal" means supported by the FTFT4A robust contact/uncertainty representation or existing manifold contact. It is **not** a new arbitrary scene-distance tolerance.

Use stable body/contact-feature ordering for deterministic assembly, but the physical solution must not depend materially on creation order.

### Stage D — PLUS-inspired impact response

Time and configuration remain fixed during the impact event.

Assemble generalized velocities, contact Jacobians, inverse mass/inertia and the Delassus operator:

`A = G M^-1 G^T`

Use circular Coulomb friction.

#### Impact rounds

At the start of each round classify every proximal normal contact:

- Observing: current normal relative velocity is non-closing.
- Expanding: compressed in the previous round, received nonzero compression impulse, and effective COR is nonzero.
- Compressing: all other non-observing contacts.

A currently observing contact may become closing after impulses and participate in a later round.

#### Restitution

Use Poisson restitution.

For each contact:
- accumulate normal compression impulse;
- expansion budget = `effectiveCOR * compressionImpulse`.

Effective COR:
- if approach speed is below the existing Judas restitution/capture threshold, use 0;
- otherwise use the existing contact/material restitution.

Do not introduce another material restitution curve in FTFT4B.

#### Active-set interval solve

At the beginning of each interval:
- clearly sliding contacts are Sliding;
- other non-observing tangential contacts begin as Rolling candidates.

Start with plausible compressing normals active and rolling tangential constraints active.

Solve for the minimum-2-norm / least-squares impulse satisfying the active constraints.

Inspect the solution:
- compressing normal impulse must be repulsive;
- rolling tangential impulse must remain inside the circular Coulomb cone.

If violated:
- rolling violation -> transition that point to ImpendingSlip / remove the rolling equality;
- attractive compressing normal -> remove that normal constraint;
- remove the largest violation first and resolve.

The active set must strictly decrease in this inner correction process.

Sliding/impending impulses remain on the circular Coulomb limit and oppose the current/predicted slip direction as defined by the PLUS interval construction.

#### Evolving slip direction

Expose a numerical impact setting:
`maxSlipDirectionChange`.

Default for production candidate: `0.15 rad`.

For high-accuracy validation also run `0.01 rad`.

Subdivide an impact interval when:
- sliding changes to rolling/sticking;
- rolling becomes impending slip;
- sliding direction would rotate more than the permitted amount.

Report interval counts and convergence as this value is tightened.

Do not replace the circular cone with axis-aligned friction limits.

### Stage E — transition back to persistent support

When the instantaneous impact process finishes:
- touching/nonseparating contacts are eligible for persistent support;
- newly impacted contacts begin persistent support with zero support warm-start impulse;
- do not copy the physical impact impulse into the persistent cache;
- invalidate stale persistent warm-start data in any island materially changed by an impact.

The existing accumulated normal/friction persistent solver remains responsible for:
- gravity support;
- stacks;
- slopes;
- moving platforms;
- ordinary sustained friction.

Low-speed capture is the intended transition for Zeno-like micro-bounce behavior.

### Stage F — motion ledger consumers

Update authoritative dynamic-body sweep/query paths that currently assume a single previous->current chord.

At minimum, player/dynamic sweep logic must test the body's actual step segments.

Do not make presentation interpolation authoritative.

### Stage G — numerical safety

Use explicit diagnostics:
- number of TOI searches/iterations;
- impact events per step;
- impact rounds;
- PLUS intervals;
- active-set eliminations;
- low-speed capture events.

A high safety budget may prevent an accidental infinite loop, but:
- hitting it is a correctness failure;
- do not silently drop remaining time;
- do not force e=0 solely because the budget was reached;
- required validation fixtures may not exhaust it.

## Linear algebra

The impact interval requires rank-aware least-squares/minimum-norm solutions because redundant rigid contact constraints are expected.

Do not use unpivoted normal equations as the sole solver.

Reuse a portable rank-revealing backend already present in Judas if available.

If none exists, implement/add the smallest isolated rank-aware dense utility and validate it independently against known matrices before using it in impact response.

Do not add the whole Simbody library as a dependency.

Record:
- matrix sizes;
- detected rank;
- residual;
- condition diagnostics where available.

## Required validation

### Existing diagnostics

The committed c43af4c0 diagnostic witnesses must be rerun unchanged and pass physically.

1. Restitution-zero separated contact:
   - no indefinite pre-contact stop;
   - reaches contact at correct physical time.

2. Restitution 0.5 separated impact:
   - correct event time;
   - correct final position and velocity;
   - no premature rebound.

3. Induced collision chain:
   - second collision created by first is detected/resolved during remaining time.

4. Three-sphere energy witness:
   - initial KE 85.5625 J;
   - no unexplained energy gain;
   - momentum conserved;
   - equivalent creation-order permutations produce equivalent physical result within declared tolerance.

Do not assert one particular final velocity vector unless the selected PLUS policy and independent oracle establish it.

### Timing oracles

Run the supplied independent single-impact timing family.

Cover:
- e = 0, 0.5, 1;
- multiple gaps/speeds/timesteps;
- translated/rotated equivalents.

### Impulse budgets

Cover:
- mass ratios at least 1e-2 to 1e4;
- off-centre contacts;
- linear and angular momentum;
- restitution/energy behavior.

### Friction

Include the supplied off-centre coupled rotation/friction witness.

Also cover:
- sliding;
- sliding -> rolling;
- impending slip;
- tangent-basis rotation covariance;
- circular friction-cone compliance;
- no unexplained energy increase in closed cases;
- convergence comparison for maxSlipDirectionChange 0.15 -> 0.01.

### Redundant and simultaneous impacts

Include:
- symmetric four-point flat-face impact;
- redundant contact points;
- body/contact ordering permutations;
- induced ball chain;
- impact into a resting stack.

Verify symmetric geometry produces symmetric load distribution within numerical tolerance and no arbitrary contact subset is selected.

### Persistent contact regressions

Retain:
- resting box and sphere;
- five-box stack;
- rotated stack;
- slope;
- moving platform;
- lifecycle first-step behavior;
- friction/restitution current expected behavior where not intentionally superseded by the declared impact model.

Add:
- low-speed impact -> stable persistent support;
- high-speed impact -> persistent support on subsequent frame;
- no double application of impact via warm start.

### Motion history

Include:
- out-and-back body ending at its starting pose;
- player/dynamic sweep must still observe its path;
- collision chain candidate refresh;
- between-endpoint rotating impact witnesses.

### Rotation compatibility

No-impact bodies must preserve pre-FTFT4B endpoints regardless of unrelated impacts elsewhere.

Impacted bodies must use the same anchored segment equation for event search and pose advancement.

### Regression

Run all production suites plus FTFT1–4A validation and protected fluid fingerprint checks.

Do not modify fluid prototypes.

## Performance

Use Release builds and paired/interleaved runs.

Measure:
1. 1,500 persistent/resting crates;
2. collision-heavy falling bodies;
3. simultaneous redundant impacts;
4. induced chain;
5. frictional oblique impacts.

For the 1,500 resting-crate workload, this new impact handler should not run as a full PLUS solve every frame.

Gate for FTFT4B-specific regression:
- median whole-step time must be <= 1.10x the c43af4c0 baseline in the same paired run.

This does NOT close FTFT4A's pre-existing narrowphase performance debt.

If the gate is exceeded:
- report the measured mechanism;
- do not start an unrelated optimization campaign before review.

## Documentation and evidence

Update `docs/FTFT.md` and relevant architecture text honestly.

Preserve the old failing c43af4c0 evidence.

New evidence must include:
- exact source fingerprints;
- raw diagnostic results;
- event/round/interval counters;
- energy/momentum budgets;
- creation-order/covariance results;
- performance raw data;
- commands/environment;
- final git status.

## Stop condition

Fix ordinary compile/indexing/test bugs as needed.

If the faithfully implemented PLUS-inspired policy contradicts a required physical oracle:
- preserve the smallest counterexample;
- report the exact stage/equations involved;
- stop before inventing another model.

Do not return to literature/model selection.
Do not weaken the oracle.
Do not turn an event budget into a physics rule.

When all required cases are green, report for operator review and STOP.

No commit, push, milestone tag, FTFT5 or fluid work until approved.
