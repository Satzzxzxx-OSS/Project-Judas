# FTFT4 — source-specific contact/broadphase repair
## First internal pass: FTFT4A, geometry and numerical contact truth

FTFT = FixThisFuckingThing.

Implement FTFT4A only, then report and STOP. FTFT4B is the impact-time integration
pass; it is not authorized here. Full FTFT4 remains open until both are complete.
Do not begin FTFT5 or fluid research.

## Baseline and supporting evidence

Expected reviewed checkpoint:
`3404388f1af48419505c802fc11dbf8a0b577f38`.
Verify actual HEAD and worktree; do not reset newer/unrelated work.
Read `SOURCE_REVIEW.md` and `evidence/SOURCE_MANIFEST.json` in this package.
The supplied source archive was inspected directly, but the complete engine
could not be compiled by the reviewer because GLM was unavailable. Do NOT cite
its unexecuted source adapters as already-passing evidence.

The original independent research ZIP supplies exact/interval reference
geometry, distance and temporal witnesses. Its four Python programs and C++
scalar probe were rerun. These are supporting oracles, not engine acceptance.

All protected FTFT1/FTFT2/FTFT3 code/evidence and fluid prototypes stay intact,
except the directly intended physics geometry changes and minimal integration
required below. No production fluid, persistence, resource or gravity changes.

## 1. Reproduce before editing

Build from the actual complete repository, not the partial source export.
Run current rigid-contact, broadphase and affected production regressions.
Run the supplied `code/run_source_probes.sh <repo> <diagnostic-build-dir>`.
The script uses actual PhysicsWorld/DynamicAabbTree and needs ordinary GLM.
Fix adapter-only compile errors if found; do not alter physics to make a
baseline reproduction easier.

Record:
- known positive-gap classification with tangent motion and normal motion
  that cannot close the gap this step;
- four-step zero-gravity premature stopping;
- nonzero-restitution end position as well as end velocity;
- the raw-tree replay and existing lifecycle/broadphase checks.

The temporal witnesses are known OPEN FTFT4B issues. Do not reinterpret them
as passes merely because FTFT4A does not fix them.

## 2. Selected numerical design

Separate:
A. represented shape geometry and its signed separation;
B. uncertainty in evaluating that geometry;
C. conservative broadphase/proximity/cache policy.

An error interval containing zero is not proof the true separation is zero.
A fat AABB or candidate margin is not physical collision geometry.

Implement pair-local binary64 geometry for the existing sphere, box and
compound-box contact path. Promote stored float positions BEFORE subtracting
or composing child offsets. Do not reconstruct a large world-space float
point and subtract it again to obtain a tiny separation or lever arm.

Conventions:
- sphere gap: distance between centers minus radius sum;
- sphere-box gap: sphere-to-box closest distance minus sphere radius;
- box-box: test all nonzero SAT face/edge axes for separation; SAT separation
  is not generally Euclidean distance;
- compound: union of actual children, retaining parent identity and child
  indices, never the convex hull of the compound.

Keep Judas's normal convention: from B toward A.
Keep positive separation distinct from penetration. A generated nearby
candidate may have a positive gap; preserve it through contact generation.
Exact equality belongs to the closed touching classification, not to the
strictly separated class.

### Orientation and input meaning

Record the ACTUAL stored binary32 position, dimension and quaternion inputs
when evaluating an oracle. Quantization before the algorithm is not an
arithmetic error that it can undo.

Use one consistent proper rotation interpretation for nonzero quaternions
in relative geometry and its oracle. Normalize in binary64 or use the
algebraically equivalent R(q)/dot(q,q) matrix construction. The rational
reference uses that latter convention. Do not compare it to a different
unnormalized matrix and call the discrepancy a physics failure.

Verify existing quaternion input contracts before changing public validation.
Do not silently map an invalid zero quaternion to an arbitrary valid shape.
No global rigid-body-state/double-coordinate conversion or new live rebasing.

### Uncertainty handling

Use operation-specific error/interval checks for sign-critical calculations,
following the supplied interval reference. Re-evaluate uncertain signs in
higher precision. Exact/robust predicates are permitted where required.

Do not replace gamma8 with another guessed scalar that grows with origin
magnitude. Do not claim a universal floating-point proof from a finite test
set. A precisely resolved positive gap must remain positive.

If a path cannot resolve its sign reliably, represent that uncertainty
explicitly and preserve a witness; do not call it known zero. Report unresolved
supported fixtures rather than changing their expected values.

There is no requirement to use arbitrary precision for every normal contact.
Keep the fast common case cheap; test fallback invocation and cost honestly.

## 3. Exact source work sites

Review and repair these linked calculations together:

- `Narrowphase.cpp::PrimitiveAt`: compose parent/child poses in relative double
  precision for collision, avoiding its current float world-center intermediate.
- `Contacts.cpp::ClosestPointOnOBB` and `SphereVsBox`: compute closest point,
  difference and normal locally before world conversion.
- `TestSATAxis` / `ClassifyBoxBox`: reliable sign classification distinct from
  face-versus-edge manifold preference; do not discard a real separating axis
  merely to prefer a stable face manifold.
- `BoxVsBoxManifold`: clip and reduce candidate points in the pair/reference
  frame, not via rounded large world vertices.
