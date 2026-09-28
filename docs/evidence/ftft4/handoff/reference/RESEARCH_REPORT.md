# Judas FTFT4 — contact timing, numerical geometry, and broadphase research

## Decision and evidence boundary

Research date: 2026-09-27. Last user-reported production checkpoint:
`3404388f1af48419505c802fc11dbf8a0b577f38`.

**This is independent mathematical/reference-code research, NOT a source audit or a passing production FTFT4 implementation.** GitHub HTML/raw/codeload retrieval failed in this environment; `git ls-remote` and curl failed DNS resolution. The GitHub connector was discoverable but not installed. Conversation/library retrieval located reports, not the current physics source. Consequently no current Judas source was compiled, no Judas dynamic tree was run, and no actual current gamma8 implementation was modified or executed. See `sources/ACCESS.md`.

The reported numerical-gap policy admits a precise counterexample to the description “only gaps indistinguishable from zero.” A broad error bound does not establish that a small measured gap is actually zero. Independently, the speculative-hover/restitution issue is a collision-timing problem: one constant integration velocity generally cannot represent both the pre-impact travel and the post-impact final velocity.

Recommended direction:
1. Separate geometric gap/classification, deliberate proximity/cache policy, and physical contact activation.
2. Evaluate narrowphase in pair-local coordinates with explicit uncertainty; use a higher-precision/robust fallback where the sign is ambiguous. Do not enlarge collider geometry according to absolute coordinate size and label that a proof of touching.
3. Use time-of-impact-aware advancement for new separated impacts and consume the remaining physical time. Retain the existing accumulated/warm-started solver for actual contact and resting manifolds; no wholesale solver replacement is justified by these tests.
4. Replay supplied oracle fixtures against the actual current tree and narrowphase before selecting precise source edits.

The literature and elementary impact equations are sufficient to select this direction. The remaining inputs are current source, integration details and engine regressions—not a need for an open-ended fluid-style research expedition.

## 1. What the supplied Judas reports establish

The source-only audit and the attempt-2 report describe:
- speculative candidate reach based on relative translation plus angular reach over one fixed step;
- a gap policy `gamma8 * (|pA| + |c-pA| + |pB| + |c-pB|)`;
- collapsing positive gaps below that quantity before constraint solving;
- accumulated impulses, friction, warm starting and clipped manifolds;
- early stopping of sufficiently fast zero-restitution bodies at their pre-impact gap;
- a rejected `-gap/dt + e*closing_speed` velocity target which damaged restitution;
- a real dynamic AABB tree and already-existing oracle tests.

Those are reported mechanisms, not new source observations in this pass. “A bound is not established for all paths” is distinct from proving it underestimates error on a particular path. This pass does not claim an actual-current-code underestimated-error counterexample. It establishes that using the reported bound to declare zero separation can collapse an exactly representable and exactly computed nonzero separation.

## 2. Three different numerical concepts must not be conflated

### Stored-geometry truth

IEEE input bits define the represented positions/dimensions. An independent oracle should first evaluate that represented geometry at high or exact precision. It must not silently substitute the intended pre-rounding geometry.

### Arithmetic uncertainty

Rotation, relative transforms, dot products, closest-feature selection, clipping and normalization introduce their own error. That error must be analyzed through the actual operations. An interval straddling zero says the fast computation has not decided the sign; it does not prove touching.

### Deliberate contact/proximity policy

Keeping near pairs/manifolds in a cache and using a numerical contact envelope can be legitimate engineering. Its size, units and effect must be declared independently of any assertion of exact overlap. A candidate is not itself permission for an immediate bounce, friction impulse, or ghost support across a known gap.

### Counterexample family for the reported gamma8 expression

Two axis-aligned unit boxes have centers `pA=T`, `pB=T+1+g` on x and half-widths 0.5. Inputs are actual binary32 numbers. Choose `pB` two representable float steps above `T+1`. Let `c` be the midpoint between the opposing faces. Then the exact gap is `pB-pA-1`; in these cases ordinary binary32 subtraction computes it exactly.

