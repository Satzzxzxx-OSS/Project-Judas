# FTFT4B — impact activation, friction work and event progress

## Decision

The reproduced three-body energy defect has a concrete frictionless repair:
resolve contacts that are closing/nonopening at the start of an impact round,
and recompute any newly induced closing contacts from the UPDATED state. Do not
freeze a future-impact response for a contact that is currently opening.

The previous three capped 1D cases are finite under the existing low-speed
capture rule; their 128-event exits were not evidence of infinite collision
accumulation. They need 272, 184 and 141 events respectively.

That normal-only result must not be promoted into an unqualified frictional
repair. I found an independent one-contact Newton/Coulomb energy counterexample,
and implemented/tested an energetic, evolving-slip alternative first in planar
closed-form intervals and then in a restricted spatial numerical reference.

A complete production 3D multi-contact friction/progress policy is NOT certified.
The source-specific timing/motion requirements from the prior research remain.
No production code was changed. This package is research, not a complete Astra
implementation handoff.

## 1. Provenance and scope

Latest reported pushed checkpoint:
`c43af4c0b65c597bff7fd03bed55173f5c483139` (diagnostic-only).
Actual inspected production bytes:
`422fd275c5f8b54ad4ff37d851eab24a36e8e089`.
All 152 manifest entries in the supplied current-engine review ZIP verified.
Selected source and user-executed results are under `reference/`.

GLM is not installed here. I did NOT recompile or execute the complete current
ContactSolver/PhysicsWorld. The full-engine energy/timing numbers belong to the
operator's verified baseline. I executed independent rational/NumPy/SciPy
calculations and a C++ scalar specialization of its normal/tangent operations.

The new optional `production_single_contact_witness.cpp` calls the real solver,
but it is explicitly unrun and is not counted as evidence that the new oblique
case already fails the compiled current engine. No more source export is needed.

## 2. The frictionless normal rule and its energy identity

At an isolated impact time, fix positions and orientation for impulse resolution.
Let M be the generalized mass/inertia matrix and J the normal-contact Jacobian.
Use only touching contacts whose current normal relative velocity is nonpositive:

    u = J v_minus <= 0
    G = J M^-1 J^T

With per-contact coefficients 0 <= e_i <= 1, solve:

    lambda >= 0
    r = G lambda + (I+E) u >= 0
    lambda^T r = 0

Then:

    v_plus = v_minus + M^-1 J^T lambda

The exact kinetic-energy change is:

    Delta K = u^T lambda + 0.5 lambda^T G lambda
            = 0.5 sum_i lambda_i (1-e_i) u_i <= 0.

For an approximate solve there is the additional term 0.5*lambda^T*r. A finite
iteration count does not establish complementarity or the energy bound by itself.

If a previously opening, touching contact becomes closing, process it in a new
impact round using the updated incoming velocity. Do not substitute its old
separating velocity or a stale zero target. The exact 1D reference also has an explicit LOW-SPEED SUPPORT branch: if no
contact in a connected touching group closes faster than the existing 0.5 m/s
capture speed, it projects all that group's touching normal constraints with
zero restitution targets, including opening rows. This is a mass-metric
compression/support projection, whose energy change is -0.5*lambda^T*G*lambda
at an exact solve; it does not need the u<=0 assumption. The fast-impact branch
uses only nonopening rows and its per-row effective restitution. Neither branch
changes coefficients after an iteration-count limit. Positive geometric gaps still
require time of impact before physical restitution is applied.

This identity is our derivation. It assumes normal-only impulses, nonopening
selected rows, nonnegative restitution coefficients <=1, and a solved LCP.
It is NOT a proof for the existing tangential-friction loop.

### Current three-body witness

Masses [10,1/8,10], initial velocities [1,-3,4], friction zero and e=1.
The exact closing-first reference performs two rounds at the same physical time:

    after contact 0-1: [73/81, 397/81, 4]
    after induced 1-2: [73/81, 20477/6561, 26390/6561]

Final velocities are approximately:

    [0.901234567901, 3.121018137479, 4.022252705380] m/s.

