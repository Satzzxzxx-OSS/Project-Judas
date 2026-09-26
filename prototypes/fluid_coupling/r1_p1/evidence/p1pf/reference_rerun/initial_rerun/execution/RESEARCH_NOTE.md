# Judas R1: frozen-geometry coupled pressure/body projection

## Status

Independent research and executable reference checks, written outside the Judas
repository. This does not change or rerun production Judas, does not certify the
agent's C++ implementation, and does not complete P1-D/E or R1.

**Ready to specify as a bounded component:** multi-cell incompressibility and rigid
velocity coupling on resolved, frozen Cartesian geometry, using actual fluid
staggered masses, boundary half-duals, explicit normal traction unknowns and one
mass-weighted projection.

**Not yet specified here:** arbitrary cut-cell geometry, moving-body geometry
transport, topology changes, tiny-cut-cell stabilization, the full advection/body
pose time integrator, 3D, swimming, or a production performance budget.

The chosen component uses actual unions of grid cells as its authored shapes.
It does NOT voxelize an arbitrary shape and claim its true boundary was retained.
Rigidly rotating the entire grid and geometry is tested; that is not a test of an
oblique thin solid cutting through an otherwise fixed grid.

## 1. Research decision and lineage

Source [1] supplies the variational pressure/kinetic-energy interpretation. Source
[2], especially Eqs.8-12, supplies the more useful explicit interface-impulse
formulation for this reference: fluid boundary velocities carry mass, normal
velocity constraints transfer impulses, and structure interpolation has a transpose
reaction. The impulse application point matters for angular momentum.

The code below is our original 2D, inviscid, frozen-geometry specialization of those
principles. It is not copied author code and is not the complete time integrator of
[1], [2], or the van der Eijk-Wellens paper [3]. In particular, it does not silently
import [3]'s Crank-Nicolson old/new-pressure body update into a first-order projection.

Both liquid and gas have positive prescribed density, consistent with the P1-C-M
state model. No internal liquid/gas face is assigned p=0 merely because gas is
present. A prescribed-pressure OUTER boundary is separate from the internal phase
interface. Surface tension, viscosity, compressibility and cavitation are absent.

The next implementation should retain this declared component scope and the already
passing P1-C/P1-C-M algorithm. It must not claim arbitrary cut cells from this proof.

## 2. Why boundary fluid mass cannot be discarded

Let the MAC mesh spacing be H and the fine phase/mass mesh spacing be H/2. The mass
of a component velocity is the sum over the fine cells in its dual volume, excluding
solid/outside portions. For a grid-aligned fluid/solid boundary the face velocity has
an actual fluid HALF-dual volume. Keep it as a velocity unknown with that mass.

At a body interface it is tempting to delete this fluid velocity, replace it with the
body normal velocity, and then forget its inertia. That is wrong. Exact elimination
would add R^T M_boundary R to the reduced body mass block and R^T M_boundary u_boundary*
to its generalized momentum. Keeping the fluid trace explicitly avoids losing those
terms. This is physical represented fluid inertia, not an empirical added-mass term.

Our actual wrong-mass negative control halves the interface half-dual masses and
changes the analytic piston velocity by about 4.38 percent.

For this fixed geometry, masses are frozen during projection. Pressure changes
momentum, not liquid fractions or masses. Component mass arrays each partition the
fluid mass separately; do not add x-dual and y-dual masses as if they were distinct
fluids.

## 3. Exact variables, units and signs

Use a 2D unit-depth slice. Body b has generalized velocity:

    q_b = (Vx, Vy, omega)

and generalized mass:

    M_b = diag(m_b, m_b, I_b).

Let:

    w = [u; q]
    H = block_diag(M_fluid, M_bodies)

where u contains all active scalar MAC face velocities, including fluid half-duals
at solid/domain boundaries. Use actual positive masses, never density/mass floors.

### Integrated cell divergence D

One row per fluid cell, with +face_area for its right/top faces and -face_area for
its left/bottom faces. The same face DOF is shared across neighboring fluid cells.
Do not divide rows by cell volumes in this reference. D u is integrated volume flux.