| T (local m) | Exact stored/computed gap (m) | Reported gamma8 policy (m) |
|---:|---:|---:|
| 0 | 2.384185791015625e-7 | 9.536749985272763e-7 |
| 137 | 3.0517578125e-5 | 1.316071429755539e-4 |
| 1024 | 2.44140625e-4 | 9.775168728083372e-4 |
| 32768 | 0.0078125 | 0.0312509760260582 |
| 1048576 | 0.25 | 1.000001668930054 |

Both NumPy/binary32 and a compiled C++17 probe verified this selected family. **All five known-positive gaps fall under the reported collapse threshold, with zero gap-arithmetic error.** The C++ uses `-O2 -fno-fast-math`. These are scalar replicas of the reported policy, not calls into Judas.

This does not mean stationary bodies automatically collide at these distances: the production report says the rule is applied only after speculative contact creation. For a runtime witness choose a closing speed that makes the positive gap a candidate within the timestep, then check the raw gap, policy classification, impact time and physical response separately.

Large T here means distance from the LOCAL simulation origin, not a billion-metre absolute offset already subtracted by M23. The last two rows also highlight limited local-float range; they are not a claim that M23 promises submillimetre precision there.

A second negative control authors a 0.001 m gap near local coordinate 1e6; binary32 stores it as zero. Widening the already-rounded values cannot recover the lost authoring information. A contact fix cannot substitute for a coordinate representation/rebase contract.

## 3. Independent exact geometric oracles

`geometry_reference.py` implements:
- sphere/sphere overlap by exact squared center distance;
- sphere/OBB overlap by exact closest-point squared distance;
- OBB/OBB overlap using every nonzero face/edge-cross SAT axis;
- compound collision as union of actual children, not their convex hull;
- exact squared exterior box distance from vertex/box and edge/edge features;
- exact fixed-orientation box/box translational TOI by intersecting temporal SAT slabs.

Rational rotations use `R(q)/dot(q,q)`, so the declared mathematical rotations are orthogonal exactly. This is a reference convention, not a verified statement about Judas's quaternion normalization path. A future adapter must record the actual stored quaternion bits and distinguish mathematical normalization from rounded runtime matrix construction.

An outward-rounded binary64 interval filter is checked against exact predicates. For uncertain signs it falls back to the exact predicate. All branches operate on represented geometry, not a guessed origin-distance tolerance.

Executed:
- 287 shape pairs; 75 have separately authored exact expected contact signs;
- 287 rigid-transform covariance checks;
- 2,127 interval enclosure comparisons against rational values;
- 271 decisions resolved by the filter, 16 exact fallbacks;
- 287 exact AABB-superset checks and three compound-union checks;
- 27 exact exterior box-distance checks;
- 18 exact translational box TOI checks;
- 28 checks of SAT separation as a LOWER bound on exterior Euclidean distance;
- an explicit compound central-opening sweep.

The SAT value is **not** generally the Euclidean exterior distance. The code labels that distinction. The exact oracles are not proposed as per-frame arbitrary-precision engine replacements. This interval filter does not yet cover the production compound-transform and manifold-clipping pipelines; those remain actual-source work.

## 4. Why the hover fix needs time, not another restitution target

Consider a single flat normal contact, positive gap `s`, positive closing speed `q`, timestep `h`, and effective restitution `e` in [0,1]. Assume constant-velocity drift between impulses. If `s <= q*h`, the impact time is

    tau = s/q.

The correct end-of-step normal separation and terminal relative velocity are

    s_end = e*q*(h-tau) = e*(q*h-s)
    v_end = e*q.

The average velocity that reproduces the displacement is instead

    v_drift = (s_end-s)/h = e*q - (1+e)*s/h.

Unless the initial gap is zero, `v_drift` and `v_end` differ. The rejected expression `e*q-s/h` is generally neither. Treating the terminal bounce velocity as the velocity for the whole step also gives the wrong final position, even if its speed ratio passes.

Examples, h=1/60 s:

| s | q | e | Correct s_end | Correct v_end | Immediate-bounce s_end | Rejected target velocity |
|---:|---:|---:|---:|---:|---:|---:|
| .001 | 1 | 0 | 0 | 0 | .001 | -.06 |
| .001 | 1 | .5 | .00783333333 | .5 | .00933333333 | .44 |
| .1 | 6.38 | .5 | .00316666667 | 3.19 | .15316666667 | -2.81 |

