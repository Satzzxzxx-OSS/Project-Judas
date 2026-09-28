# FTFT4B research: collision time, motion history, and induced impacts

## Status and execution boundary

This is independently executed research, not a modification of Judas and not an FTFT4B pass.
The operator reports production checkpoint `422fd275c5f8b54ad4ff37d851eab24a36e8e089`, with FTFT4A correctness partially accepted and performance/FTFT4B still open.

I used the uploaded correct-but-slow FTFT4A source export, the later allocation-only patch/report, and the final source-fingerprint manifest. `RigidBody.cpp/.h`, `Door.cpp`, `Simulation.cpp`, and `PlayerController.cpp` match the later supplied fingerprints. The exported `ContactSolver` and `PhysicsWorld` predate cache optimizations; their hashes do not match the final source. The operator's report says their numerical/timing law was unchanged, but this is not a byte-for-byte current checkout. A raw-GitHub fetch failed DNS; GLM is unavailable here. No complete Judas build/test is claimed.

Programs executed here:
- exact rational 1D multi-body event scheduling;
- two explicitly experimental induced-impact models;
- quaternion-drift partition checks;
- exact motion-history/endpoint-bound comparisons;
- 80-digit analytical rotating-box/plane witnesses;
- a compiled scalar extraction of the normal solver/rotation equations.

`code/production_energy_witness.cpp` is supplied for the actual engine but was NOT compiled or run here.

## Executive decision

The chronology repair remains: advance to an actual impact, apply the impulse there, then consume the remaining interval. However, a safe engine implementation must carry piecewise motion history, keep force/warm-start accounting separate from event count, use a single consistent angular trajectory, and handle newly induced contacts.

Two issues were exposed before a production handoff:
1. Repeated normalized-Euler rotation makes pose depend on arbitrary event subdivision.
2. Freezing independent Newton restitution targets across a coupled set of initially closing and initially separating contacts can create energy, even with exact momentum conservation.

Neither is fixed by a new gap tolerance. The first has a precise kinematic alternative. The second has an explicit small engine witness and candidate research models, but no general 3D frictional replacement is approved by this report.

## 1. Source map

In the supplied export:
- `PhysicsWorld::Step` applies force integration once, builds current/speculative contacts, solves velocities, then integrates positions for the whole fixed step. This is the root of early stopping/rebound for separated contacts.
- `ContactSolver::Prepare` freezes restitution targets before warm starts, a necessary discipline. The separated fast-impact branch can assign post-impact normal velocity before reaching the surface.
- `RigidBody::IntegrateRigidBodyPosition` advances orientation using normalized explicit quaternion Euler.
- `PhysicsWorld` stores previous/current poses for sweeps and history bounds. A pair of endpoints is not enough once paths contain a bounce.
- `Door::FixedUpdate` uses `ResetBody`. Reset zeroes velocities and resets both poses in the supplied source. Such a reset is not evidence of a supplied continuous kinematic trajectory.

Do not silently turn every reset/teleport into physical movement. A true kinematic sweep needs an explicit trajectory and matching point velocity. Conversely, do not claim moving-door CCD from endpoint resets.

## 2. Correct chronology is not a restitution target formula

Positive initial gap s, closing speed q, constant-velocity drift duration h, restitution e:

    impact time tau = s/q
    end separation = e*q*(h-tau), when tau <= h
    terminal normal velocity = e*q

The displacement-average normal velocity is instead

    v_drift = e*q - (1+e)*s/h.

No single one of these velocities can simultaneously stand for terminal bounce velocity and full-step drift in general. The previously suggested `-s/h+e*q` target is not the solution.

The event reference matches independent exact formulas in 108 isolated wall cases, including misses, exact-endpoint impacts, and repeated no-force post-impact frames. For zero restitution it reaches the surface rather than hovering in the initial gap.

It also checks 21 elastic chains (2--8 bodies), nine multiple-wall-rebound cases, and 120 resting-stack steps. Contacts transmit impulse through touching support rows, gravity is kicked once per fixed step, and a closed body chain does not acquire momentum.

One chain hits at 0.08 and 0.16 seconds during a 0.2-second step. Reusing only the initial candidate set misses the second event. Exhausting an event budget returns UNRESOLVED with consumed/remaining time, not success or a fabricated collision-free remainder.

These are exact 1D frictionless reference calculations. They do not certify the production broadphase, arbitrary angular TOI, friction, or the production iterative solver.

## 3. The whole timestep needs a motion ledger

A body centre begins at x=0.5, moves right at 10 m/s, rebounds at a wall with allowable centre x=1.9, and ends a 0.28 s step back at x=0.5.

    actual centre at half step: 1.9
    endpoint interpolation:   0.5

