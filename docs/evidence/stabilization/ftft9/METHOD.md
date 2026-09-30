# FTFT9 production approximation

This is production hybrid coupling, not the protected R1 pressure/transport research.
The active candidate is described below. Current execution is recorded in
completion-candidate/; earlier rejected candidates remain linked and unchanged.

## Ownership and state

`StepPlayedWorld` supplies the ordinary scene gravity kick once.
`ProductionFluidCoupling::PrepareRigidStep` applies hydrostatic/drag forces before
the ordinary authoritative `PhysicsWorld::Step`. `AdvanceResolvedLiquid` then
advances particles at the authored cadence (default 30 Hz) using copies of the
actual resolved rigid motion segments. The custom player samples the resulting
field before its ordinary controller update. No force, gravity kick or rigid
pose integration is replayed by the fluid stage.
A fluid execution consumes all accumulated time; held frames predict sampled fluid
velocity from the last measured acceleration and its actual age. Field positions
are held for presentation. The fluid collider history spans fluid executions,
not merely adjacent rigid ticks. Reset/slot-generation handling uses live physics
handles; no cache is serialized.

Rigid quadrature is deterministic and shape-local: equal box subvolumes, antipodal
sphere samples, and a nonoverlapping union partition for compound boxes. No mass
enters the displaced-volume estimator. The resulting force is
`-rho * submergedVolume * (gravity - measuredFluidAcceleration)`, integrated over
the samples with torque about the same rigid centre. The latest executed particle
velocity difference supplies acceleration; there is no long temporal low-pass.
Bulk velocity and acceleration use a continuous compact spatial weight
`particleVolume * (1 - distanceSquared/reachSquared)^2` inside the geometric reach.
This avoids a discontinuous acceleration jump when a new particle row only just
intersects a volume sample. Occupancy and bulk motion both extend the same
geometrically unoccluded exterior columns. A solid's displaced/contact shadow
cannot feed its own one-way wall motion back as analytic hydrostatic support.
There is no velocity classifier or temporal filter. Surface reconstruction and
bulk averaging have distinct jobs; neither is an independent pressure-support force.
Their finite supports differ: occupancy is cylindrical and resolved bulk motion
is spherical. When a wet envelope has no spherical donors, actual admitted-column
velocities/accelerations provide a positive-weight extension. Existing resolved
samples retain their original spherical weights. Uniform vectors are preserved
in this missing-data case; nonuniform continuity at the seam is not guaranteed.
A broader replacement of all spherical weights worsened coarse-body immersion
and was reverted. Both executions are preserved in
[support-consistent-motion](support-consistent-motion/README.md).

For hydrostatic occupancy, each locally resolved particle column spans its lowest
to highest liquid interval within the finite query window. This **filled bulk
column approximation** avoids inventing internal air gaps from disordered particle
rows. Column fractions are averaged, rather than taking the tallest column over
the whole body. Exact solid geometry excludes columns occluded by the sampled
body before extending the exterior field into its displaced volume. This is not
a reconstruction of small trapped bubbles or closely separated sheets: internal
gaps within the local window can be filled. Finite search reach, particle order
and quadrature resolution remain explicit approximation limits. Both this scope
and the failed literal-interval candidate are preserved in the component evidence.

Drag is a configurable linear relaxation. The point effective mass includes rigid
inertia. The relaxation factor `1-exp(-rate*immersion*dt)` follows the known
hydrostatic kick, so neutral hydrostatic cancellation is not damped twice. This is
not a full hydrodynamic drag law. Exterior particle collision displaces the liquid
but contributes no rigid reaction: exterior fluid/body momentum is deliberately
not conserved.

## Declared cavities and contact load

A `fluidCavities` entry is an authored body-local box describing the resolved
interior of an ordinary collision shape, not an inferred “held” fluid category.
Resolved walls are still required. Own cavity particles are excluded from the
owner's exterior hydrostatics using the body pose at the **same fluid timestamp**.
They remain visible to other bodies and the player.

PBF solid position projection does not independently create a velocity impulse.
All distinct contacted solid normals survive the position iterations. Exact-touch
inward sweeps retain their entry plane at time zero; outward and tangent motion
remain free. Once per fluid substep, contained particle velocity contacts and the
owner's currently touching rigid-contact island share the existing accumulated
impulse solver, with zero restitution. Temporary particle point masses have zero
rotational inertia. The actual rigid masses/inertias and authored rigid-contact
friction participate. The island expands through dynamic bodies only; a static
floor is an external boundary, not a connection to every body standing on it.
Zero-gap/overlap geometry joins this support solve. A separated row is admitted
only when its live generation, primitive pair, normal and nearest unused local
anchor match a contact already held in the main solver's cache. It reuses the
existing target for the upcoming rigid timestep. New separated contacts do not
join this auxiliary solve. This preserves the main solver's small support gaps
instead of alternately dropping and restoring a resting container's support.