**This corrects the earlier assistant recommendation too:** the proposed `-s/h + e*q` patch was not a valid general post-impact velocity rule. Its failure was predictable analytically.

The reference event implementation advances to contact, applies the impulse at contact, then advances the remaining time. It matches the closed-form result in 432 gap/speed/restitution/timestep cases (including misses and initial touching), max discrepancy 4.44e-16.

This establishes an isolated contact rule, not a complete engine TOI scheduler.

## 5. Two-body impulse and invariants

Let normal n point from A to B, contact arms be rA/rB, and

    vn = n dot [(vB + omegaB cross rB) - (vA + omegaA cross rA)].

At a genuine closing contact, the inverse effective mass is

    k = 1/mA + 1/mB
        + (rA cross n)^T invIA (rA cross n)
        + (rB cross n)^T invIB (rB cross n).

The frictionless impact magnitude is

    j = -(1+e)*vn/k.

Apply `-j*n` to A and `+j*n` to B at the same contact point, with their corresponding angular impulses. For an isolated impact the energy change is

    DeltaK = -(1-e^2)*vn^2/(2*k).

This is the oracle. Under a production low-speed restitution policy, use its explicitly selected effective e, not a contradictory raw-material expectation.

Executed: 300 arbitrary off-center two-body impulse cases, each with rotated and Galilean-boosted equivalents. Masses range from 1e-2 to 1e4. Worst errors:
- normalized linear momentum 2.46e-16;
- normalized angular momentum 7.30e-16;
- normalized energy-law residual 8.77e-16;
- normalized restitution residual 8.91e-16;
- covariance difference 3.13e-15 (absolute velocity/angular-velocity units).

Also 54 frictional sphere/plane impacts retain the normal restitution result and do not increase kinetic energy. These are single-contact checks, not evidence that a stack or simultaneous multi-contact impact has been solved.

## 6. Actual shape sweeps and changing event order

A stable quadratic sphere/sphere TOI implementation was compared to 90-decimal-digit mpmath evaluation for 300 selected crossings/misses; max TOI difference 9.99e-16 s. These are well-conditioned fixtures, not a proof for arbitrary near-tangent inputs.

A piecewise-quadratic sphere/box sweep handles face, edge and corner regions, rather than pretending an expanded box has rounded corners. Nineteen checks pass; the expanded-box false-corner-hit negative control is rejected. This research helper uses a declared absolute 1e-13 root/endpoint tolerance and is NOT a certified production CCD implementation.

The exact box/box translation oracle handles arbitrary fixed orientations. Compounds take child-pair minima; a gap through an open compound remains a gap.

A three-body one-dimensional elastic chain illustrates why candidate reach must be updated after an impulse: A hits B at .08 s, B then hits C at .16 s, within a .2 s step. B was initially stationary, so its original swept envelope did not predict the later collision. Final positions (.8,1.8,2.4), velocities (0,0,10), momentum 10 and energy 50 are checked independently.

For general engine implementation:
- existing resting contacts must not be repeatedly treated as fresh restitution events;
- simultaneous contact groups need coherent resolution, not independent contradictory pair impulses;
- never reapply the full-step gravity/force kick at every TOI segment;
- the angular trajectory used by the sweep must match the pose integrator;
- warm-start state and broadphase reach must be refreshed/invalidated appropriately;
- no event-budget exhaustion may silently become “no collision” or discard the remaining time.

Those are integration requirements; no current Judas behavior on them was tested here.

## 7. Broadphase: conservative envelopes, not false certification

I generated deterministic direct-tree replay data containing 41 creates, 27 moves, 14 destroys and 400 query oracles. Queries come from an exhaustive live-AABB registry. **The Judas tree adapter has not run.** These numbers are generated fixtures, not production passes.

Fat AABB false positives are legitimate; conservative inclusion of real candidates is the correctness condition. Exact collision-pair equality may be checked after narrowphase. Destroyed identities must never reappear as live query results.