For radius 0.1, the true swept bound is [0.4,2.0]; the endpoint-only bound is [0.4,0.6]. A query near x=1.5 must not miss the body because it returned to its starting point.

Six exact cases, 101 time samples each plus bound checks, pass (618 assertions). One and three rebounds are covered. The expected paths come from independent unfolded-coordinate reflection formulas.

Required engine contract:
- retain per-body actual drift segments across a fixed step;
- query poses from those segments for authoritative moving-body sweeps;
- use the union of conservative segment bounds for history queries;
- refresh future candidate reach after impulses;
- keep presentation downstream, but do not use a visually interpolated chord as authoritative movement history;
- record segments only for actual trajectory changes, not every unrelated global event.

This is a consequence of adding piecewise trajectories. It is not a claim that current endpoint-based rendering is a newly reproduced production defect in FTFT4A.

## 4. Angular drift and event partition must agree

For constant world angular velocity magnitude w, the source's normalized Euler update rotates by

    theta_E(h) = 2 atan(w*h/2).

Splitting into n equal pieces instead rotates by

    theta_split = 2*n*atan(w*h/(2*n)).

They differ. At w=100 rad/s and h=1/60 s:

    one step:   1.3894765523934 rad
    two halves: 1.5791644787990 rad
    difference: 0.1896879264056 rad (about 10.87 degrees)

This is a source-equation result, not evidence that the current unsplit engine already responds to unrelated TOI events. A naive new global substep loop WOULD introduce that dependency.

I tested 240 arbitrary orientation/axis/partition cases. Exact constant-omega exponential drift agrees across partitions to worst quaternion distance 1.1843e-14. Anchoring the old Euler path to one original segment instead of restarting it also preserves its old endpoint to 2.544e-16.

Those are two different policies:
- anchored legacy path preserves the old numerical trajectory;
- exponential drift follows the stored constant omega exactly.

The legacy path's actual angular derivative decreases as

    theta_E'(t) = w / (1+(w*t/2)^2).

Using that path in CCD while using stored w as the contact point's exact angular velocity is a discretization mismatch. For coherent constant-velocity drift, I recommend an explicitly approved exponential orientation update shared by CCD and pose integration. This intentionally changes the old drift; it must not be described as bit-identical optimization.

This does NOT implement full torque-free asymmetric-top dynamics or change how external torque is sampled. Those are separate integration contracts.

## 5. Rotation requires searching the interval, including initially departing contacts

Twenty-four rotating OBB/plane configurations produce 48 analytical event witnesses, evaluated at 80 decimal digits:
- an initially separated rod can hit and exit before returning to a clear endpoint;
- a rod initially touching but departing can contact again later in the same interval.

All root and adjacent-sign checks pass (168 assertions). The shape function is

    gap(theta) = centre_height - a*|sin(theta)| - b*|cos(theta)|.

An initial-touch flag cannot suppress the pair for the remainder of the frame. Neither can matching endpoints prove no angular impact.

This file supplies analytical witnesses, not a production rotational root finder. Earlier research already demonstrated a simple conservative-advancement witness. The current extension must still handle persistent-contact feature changes and explicitly report unresolved searches rather than return NO_HIT.

## 6. A concrete energy counterexample for frozen pairwise restitution targets

Three touching collinear bodies:

    masses:     [10, 1/8, 10] kg
    velocities: [1, -3, 4] m/s
    e = 1; friction = 0

The first contact is closing at 4 m/s. The second is initially separating at 7 m/s. Freezing the normal targets as [4,0] and solving both contacts together gives:

    velocities = [73/161, 717/161, 717/161]
    initial energy = 1369/16 = 85.5625 J
    final energy = 37327/368 = 101.432065217... J
    energy increase = 365/23 = 15.869565217... J

Momentum is unchanged. In fact, the second active impulse does positive work because it acts at a contact with positive pre-impact separation velocity while the first frozen bounce target is retained.

The same accumulated scalar normal iteration as the exported solver, compiled without GLM, increases energy by 2.820356455 J after ten iterations. More iterations approach the larger exact-LCP increase; this is not remedied by converging harder.

IMPORTANT: this is an exact mathematical/source-style witness and an executed extracted scalar program, NOT a measured current PhysicsWorld result. The supplied full-engine probe must be run against 422fd27 before filing a production finding. Cache-optimized final ContactSolver code was not available here, though the supplied pass report says its numerical law was unchanged.

The multiple-impact literature and Simbody's documentation explicitly warn that normal-velocity restitution can add energy in coupled simultaneous impacts. Do not treat single-contact Newton restitution as a universal simultaneous-impact theorem.