The dedicated solve starts with the existing ten-iteration accumulated-impulse
solve and permits up to 64 PGS iterations. When necessary, a matrix-free normal
Schur-complement solve accelerates convergence of those same rows, with the same
nonnegative accumulated impulses and friction projection. It does not change the
collision law or main solver. Redundant normal rows may end the Krylov solve early;
PGS still checks the physical residual. Iterations, matrix products, acceleration
attempts, declines and caps are observable across every executed fluid substep.
See [the exact auxiliary method and independent budgets](particle-batch/METHOD.md).
Optional normal acceleration is now declined for active moving prescribed normal
boundaries: nearly opposed coarse walls can otherwise admit enormous constraint
work. The unchanged bounded PGS still processes every row and reports unresolved
caps. Eligible tentative corrections must also pass actual finite-state,
impulse-cone and closing-residual checks, or are rolled back exactly. This guard
prevents the reproduced accelerator explosion; it does not make conflicting wall
constraints feasible. See [before/after witnesses](particle-batch/acceleration-safety/README.md).

The dedicated solve changes velocities only. It does not reapply gravity, advance
poses, change the main rigid warm cache, or change the rigid impact policy. Each
particle constraint's accumulated impulse supplies the equal-and-opposite
contained-load diagnostic. Static support impulse is reported separately. Exterior
contacts use prescribed boundary velocity and never become a second rigid support
mechanism. This finite-iteration contact exchange is not an incompressible pressure
solve or a proof of conservation for the complete PBF/hybrid model.

### Coarse particle clearances: endpoint escape rejected

**REJECTED AFTER INTEGRATED REFERENCE-POOL CONTAINMENT FAILURE.** The two
bounded box endpoint-exclusion candidates passed 12/60 and 14/78 isolated
boundary checks. They enumerated encountered box face/edge/corner projections,
validated candidate endpoints against prepared solids, and later added a
stationary thin-wall path barrier. These were explicit positional approximations,
not conservative continuum remapping.

The combined dense .20 m reference-pool snapshot nevertheless contained two
particles below its floor at y=-71.3106 m and -69.5166 m, with vertical velocities
-37.3302 and -36.8334 m/s. Its measured interval reported 143 unresolved geometry
events and zero auxiliary contact caps. Preserving all 1,152 particle identities
and their mass did not preserve spatial containment. This was discovered by
reviewing the raw particle CSV; an existing count/mass assertion did not catch it.
The separate dynamic cup/tank family passed eight cases, so this failure must not
be described as failed cup load or cup retention evidence.

The candidate collision/escape code and its six unaccepted boundary fixtures were
restored after that failed integrated review. Accepted eight-case/24-check
boundary assertions remain intact. The rejected source, focused PASS results,
raw failure and rollback manifest are [preserved here](particle-batch/geometric-escape/rejected-integrated/README.md).
At that rollback checkpoint the earlier sequential solid projection was
restored, leaving coarse-solid clearances open. Its retained diagnostic fields
emitted zero and established no endpoint-exclusion capability. This historical
state is superseded by the specifically authorized repair below.
Rollback itself reran no experiments. The subsequent limited check built the
restored Release source with zero warnings, passed Project and the original
eight-case/24-check boundary checks, and passed one shipped classic/terrain
performance pair (360 rigid ticks and 180 fluid executions each). Particle
medians were 3.176545/3.642064 ms; amortized hybrid costs were 2.314343/1.917584
ms per rigid frame. The complete integrated gate was not rerun after rollback,
so these targeted results do not close FTFT9. No third geometric policy is
implied by this disposition.

## Active hard-solid / soft-clearance repair

Particle centres are tested against radius-zero physical solid geometry. The
particle collision radius remains a preferred numerical clearance. Original
clearance arithmetic is retained whenever feasible. A proposed clearance that
enters another solid is shortened at that union boundary; a blocked preference
cannot be undone by a later incompatible soft projection. Zero available
clearance means zero soft correction, not particle removal.

Rare initial/intermediate physical overlap uses actual box face/edge/corner
candidates, validates the complete union, and preserves stationary-wall path
barriers. Representable surface rounding is bounded and counted. An unresolved
hard exclusion throws an explicit error. All positional corrections are excluded
from reconstructed physical velocity. This is a finite geometric repair, not
conservative continuum transport or an exact continuous swept-union solution.
Terrain path checks are sampled; pathological overlap can still fail explicitly.