Reference swept-bound tests cover:
- exact touching as closed AABB overlap;
- a half-turn rod whose intermediate shape hits an obstacle although both endpoint AABBs miss it;
- a full-revolution rod with clear endpoints but an intermediate sphere impact;
- compound bounding radii including child offsets;
- 7,200 transformed vertices within COM-segment-plus-circumsphere envelopes.

The full-revolution analytical first impact is at 0.003900964329756167 s. A conservative-advancement example using distance and a global angular speed bound reaches 0.0039009643297397384 s in 20 iterations. It is a useful witness against endpoint-only sign checks, NOT certification of arbitrary angular CCD or its runtime.

For linear COM motion, the AABB of the center segment expanded by a true body circumsphere radius encloses every orientation. That is conservative but can be loose. The reported Judas implementation already includes angular reach and spherical coverage; these negative controls are things it should survive, not evidence that it currently fails.

## 8. Literature findings and what NOT to copy blindly

- Catto's 2013 continuous-collision presentation explicitly explains speculative slowing, special restitution treatment, and the alternative of advancing to impact plus consuming the remaining substep. It also exposes missed angular glancing impacts in its particular later algorithm. Do not inherit an acknowledged miss and claim universal CCD.
- Catto's Solver2D notes describe local contact anchors and delta-position arithmetic for improved large-coordinate robustness. They do not make representation precision unlimited or automatically supply certified contact predicates.
- PBRT's rounding-error chapter shows operation-specific error propagation, interval arithmetic and higher-precision fallbacks. Its ray algorithms are not drop-in rigid-body narrowphase, but the uncertainty-handling principle is applicable.
- Shewchuk's adaptive-predicate work is a primary precedent for escalating precision on uncertain signs. Orientation/incircle routines are not themselves box-distance/manifold solvers.
- Box2D's documentation treats speculative candidate contacts as potentially separated and describes approximate multi-contact restitution and an explicit low-speed policy. Constants and approximations are not inherently dishonest; claiming more than their guarantees is.
- Multi-contact impact literature makes clear that isolated pair restitution is not universal proof of simultaneous-impact correctness.

Sources and reading scope are itemized in SOURCES.md.

## 9. Recommended FTFT4 implementation boundaries

### Ready now
- Exact represented-geometry witnesses and conservative candidate/query fixture files.
- Rejection of the blanket inference `gap below gamma8 => geometrically zero`.
- Distinction between proximity persistence, raw separation and physical activation.
- Isolated TOI + impulse + remaining-time acceptance equations.
- Negative controls for the rejected velocity formula, endpoint-only sweeps, compound hull/radius mistakes and stale post-impact candidate reach.

### Needs actual current source before prescribing a production patch
- Actual separation arithmetic and normal/manifold feature-selection paths.
- Quaternion/compound transform conventions and all stored rounding points.
- Contact cache activation and existing iteration/substep order.
- Which existing sweep APIs can supply TOI versus sampled player queries.
- Broadphase query/update semantics and handle invalidation.
- Integration of true TOI handling with existing contacts, forces, friction and angular pose advancement.

### Required actual engine regression after implementation
- First-step support, stable flat/rotated stacks, slopes and moving platforms.
- Restitution-zero landing and nonzero-restitution speed AND position.
- Grazing misses, high angular motion, compound openings, dynamic/dynamic chains.
- Tree oracle replay and existing player sweep/broadphase tests.
- M29 reconstruction and FTFT1 save behavior.
- FTFT2 async and FTFT3 gravity preservation.
- Existing fluids unchanged in source, with legitimate rigid-response changes measured.
- Benchmarks at the current 1,500-body workload; no performance extrapolation from these Python fixtures.

No production numerical fix or broadphase replacement is approved by this report. The next concrete input is the current physics source snapshot, not another request for Astra to invent numerical physics.

## Reproduction

    python code/run_all.py

Dependencies are NumPy 2.3.5, mpmath 1.3.0 and a C++17 compiler named g++. Reference compiler: g++ 14.2.0. `run_all.py` records commands/versions and deliberately marks every production-execution field false. Generated binaries go in `.build/` and are not shipped.

The archive manifest covers source, notes and recorded results. Verify it before rerunning; reruns intentionally overwrite timing-containing reference result files. All results were repeated from a fresh archive extraction, comparing all non-timing JSON values and C++ output.