Pressure multiplier pi is a pressure impulse: pi=dt*p, not pressure alone.

### Normal interface rows

At boundary face f, let:
- A_f be its area (2D length times unit depth);
- e_f be the positive global direction of that MAC component;
- s_f be +/-1 for the outward normal from fluid;
- n_f=s_f e_f;
- r_f be face point minus body COM.

Define:

    S_f u = A_f s_f u_f
    J_f q = A_f [n_x, n_y, cross(r_f,n_f)] q_b.

The interface condition is:

    -S_f u + J_f q = 0.

It enforces NORMAL compatibility only. Tangential fluid velocity is not tied to a
body. The corresponding multiplier tau_f is the boundary pressure impulse.

The fluid impulse from this row is -A_f n_f tau_f. The body receives the opposite
linear impulse and the corresponding torque at THE SAME point. Cell pressure and
boundary traction are parts of one solve, not duplicate hydrostatic force channels.

For a prescribed wall, omit its dynamic body contribution and set:

    -S_f u = -A_f (n_f dot V_wall).

A support/guide can explicitly constrain a generalized body velocity component.
Those are mechanical constraints in the benchmark, not engine contacts or fluid
classifiers. Their multipliers are external generalized impulses.

A prescribed-pressure boundary has no normal-velocity constraint; add the known
external pressure impulse -dt*A_f*n_f*p_external to its fluid momentum before the
projection. The examples otherwise use p_external=0. Known-pressure work is distinct
from the homogeneous projection energy theorem.

### Assembled coupled operator

Without displaying optional support rows:

    B = [ D    0 ]
        [-S    J ]

    B w_new = b.

All dynamic bodies are included in the same matrix. No one-body force-after-fluid
path, mass cutoff, analytic buoyancy, or extra added-mass force is present.

## 4. Projection and energy identity

The desired correction minimizes:

    0.5 * (w-w*)^T H (w-w*)   subject to B w = b.

Our multiplier sign convention gives:

    H (w_new-w*) = B^T lambda
    A lambda = b - B w*,       A = B H^-1 B^T
    w_new = w* + H^-1 B^T lambda.

Ignoring support rows, the Schur matrix is explicitly:

    [ D M_f^-1 D^T             -D M_f^-1 S^T                  ]
    [-S M_f^-1 D^T    S M_f^-1 S^T + J M_b^-1 J^T             ].

This matrix is positive semidefinite for positive H. The full mixed KKT matrix is
indefinite; do not indiscriminately apply ordinary CG to that matrix. The reference
uses a small dense, row-equilibrated SVD of B H^-1/2 to identify its nullspace and
compute the minimum mass-norm correction. This is a correctness reference, not a
production linear-solver recommendation.

For delta=w_new-w*:

    K_new-K_star = lambda^T b - 0.5*delta^T H delta.

Thus homogeneous projection cannot add kinetic energy. Prescribed moving walls can
supply work through lambda^T b. This identity is checked independently of the force
update and does not prove full-timestep energy conservation.

SVD rank threshold in the code:

    64 * eps_double * max(matrix dimensions) * largest singular value.

The actual threshold, conditioning, residuals, and failure behavior are exposed. It
is a numerical rank choice, not a hidden physical mass cutoff or claimed universal
roundoff theorem. Constraint compatibility is checked after the solve; a material
incompatibility is rejected, not silently projected out.

## 5. Pressure gauges: the complete coupled operator decides

Two fluid chambers separated by a free piston are disconnected as fluid regions but
mechanically coupled by the piston. Their pressure DIFFERENCE is physical. Fixing
one arbitrary pressure separately in each chamber can remove that physical degree
of freedom.

For the sealed-piston test, the coupled matrix has one common pressure gauge, not
two. It recovers the 100 Pa difference needed to balance a 100 N force on a 1 m^2
piston. The deliberate wrong two-gauge solve leaks volume and moves the piston.