- `PhysicsWorld.cpp` contact assembly and warm-start matching: retain local
  parent anchors from precise geometry, rather than recovering them from the
  debug world point.
- `ContactSolver.cpp::Prepare` and `SolvePositions`: consume those local
  anchors; evaluate world differences from relative offsets without adding
  and subtracting large floats. Do not change friction/restitution laws here.
- `ShapeAabb`: bound the same represented geometry conservatively. Round
  outward on conversion to float; do not shrink a bound during conversion.

Small internal data/interface additions are authorized. Preserve public
engine-facing contracts where practical and keep debug/world points as
presentation outputs, not the sole physical anchor representation.

Replace the current NumericalGapBound zero-collapse path only after the
relative calculations and signed-gap tests are in place. Do not just delete
it while retaining the arithmetic that originally required it.

Terrain is a specialized approximate path. Preserve it and test regressions;
do not certify it using primitive exact predicates, or reintroduce a global
false zero-gap proof to avoid addressing an exposed problem. Any unresolved
terrain effect must remain visible in the report.

## 4. Broadphase: keep it unless evidence requires a local fix

Replay the actual tree with the supplied 400 query expectations.
For exact supplied raw proxy AABBs, equality is appropriate.
For PhysicsWorld fat bounds, extra candidates are allowed; missed relevant
pairs and stale/dead handles are not.

Test moving/reinserted/destroyed/recreated proxies, rotated shapes, compound
child offsets, and player sweep candidate completeness.

Existing exhaustive comparisons use the same narrowphase. Keep them, but
add independently expected shape pairs to expose shared narrowphase mistakes.

Do not replace the tree with a grid, all-pairs runtime path or a new library.
Post-impact candidate regeneration belongs to FTFT4B.

## 5. Required independent geometry checks

Use the supplied rational/interval functions or a genuinely independent
higher-precision oracle. Tests must evaluate exactly the represented inputs.

Cover:
- sphere-sphere, sphere-box faces/edges/corners;
- rotated box-box face and edge contacts, including nearly parallel edges;
- compounds with an actual opening and offset children;
- exact touching, penetration, and both signs of small nonzero separation;
- shape sizes 1e-3 to 1e3 metres;
- local translations 0, 10, 137, 1000 metres and rotated equivalents;
- larger-coordinate diagnostics, separately labelled representation limits;
- canonical ±axis, oblique gravity fixtures and zero-gravity queries.

Specifically retain positive represented gamma witnesses at T=137 and 1024.
Do not use a pre-rounding 'intended gap' as truth after float storage erased it.

Check contact normals, local anchor placement and manifold points as well as
hit/miss. Independently verify points lie on the intended features within the
actual numerical error, not the old 5 cm cache matching radius.

A normal cannot be uniquely rotation-covariant for exactly coincident,
perfectly symmetric shapes. Label such degenerate conventions rather than
inventing an impossible unique-normal oracle.

## 6. Behaviour and performance gates

- Exactly touching supported bodies still get first-step contact.
- No known-positive gap becomes zero due only to coordinate magnitude.
- The non-closing positive-gap source witness receives no ghost support.
- Existing resting boxes/spheres, rotated rest, friction, slopes, platform
  and stack requirements pass without widening assertions or hiding drift.
- M29 reconstruction/persistence does not regain the gravity-step spike or
  false movement records. Test valid baselines through FTFT1's real policy.
- Run the complete current production suites and applicable near/far,
  editor Play/Stop and protected FTFT regressions.
- Geometry changes may legitimately change numeric contact trajectories;
  explain changes using physical/oracle evidence, not blanket rebaselining.
- Measure the same 1500-body fixture before/after on this machine, over
  repeated runs. Record median/range and fallback counts. Investigate material
  regressions; do not claim prior-machine milliseconds as a current benchmark.

No new sleeping, velocity clamps, stronger damping, widened save tolerances,
fixture names or force-discarding branches.

Do not change ContactSolver's separated-impact timing target yet. Its known
failures remain explicit FTFT4B evidence, not grounds to mask geometry errors.

## 7. Working and delivery protocol

Implement the connected geometry repair, not another literature review.
Ordinary compile/indexing errors may be corrected without stopping after an
arbitrary failed-attempt count. Preserve reversible checkpoints.

If a genuine mathematical incompatibility blocks the specified approach,
return the smallest counterexample and exact operation; do not switch models.

Store source/evidence durably in docs/evidence/ftft4/ or another reviewed
tracked evidence directory, never exclusively /tmp or disposable build output.
Do not commit the attached archive as a substitute for reproducible source.

Update docs/FTFT.md:
- FTFT3 is checkpointed at 3404388;
- FTFT4A status and actual evidence;
- FTFT4B remains open for event timing;
- full FTFT4 is not closed by this pass.

Report:
1. Executed baseline witnesses and what they show.
2. Geometry/uncertainty/anchor mechanism and exact changed files.
3. Oracle coverage and concrete residuals, including uncertain cases.
4. Tree/player-sweep and production regression results.
5. Same-machine performance and fallback cost.
6. Remaining temporal/terrain/representation limits without overstating them.
7. Final git state and proposed commit message.

Then STOP for review. No commit, push, tag, FTFT4B, FTFT5 or fluid integration.
