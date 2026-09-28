# FTFT4 source-specific review — 3404388

## Evidence boundary

Reviewed the user-provided `git archive` snapshot. ZIP comment equals
`3404388f1af48419505c802fc11dbf8a0b577f38`; source hashes are in
`evidence/SOURCE_MANIFEST.json`. The original ZIP and exported files were not
modified. This is not an independent check of the live remote.

Inspected actual PhysicsWorld, Narrowphase, Contacts, ContactSolver,
RigidBody, Broadphase, relevant tests, CMake and FTFT ledger. The source is
now available; the earlier access limitation is superseded for this snapshot.

**No complete Judas build or production test suite was executed here.** GLM
headers are absent and dependency retrieval failed. `source_build_attempt.log`
records the unsuccessful build. No substitute GLM implementation was used.
The export also excludes assets, third_party and tools needed by a full root
CMake build, so it should not be described as a buildable complete repo.

What was run: the prior four independent Python reference programs and C++
scalar gamma probe; ten evaluations of the scalar target branch extracted
from the actual solver; independent arithmetic checks of those evaluations.
The new real-PhysicsWorld and tree adapters are supplied but **unexecuted**.

## 1. Confirmed call chain

`PhysicsWorld::Step`, src/PhysicsWorld.cpp:616–768:

1. Save previous outer-step poses.
2. Integrate accumulated forces/torques into velocities once. Gravity was
   already applied by the caller through ApplyLinearAcceleration.
3. Expand bounds by velocity/angular reach and generate candidates once.
4. Expand compound children and generate current-pose margin contacts.
5. Collapse small negative penetrations using NumericalGapBound.
6. Match cached impulses using parent-A local anchors.
7. Prepare/SolveVelocities, then save impulses for the next step.
8. Integrate poses for the entire timestep using solved velocities.
9. SolvePositions and refresh bounds.

That order, rather than a missing coefficient, explains the separated-impact
position error. The new narrowphase and event work must preserve the force
ownership and previous-pose contracts.

## 2. The small-gap rule is exactly the reported mechanism

`PhysicsWorld.cpp:54–75` computes the gamma8 expression; lines 680–687 replace
a negative penetration with zero. The source comments incorrectly equate
being inside an upper bound on arithmetic error with being indistinguishable
from actual zero geometry.

The independently checked axis-aligned witnesses remain relevant: genuine
represented gaps of 0.030517578125 mm at local T=137, and 0.244140625 mm at
T=1024, are computed exactly yet lie below that threshold. The scalar branch
is not a proof of arbitrary-shape arithmetic; it is a counterexample to the
rule's claimed interpretation. Candidate generation is still required before
this affects the solver.

A real engine witness is supplied: tangential velocity ensures a nearby pair
is a candidate, while the normal motion closes only a quarter of the known
positive gap. After repair, that gap must not be reported as zero or produce
an immediate supporting impulse.

## 3. Precision is lost before NumericalGapBound

These locations must be addressed rather than just deleting the final clamp:

- `Narrowphase.cpp:13–18`: `PrimitiveAt` adds child-local offsets to the large
  parent position in binary32 before narrowphase receives them.
- `Contacts.cpp:12–19`: SphereVsBox obtains a world-space closest point and
  then subtracts it from the world-space sphere center at lines 66–81.
- `Contacts.cpp:315–405`: face clipping constructs world-space polygon
  vertices and subtracts world-space face centers.
- `PhysicsWorld.cpp:688–703`: warm-start anchors are recovered by subtracting
  a rounded world contact point from the parent position.
- `ContactSolver.cpp:75–79,126–128,170–172`: lever arms/position corrections
  repeat world additions/subtractions.

**Selected first repair:** relative geometry and solver anchors should be
computed in a pair-local frame in binary64 before any world-space float
conversion. World points can remain float presentation/debug outputs; they
must not be the only source of physical lever arms.

Contact equality also needs explicit treatment. Sphere functions reject
`distance >= radius_sum + margin`; SAT uses a strict `overlap > -margin`.
At zero margin exact touching is excluded by those predicates. Closed contact
classification should distinguish separated, touching and penetrating without
having to expand shapes according to distance from the local origin.

## 4. The documented 'one-frame hover' is too narrow

`ContactSolver.cpp:83–102` sets `bounces` from approach speed alone. A separated
pair whose speed exceeds 0.5 m/s and whose gap is reachable receives target
`-restitution*closingSpeed` immediately. This branch is taken even when
restitution is zero. `PhysicsWorld` then advances the entire step at the
resulting velocity.

For a central frictionless impact against an immovable plane, the scalar
branch and full-step drift give:

| Initial gap | Approach | e | Source drift end gap | Physical event end gap |
|---:|---:|---:|---:|---:|
| 0.001 m | 1 m/s | 0 | 0.001 m | 0 |
| 0.001 m | 1 m/s | 0.5 | 0.00933333 m | 0.00783333 m |
| 0.1 m | 6.38 m/s | 0.5 | 0.15316667 m | 0.00316667 m |

The exact binary32-input evaluations are in `source_target_probe.csv`.
These are executed scalar source excerpts plus analytical reduced-case
kinematics, not a linked PhysicsWorld execution.