Nullspaces must be established for B^T (or the equivalent coupled Schur operator),
including support constraints and prescribed-pressure boundaries. Do not derive
pressure pinning solely from connected components of the fluid adjacency graph.

Likewise, a prescribed wall that compresses an entirely sealed incompressible domain
can produce an inconsistent constraint set. Returning a least-squares answer after
removing its incompatible RHS is not a valid conservation-preserving solution.
The negative control requires an explicit incompatibility error.

## 6. Geometric and momentum guarantees actually tested

For a full resolved cell, its oriented edge normals close. The moment of those edge
normals about any origin also closes. Cell-pressure impulses therefore cancel in the
GLOBAL discrete linear/angular budgets.

Interface impulses use the same point and normal on fluid and body. Prescribed-wall
and explicit-support impulses are included as external reactions. In a tank, fluid
plus body momentum alone is NOT conserved when the tank wall pushes on them.

The angular budget uses the lumped MAC momentum locations represented by the code.
It is not a proof that P1-C-M advection conserves continuum angular momentum for
arbitrary unresolved density distributions. The complete advection/projection/body
scheme needs that assessment separately.

Mass/inertia, support conditions and body geometry are test inputs. The algorithm
contains no cup/ship/floater branches. The cup is an ordinary U-shaped union of solid
cells; its inner and outer faces use the identical interface rows.

## 7. Executed reference evidence

Run:

    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python projection_checks.py

Dependencies: NumPy and SciPy only. No Shapely, GL, Judas build or network required.

The final run executes 42 named positive cases and five negative controls. Most use
12x12 MAC / 24x24 fine mass cells; piston cases use 12x6 / 24x12. The smooth pressure
refinement uses MAC 6,12,24. These are frozen-geometry projections, NOT moving-body
simulation timesteps. Extra idempotence projections run within some cases.

Families:
1. Random fields on closed/open all-fluid grids with varying positive density.
2. Sharp hydrostatic density jumps of 1, 1,000 and 1,000,000.
3. Fully submerged free and supported bodies of density ratios .001,.5,1,2,1000.
4. Partially immersed asymmetric body: independently known hydrostatic force/torque.
5. Supported resolved cup with contained liquid and surrounding positive-density gas.
6. Multi-cell open piston: six body masses from .01 to 1e6 kg.
7. Sealed piston: physically required differential pressure across two chambers.
8. Nonzero constant ambient pressure: no body force/torque or spurious velocity.
9. Coherent common free fall, including prescribed wall normal velocity.
10. Whole-grid rotation/translation, Galilean covariance with transformed wall data.
11. Normal-only free-slip channel.
12. Two dynamic bodies with different masses/inertias in the same solve.
13. Fixed-grid uniform-density hydrostatics under three non-axis-aligned/sideways
    gravity vectors; prescribed outer pressures match the analytical hydrostatic field.
14. Smooth analytical pressure convergence, using continuous derivatives to initialize
    provisional velocity (not the discrete matrix to manufacture an exact RHS).

### Independent physical oracles

Submerged supported rectangle:
- fluid density 1000 kg/m^3;
- volume (2/12)*(2/12)*1 m = 1/36 m^3;
- pressure force expected 272.5 N upward at g=9.81;
- recovered to about 1e-11 N, for each tested BODY mass;
- neutral free body receives essentially zero velocity, about 1e-16 m/s.

Asymmetric partially immersed L-shape:
- piecewise analytical area/centroid integrals predict force 340.76125 N upward;
- torque 9.72241071428571 N m;
- reference pressure reactions agree to rounding error.

Supported cup:
- liquid volume 1/12 m^3, solid volume 1/8 m^3;
- rho_l=1000, rho_g=1, body density=500;
- net FLUID force on cup:
      g*(rho_g*V_solid - (rho_l-rho_g)*V_water)
      = -815.45625 N;
- external support force including cup weight: 1428.58125 N;
- both recovered to rounding error.
This includes surrounding-gas pressure; it is not a claim the net fluid force should
be exactly the raw water weight in a two-phase bath.