Initial and final energy are exactly 1369/16 = 85.5625 J. Momentum is exact.
This is one explicitly defined rigid-impact outcome, not the unique continuum
material answer. The user's current engine ends at about 88.3829 J instead.

### Executed normal checks

- Exact 1D family: all 360 prior fixtures complete at research event budget 1024.
  Original material restitution combinations and 0.5 m/s capture were unchanged.
  Base cases use 2,722 events total. Row reversal, timestep subdivision and common
  velocity/position offsets reproduce their exact results.
- Three former 128-event exits finish at 272,184,141 events. Increasing the
  RESEARCH budget diagnoses those cases; it is NOT a production event-budget fix.
- 600 independent fixed-geometry 3D normal blocks, 2–5 bodies, masses 1e-2..1e4,
  general positive inertia, shared-point torque arms; 2,400 evaluations including
  row reversal, rigid rotation and common velocity boosts.
- Worst normalized energy-identity error 3.28e-16; linear momentum 1.50e-16;
  angular momentum 2.53e-16; normal residual 2.18e-13.
- 33 normal blocks create another closing row. They are explicitly not counted
  as fully resolved 3D clusters. The point is to test the block operation and
  expose the need for induced-impact handling.

The 3D tests supply valid algebraic contact Jacobians; they do not exercise
shape generation, broadphase or actual 3D TOI.

## 3. Event progress: one corrected conclusion and one genuine pathology

The older report's three unsuccessful 128-event runs were finite, expensive
sequences. Calling them an established infinite loop would have been wrong.

Turning off the pre-existing capture law in one fixture reaches a new 512-event
research cap with time left unconsumed. This result is also NOT an infinity proof.
It remains an explicitly incomplete run.

A separate analytical example does demonstrate accumulation: a ball bouncing
under gravity with constant e in (0,1) has infinitely many flight intervals whose
sum after the first impact is:

    T = 2*e*v0 / [g*(1-e)].

With v0=2, g=9.81, e=0.5, T=0.407747196738... seconds. Continuing a simulation
past that limit requires a sustained-contact continuation; enumerating more
impacts alone is not a definition of the later state. This ideal law differs
from Judas's current velocity-dependent capture treatment.

Primary sources S2, S3, S5 and S6 distinguish impact resolution, persistent
support and induced-impact policy. None authorizes us to discard the remaining
step, arbitrarily turn e off after a count, or claim every capped run is physical
inelastic collapse.

## 4. Why friction needs more than the normal-only fix

Consider one planar oblique contact with mass 1, Izz=2/3, lever arm (1,-1),
normal +y, tangent +x, initial body velocity (-0.1,-1), and zero spin.
Its contact mobility (normal,tangent order) is:

    A = [[2.5,1.5], [1.5,2.5]].

Use friction mu=0.8 and restitution e=1. The closing speed exceeds Judas's
0.5 threshold, so this witness is not a low-speed convention mismatch.

The current-style accumulated normal-target/tangent-cone iteration gives:

    energy before = 0.505 J
    after 10 float iterations = 0.539333097256 J
    converged ideal algebra = 0.539375 J.

This is a SCALAR SPECIALIZATION, compiled here in C++17 with floating-point
contraction disabled. It is not a new full-engine execution. The optional
real-ContactSolver adapter preserves the setup for eventual regression use.

The normal/tangential mobility is coupled. Friction can change the normal
velocity; restoring a fixed Newton rebound target may then add more energy.
A final impulse lying inside a Coulomb disc is insufficient to establish the
sign of the work performed during the whole impact. Slip direction can change
while the impulse accumulates. This is consistent with the mechanisms discussed
in S2 and the author description in S4.

In the random planar family, 72/2400 generic Newton/Coulomb comparisons gain
energy. Those comparisons intentionally use the specified coefficient without
Judas's low-speed capture and are NOT 72 demonstrated production defects.

## 5. Planar energetic reference

At a single contact let p=(p_n,p_t), u=u0+A p. Advance normal impulse
monotonically, following Coulomb friction against the evolving slip:

    dp_t/dp_n = -mu*sign(u_t)                 (sliding)
    dp_t/dp_n = -A_tn/A_tt                   (sticking, if inside cone).