Actual rigid segments are copied by value with full handle generations. Storage
is cleared after each fluid execution and spans only the configured cadence.
ResetBody/teleport discontinuities do not become wall trajectories. Static solids
without a continuous ledger retain their original constant-pose arithmetic.
Cavity tests and fluid collision consume the same copied body history; compound
child bounds include their rotational offset. No unconstrained rigid endpoint
prediction supplies a fluid wall.

The unchanged closing-gap witness, unchanged coarse half-density case (731 checks)
and legacy fluid/rigid coupling regression pass in
[boundary-repair/fourth-gate](boundary-repair/fourth-gate/results.json).
That local gate alone does not establish complete FTFT9 acceptance.

## Numerical dissipation and limits

The prior XSPH neighbour-only denominator changed momentum in an unequal-density
neighbourhood. The repaired weight uses `max(rho_i,rho_j)` on both sides. Thus
`m_i*deltaV_i = -m_j*deltaV_j` for each pair. For smoothing coefficient in `[0,1]`,
the off-diagonal row sum is at most that coefficient, since each denominator is
at least the receiving particle's own density. The simultaneous update is a
mass-weighted conservative convex average and cannot increase kinetic energy in
exact arithmetic. Invalid coefficients are rejected rather than clamped. Smoothing
occurs before the final wall-velocity projection; doing it afterwards demonstrably
reintroduced inward velocity at a stationary wall.

PBF density-position relaxation and finite spatial/temporal resolution remain
approximate. Particle micro-agitation is not the same measurement as net pool
motion; both are recorded. Initial lattice relaxation is reported separately from
a prepared-pool equilibrium experiment. A held 30 Hz particle position must not
be compared with a later 60 Hz rigid position and called a physical drift.

The legacy Poly6-density/Spiky-gradient convention is retained. Replacing it with
the exact Poly6 derivative passed independently differentiated pair checks but
increased the unchanged .20 m pool mean particle speed from .0833565 to 1.07689
m/s; that candidate was restored immediately. It was a numerical-model
consistency experiment, not proof that the conventional surrogate was a coding
error. Its source and failing results are [preserved](rest-exact-gradient-attempt1/README.md).

The second bounded settling investigation was passive. Across eleven measured
body cases, the maximum subtraction-versus-represented-PBF-increment velocity
residue was 1.67638e-6 m/s and maximum per-step RMS residue 3.67076e-8 m/s. The
neutral .25 m case measured 2.61934e-9 m/s maximum residue while its preparation
particle mean speed was .0727231 m/s. Zero-gravity residue was exactly zero while
its preparation mean speed was .0972153 m/s. These diagnostics cover the measured
post-release interval, not the preceding warmup itself. They disprove cancellation
as an explanation for material agitation in this continuing pool operation;
the velocity formula is unchanged. [Per-case observations and hashes](rest-cancellation-diagnostic/combined-family-analysis.json)
remain available. Strict .05 m/s preparation failures are still failures. After
two bounded investigations there is no demonstrated correction supporting
another autonomous pressure-model or tuning attempt. Neither solver caps nor
local particle agitation are silently reclassified as convergence.

The body reference pool is shared across density/spacing cases and is sufficiently
deep for an independently measured fully submerged release. Particle mass divided
by nominal rest density is not used to assert a PBF surface height: missing
boundary-kernel support changes its packing. All original motion tolerances and
the original prepared-pool speed predicate remain unchanged. Individual-particle
micro-rest and equilibrium of a body smaller than one particle spacing are
additional diagnostics, separate from the operator's behavioural gate. Their raw
failures are reported; `--strict-diagnostics` restores failure exit status for
those checks. Bulk stationarity, containment, density ordering and the original
mandatory motion tolerances remain acceptance requirements.

The player remains a custom controller. Its bounding-box quadrature approximates
capsule immersion; density, drag, and bounded swim acceleration are authored
parameters. It explicitly uses `NonDisplacingVolume` sampling: unlike a rigid
solid, the query-only controller does not exclude interior particle columns.
Using rigid shadow exclusion there was a demonstrated sampling defect; the
unchanged rotated free-fall endpoint gate passes after the correction.
Particle contacts never push the player. No exact player/fluid momentum or
energy conservation is claimed. See
[non-displacing-player](non-displacing-player/README.md).

## Persistence and protected work

Authored cavity, player, and cadence settings participate in canonical scene
fingerprint schema 2. Scene syntax remains version 3 and accepts absent new fields
using declared defaults. World saves remain format 2; older canonical-schema saves
are rejected with an error, not silently migrated. FTFT1 validation/atomicity and
historical evidence are preserved. P1-C/P1-C-M/P1-PF are untouched research checkpoints.