**Source-derived consequence:** without a subsequent force, the e=0 body
keeps zero velocity at its positive gap. The next step's margin is zero,
there is no overlap, and nothing makes it land. It can therefore remain
stopped indefinitely. A four-step, zero-gravity real-world witness is in
`code/world_probe.cpp` for Astra to execute before implementation.

For positive restitution, a correct speed ratio does not establish correct
position or impact timing. The old rigid suite emphasizes rebound ratio and
does not settle this missing temporal check.

## 5. Correct event arithmetic, source sign convention

Judas's contact normal points from B toward A. Let closing normal speed be
`vn = dot(vA + omegaA x rA - vB - omegaB x rB,n) < 0`.

For a central normal impact with positive gap s, q=-vn, outer drift h and
constant velocities between impulses:

    tau = s/q
    vn_after = e*q
    s_end = e*q*(h-tau)

The average drift velocity is `e*q-(1+e)*s/h`, NOT the final normal velocity.
Do not install either drift expression as a universal restitution target.
Actual multi-body motion needs event advancement and updated remaining-time
candidates, not a per-contact 'move the body afterwards' correction.

The repair direction remains advance to impact / solve there / consume the
remaining time. Catto's GDC2013 discussion supports this distinction, but its
published example also explicitly admits rotational misses in a later
algorithm. It is not a blanket implementation/proof to copy uncritically.

## 6. Actual angular motion is not an exponential rotation

`RigidBody.cpp:44–51` uses normalized forward quaternion Euler:

    q(t)=normalize(q0 + .5*t*omega_quat*q0)

For a normalized initial quaternion and constant angular velocity, its turn
angle is `2*atan(|omega|*t/2)`, not `|omega|*t`. Its rate is bounded by |omega|,
so the radius-times-angular-speed envelope remains a valid conceptual bound
for this sampled trajectory. Exact swept arithmetic still needs verification.

A future TOI evaluator must sample the actual selected drift law. A full-360°
endpoint witness based on exponential motion is not a valid expected endpoint
for this current integrator. Changing the rotational integrator would be an
explicit, independently tested change, not an invisible CCD helper detail.

This is one reason to separate the geometry repair from event integration.

## 7. Broadphase findings and limits

The source contains a real DynamicAabbTree with updates and queries.
`ShapeBoundingRadius` includes compound-child offsets. `TightBound` includes
previous/current poses and a sphere envelope when endpoint orientation
changes; `CoverStepReach` adds velocity/angular reach. These mechanisms should
be preserved unless the independent replay finds a defect.

The current all-pairs comparison calls the SAME ComputeContacts with default
zero margin. It is useful for discrete broadphase completeness but does not
independently prove the narrowphase or future collision timing.

Candidates are generated once before all impulses. A post-impact velocity
change can require a new B-C collision even if B was stationary at the start.
That requirement belongs to the event pass; it is not evidence that the tree's
basic Query algorithm is wrong.

`tree_replay.cpp` calls the actual DynamicAabbTree with the supplied dyadic
proxy boxes. For those exact proxy boxes equality is appropriate. PhysicsWorld
fattened-query tests should require a conservative superset, not zero extras.
The replay remains unexecuted in this environment.

## 8. Explicit scope for the next handoff

I recommend one FTFT4 with two reviewed internal passes:

**FTFT4A, authorized by the supplied brief:** pair-local geometry, signed
separation, stable local anchors, conservative bounds and independent oracle
coverage. Preserve the temporal solver during this pass. Keep the known
impact-timing witnesses recorded as unresolved, not as accepted success.

**FTFT4B, not yet authorized:** event-aware drift/impulse/remainder integration,
now using reliable contact geometry. This includes cache lifetime, contacts
created by other impacts, actual rotational trajectory sampling, and terrain
scope. Its detailed integration must not be inferred from one flat-plane
formula. FTFT4 is not closed until both passes meet their requirements.

This is not splitting a failed feature off and calling it done. It is limiting
one change set so a geometry regression cannot be confused with a new event
scheduler regression, the exact failure pattern that consumed earlier work.

The next agent needs to execute source adapters and implement FTFT4A; it does
not need to research alternative contact models.

## 9. Other observations — do not smuggle them into FTFT4A

- The FTFT ledger inside the committed export still labels FTFT3 awaiting
  review/not committed. Operator evidence establishes its 3404388 checkpoint.
  Update the ledger status when already touching FTFT documentation.
- BodyHandle generation occupies 12 bits and wraps. No indefinite stale-handle
  guarantee follows from that representation. Add a lifecycle follow-up to
  FTFT6 rather than silently widening all handles during contact work.
- Terrain contacts use a specialized approximate surface sample and 14 box
  sample points. The primitive exact-geometry oracles do not certify this
  approximation or make Sample.signedDistance an exact SDF for CCD.

## 10. Primary references consulted

- Erin Catto, Continuous Collision, GDC2013, slides 5–8 and 13–14:
  https://box2d.org/files/ErinCatto_ContinuousCollision_GDC2013.pdf
- Erin Catto, Solver2D (2024):
  https://box2d.org/posts/2024/02/solver2d/
- Pharr/Jakob/Humphreys, PBRT4, Managing Rounding Error:
  https://pbr-book.org/4ed/Shapes/Managing_Rounding_Error

The source-specific observations above come from the exported Judas source,
not these publications. Prior independent exact/interval algorithms and
impact proofs are preserved unchanged in the attached original research ZIP.