Slip-zero and compression-end events are solved explicitly. During compression:

    Ec = -integral(u_n dp_n).

During release, return e^2 Ec in normal work, while tracking tangential work:

    Wt = integral(u_t dp_t) <= 0.

Therefore:

    Delta K = -(1-e^2) Ec + Wt <= 0.

No global energy scaling or post-impact velocity clamp is used. This is an
energetic interpretation of restitution. It agrees with the usual normal speed
ratio for a frictionless isolated hit, but is NOT generally identical to that
ratio when friction and rotation couple.

For the oblique witness:

    final normal point speed = 0.941457836094 m/s
    final tangential point speed ~ 0
    final energy = 0.503857142857 J
    friction loss = 0.001142857143 J.

Demanding both this energy interpretation and an exact final normal speed of
1 m/s would impose two different constitutive prescriptions. We must declare
that decision, not quietly reinterpret existing material settings.

The piecewise planar implementation passed 2400 cases and their tangential
reflections. Maximum five physical intervals per case; normalized work identity
error <=1.28e-15. This is not general 3D or multipoint friction.

## 6. Spatial single-contact extension and an important rejected attempt

I extended the energetic idea to a 3D single contact with two tangential
components. Partition its SPD mobility:

    A = [[a, b^T], [b, C]].

For a trial positive normal-impulse increment q, compute tangent impulse r from
an implicit Coulomb cone problem:

    minimize 0.5*r^T*C*r + (u_t+q*b)^T*r
    subject to |r| <= mu*q.

Use the actual impulse (q,r) and evaluate exact work along the corresponding
linear velocity segment. Require friction power to be nonpositive at BOTH
segment endpoints. It is then nonpositive all along that linear segment.
Reduce the step if this test or the step-doubling error check fails. Stop phases
at the normal-compression zero and energetic-return budget, rechecking local
error on an event-shortened step.

This is OUR adaptive backward-cone discretization, not an assertion that PLUS
uses these exact equations. Accepted increments preserve the impulse equation;
normal/tangential work is calculated from their midpoint velocities, not from a
posterior energy adjustment. Tolerances concern numerical integration and root
finding; they do not change material e or friction coefficients.

### Executed spatial evidence

- 48 spatial single-contact cases plus 48 rotated tangent-basis equivalents.
- 12 planar cases compared to the analytic interval solver at tolerances
  1e-4/1e-5/1e-6 (36 comparisons).
- Worst planar velocity errors: 1.99365e-4 -> 7.67656e-5 -> 9.99042e-7.
- Largest tangent-basis covariance error: 2.62e-14.
- Largest normalized work-identity error: 1.08e-15.
- A separate continuous Routh ODE is integrated by SciPy DOP853 in 12 constructed
  strictly-sliding 3D cases. The same three tolerances give maximum velocity
  errors 2.87088e-5 -> 1.41867e-5 -> 4.91962e-6. This is an independent numerical
  integration reference, NOT an exact analytical oracle.

The spatial reference required up to 2812 small cone solves including rejection,
step-doubling and root searches. That is research verification work, not an
acceptable production cost assertion. It is intended to help select and validate
an efficient piecewise impact implementation, not run at every resting contact.

### Rejected midpoint variant

An earlier midpoint-cone candidate satisfied its energy accounting but missed
slip-transition behaviour. Its worst planar error stayed ~0.0079147 while the
requested tolerance tightened; one of 48 spatial cases exhausted its work cap.
Rechecking shortened event steps improved some cases but did not remove that
failure. Both versions and outputs are preserved under `evidence/`.

This is a useful falsification: energy closure alone does not validate the
intended friction law or its numerical convergence. The later backward-cone
candidate replaces it explicitly; it is not a retuned tolerance.

## 7. Small coupled spatial experiment

I composed the energetic point operation on 12 small fixed-geometry graphs with
2–4 bodies. Initial velocities were manufactured so every declared normal
contact was approaching, rather than allowing empty/open graphs to inflate the
pass count. Material friction and restitution vary between contacts.