## 7. Experimental alternatives and honest failures

I did not silently change the source-style law to make all research tests green.

### A. Compression, common restitution, reprojection

For frictionless homogeneous constraints C={v:Jv>=0}, positive mass metric M:

    vc = projection_C^M(v0)
    vr = vc + e*(vc-v0)
    v1 = projection_C^M(vr)

Cone projection gives vc orthogonal to v0-vc in M. Thus

    ||vr||_M^2 = ||vc||_M^2 + e^2 ||v0-vc||_M^2 <= ||v0||_M^2.

The final projection cannot increase that norm. Impulses remain nonnegative; pair reactions conserve linear momentum. This is a derived research candidate, not a faithful replacement for differing per-contact restitution/friction.

In 180 random exact head-on scenes:
- source-style coupled targets produced one energy-increasing witness;
- the candidate did not increase energy;
- without low-speed capture, four candidate scenes reached the 128-event cap and remain explicitly incomplete.

A second run with capture speed 0.5 (the existing source's numerical scale) completed all 180 scenes for each model. The source-style energy counterexample remained. The candidate's event-wide choice of common restitution is not identical to production's per-contact policy; this is not an approved replacement.

### B. Closing/resting-first induced-impact sequencing

A second candidate retains per-contact restitution but excludes currently opening rows during a fast impact. Contacts made closing by the impulse are handled again at the SAME time. For a block with initial u<=0, nonnegative lambda, and post target -e*u, the exact impulse energy is

    DeltaK = 1/2 sum lambda_i*(1-e_i)*u_i <= 0.

In 360 varied-mass/per-contact-coefficient scenes, 357 completed, with exact momentum and non-increasing energy; three hit the 128-event budget. Completing cases also passed row reversal, step partition and Galilean-boost comparisons. A low-speed all-touching compression attempt did not remove those three failures. Their metadata and the pre-change implementation are preserved.

These models demonstrate why neither energy clipping nor another guessed target velocity is required for the individual normal blocks. They do NOT settle a general real-time 3D frictional impact model. Inelastic collapse/induced impacts need a declared contact-cluster/time-resolution treatment rather than throwing remaining time away.

## 8. Force, cache and support discipline

- The existing caller applies gravity before PhysicsWorld::Step and velocity integration consumes force/torque accumulators once. Applying that full kick at each TOI is a multiple-force bug.
- Warm-start impulses are a numerical initial guess, not a physical event to replay whenever the queue changes. Their accumulated multiplier and the already-applied velocity change must be accounted for together.
- Persistent support contacts are not fresh restitution events each substep. Conversely, a body that departs and physically strikes again has a new impact.
- Rebuild/revalidate events for every body whose velocity, angular velocity or trajectory changes through contact. Disconnected bodies must not acquire altered trajectories because unrelated objects collide elsewhere.
- State-version and body-generation changes invalidate scheduled events. A stale event must not apply to a reused body slot.
- Preserve material friction/restitution semantics unless a separately specified correction is approved.

## 9. Recommended next engineering boundary

This report does not authorize production edits. Before a full FTFT4B handoff:
1. Run the compact energy witness through actual current PhysicsWorld; do not infer pass/fail from the extraction.
2. Adopt a single constant-velocity drift contract (recommended: exponential angular drift) and its verified no-unrelated-event dependency checks.
3. Use chronological impact events plus a piecewise motion ledger and post-impact candidate invalidation.
4. Specify contact-island treatment, near-zero-time progress/capture, and supported rotational TOI behavior explicitly.
5. Preserve existing FTFT4A exact geometry, signed gaps, protected suites and the performance debt.

The 13 open temporal fixtures remain acceptance inputs. No new broadphase or friction rewrite is justified merely by this report. A simple scalar target patch is ruled out. Production benchmarks, general 3D contact clusters, terrain/player approximate sweeps, and event-driven force/time integration must still be validated on the engine.

## 10. Reproduce and result classification

    python code/run_all.py

Requirements: Python, numpy, mpmath, a C++17 g++ compiler. The runner builds only the extracted scalar probe in temporary disposable storage. All research source/evidence is persistent in this package.

Some result files deliberately have INCOMPLETE or COUNTEREXAMPLE status. The runner succeeding means these research findings reproduced, not that every candidate passed or that FTFT4B is complete. Exact source-style failures were preserved rather than clamped or deleted.

Sources and scope of their use are in SOURCES.md. Source availability and hash comparisons are in results/source_scope.json. No production files, milestones, commits, pushes, or fluid research were changed.