Open piston:
- width 2/12 m, height 1 m;
- left liquid length 5/12 m at rho=1000;
- right gas length 5/12 m at rho=1;
- applied force 100 N over dt=.01 s;
- exact velocity:
      U = 1 / [m_body + 1000*(5/12) + 1*(5/12)].
The .01 kg body agrees to relative error about 4.57e-11; the 80 kg case to about
6.47e-16. The effective fluid inertia is produced by the solve, not inserted as an
analytic added-mass coefficient.

Smooth pressure refinement:
- analytic p=500*sin(pi*x)*sin(pi*y), zero prescribed boundary pressure;
- provisional u=dt*grad(p)/rho evaluated from continuous derivatives;
- MAC 6/12/24 pressure L1: 2.38750899 / .583185967 / .144955125 Pa.
This exhibits second-order pressure error for this smooth solid-free case only.
It does not turn donor-cell momentum advection into a second-order method.

### Negative controls

1. Halve interface half-dual masses: piston velocity is wrong by about 4.38 percent.
2. Single delayed explicit reaction estimate: .01 kg piston velocity becomes about
   -4.17e6 m/s instead of about .002398 m/s. This is an intentionally wrong algebraic
   control, NOT a reproduction or accusation about any particular old Judas source.
3. Pin pressures independently in both chambers: full constraints fail and the sealed
   piston moves at about .0003667 m/s instead of remaining stationary.
4. Prescribe incompatible sealed-wall flux: rejected rather than repaired.
5. Remove rotational reaction after an actual solve: angular budget error about
   .8602 kg m^2/s, detected.

The algebraic residual checks reuse the assembled operators and therefore primarily
verify algebra/linear-solver consistency. The piston, hydrostatic force/torque, cup
load and smooth analytic pressure cases are separate physical/analytic checks.
They must not be replaced with expected values generated from B H^-1 B^T.

## 8. What the next implementation may earn

A C++ component consuming frozen masses, provisional momenta/velocities, fluid cell
connectivity, boundary geometry and actual body mass/inertia; returning jointly
projected fluid/body velocities, pressure/traction impulses, and explicit residual
and reaction diagnostics.

Its fluid-only adapter can use actual P1-C-M masses and momenta unchanged. The body
fixtures initialize their resolved fluid masses from analytic fine-grid density
and exact masks. This research does NOT claim that the current advection routine
already transports mass through the body-containing domain.

The implementer must preserve all existing P1-C/P1-C-M tests, and run independent
component tests without silently altering the prescribed-advection tests to feed
projected velocity back into their analytical-oracle fixtures.

Do not chain moving-body velocity output into repeated frozen-geometry advection
and call it a body trajectory. The geometry/mass update must be resolved first.

## 9. Remaining research / explicit limits

Blocking for FULL moving-body integration, not this component:
- general relative-motion cut-cell phase/mass/momentum transport;
- covering/uncovering and the discrete geometric conservation law;
- small-cell stability and changing momentum DOFs;
- body pose / geometry / pressure time levels for the combined algorithm;
- pressure and momentum representations for arbitrary cut/thin-wall topology.

Not established by these checks:
- arbitrary cut-cell or subgrid-thin-wall support;
- neutral depth over a moving trajectory or final floating immersion;
- an oblique liquid/gas interface hydrostatic convergence study with moving solids;
- transport-plus-pressure angular-momentum or full energy theorem;
- actual 3D, 6DOF, swimming, viscous drag or real-time performance.

The framework is not permitted to replace existing mass with convenient rho*A*dx
in an arbitrary cut dual unless the two are proven consistent. Likewise a positive
semidefinite algebraic matrix alone does not prove spatial accuracy or physical
geometric conservation. Those are precisely why the next task stays resolved and
frozen instead of pretending arbitrary moving cut cells were solved.

Source [4] provides a topology-aware representation for thin obstacles, but its
published demonstration is one-way coupling with a different advection method.
It is not imported here or treated as proof of the missing integrated solver.