All 12 completed within the declared normal-velocity tolerance; 21 contact events
in total, maximum four in one case. Energy did not increase. Normalized linear
and angular momentum residuals were 1.19e-16 and 1.26e-16.

This sequential experiment explicitly selects the largest normal impact-energy
estimate next. That is a model choice, NOT a unique physical simultaneity oracle.
It does not prove symmetric tie handling, large/redundant manifolds, long event
chains, sustained support or universal termination. The earlier random open-graph
version is preserved separately and is not counted in those 12 approaching cases.

## 8. What the papers do and do not settle

A rigid model generally lacks the detailed compliance which determines the
sequence and relative strength of simultaneous contact forces. S3 demonstrates
this and intentionally models a set of outcomes; its termination guarantee is
for purely inelastic dynamics. It must not be borrowed for arbitrary restitution.
S7 adds relative contact stiffness and energetic contact states for a different
class of multiple-impact model. These are deliberate physical/numerical models,
not evidence that body names or fixture-derived impulse routing are needed.

S2 provides a useful architecture for compression/expansion, induced contacts
and changing slip. Its authors separate the impact handler from collision
localization and persistent-contact handling. S8's global energetic interpretation
is also not automatic authorization to replace every material in a contact island
with one arbitrary coefficient.

The engine must select and document a constitutive model appropriate to its
scope. Conservation is a necessary invariant, not a complete uniqueness rule.

## 9. Source-specific next repair boundary

The current source offers a clear separation:

- `ContactSolver::Prepare`: currently fixes all normal targets before iteration.
- `SolveVelocities`: repeatedly restores those targets and projects total friction.
- `PhysicsWorld::Step`: scans once and drifts the entire remaining interval with
  already-solved velocities.
- `RigidBody::IntegrateRigidBodyPosition`: normalized Euler is partition-dependent.

A repaired design needs:
1. true contact time before restitution;
2. current-state closing/resting impact rounds, with induced-contact updates;
3. an impact friction/restitution law with explicit impulse-work accounting;
4. consistent rotation and piecewise motion records;
5. a declared persistent-contact/progress policy, not 'give up and drop time'.

Do not replace the successful geometry, tree or normal resting-contact solver
merely because a different impact phase is required. Warm starting is a numerical
initial guess, not physical bounce energy to replay at every zero-time event.

READY AS REFERENCE EVIDENCE:
- frictionless activation rule, witness results and energy identity;
- finite progress of the previously capped cases under their existing law;
- energetic one-point friction model, planar exact and spatial convergence checks;
- scalar oblique counterexample and unrun real-solver adapter.

NOT READY AS A COMPLETE PRODUCTION HANDOFF:
- 3D simultaneous frictional group load distribution/symmetry with mixed materials;
- robust and affordable persistent-contact/event-progress integration;
- full support/friction/restitution regressions and production performance.

The next research should be a coupled frictional group with redundant contacts
and transition into sustained support, using these single-contact/work oracles.
It should not be another baseline audit or an attempt to rescue the rejected
speculative target formula. A more limited kinematic/motion-ledger implementation
can be specified separately if production progress is wanted while that last
impact-model seam is resolved; this package does not authorize it.

## 10. Reproduction and bookkeeping

Run `python code/run_all.py`. Add `--replay-rejected` to reproduce both rejected
midpoint variants in an isolated build tree. All candidate raw results are retained;
positive research checks and intentionally incomplete cases have distinct status.

The default run executes five reference programs plus the compiled scalar probe.
`coupled_spatial_experiment.py` is also included in the final runner. Source hashes,
versions, commands and runtimes are recorded. Actual engine tests, the optional
production adapter, production benchmarks and any fluid work were NOT run.

Two attempts to launch the entire optional extended suite hit the execution
tool's timeout; no physical result was accepted from that interrupted
invocation. Individual rejected replays and the complete default suite ran to
completion without changing their numerical definitions. This is execution
bookkeeping, not evidence of an impact-model failure.
