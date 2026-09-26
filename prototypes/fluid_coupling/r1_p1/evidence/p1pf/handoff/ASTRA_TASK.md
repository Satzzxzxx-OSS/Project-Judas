# Project Judas — R1 / P1-PF
## Multi-cell coupled pressure/body-velocity projection on frozen resolved geometry

Implement the specified component. Research, equations, reference code and
independent physical checks are supplied. Do not start another method-selection
or literature-review loop.

## 1. Starting checkpoint and authorization

Reported current main:

    0d73f47206cc34674bb9fbbce2cb438f38ce9f22
    Checkpoint R1 frozen-WY JSGT fraction and mass-momentum transport

Production implementation remains the rigid checkpoint 48385a7. The new commit
records prototype work only. Verify actual git state before editing; preserve any
newer work rather than resetting it.

Existing prototype:

    prototypes/fluid_coupling/r1_p1/
    bash prototypes/fluid_coupling/r1_p1/run_p1cm.sh

That checkpoint reports 55 runs / 1,655 timesteps passing, with original P1-C
fraction CSVs unchanged. Rerun that command and capture a baseline first.

This task is P1-PF: a separate component gate. It does not rename, satisfy or
implicitly authorize the existing integrated P1-D/E gates.

Allowed: prototype C++ source, its build scripts, focused fixtures/oracles and
method/evidence documents. Keep the production engine, editor and root runtime
build unaffected. Use one reusable compiled projection implementation, not a
Python subprocess masquerading as a C++ implementation or a one-cell synthetic
pressure/body example.

Frozen geometry means geometry and masses do not change DURING the projection.
Dynamic body linear/angular velocities ARE unknowns in the coupled solve. They
must not be prescribed and kicked afterwards.

Excluded: advancing body poses; general cut cells; covering/uncovering; moving
occupancy transport; thin subgrid walls; tiny-cell stabilization; triple-interface
integration; 3D; surface tension; viscosity; compressible gas; player/swimming;
production integration. Do not add analytic buoyancy, added-mass coefficients,
reaction suppression, momentum redistribution, pressure clamps or mass floors.

## 2. Evidence and source lineage already established

Read the supplied reference/RESEARCH_NOTE.md, SOURCES.md and EVIDENCE_RECHECK.md.
The full original research archive is preserved under evidence/original/.

Executable reference commands, run from a COPY of reference/ to preserve evidence:

    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python projection_checks.py
    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python adapter_reference_checks.py

The first requires NumPy and SciPy, runs 42 positive cases and five negative
controls. Its results were rerun during this handoff; all non-timing result fields
matched the original exactly. The second ran six all-fluid periodic transport
snapshots, each after 12 transport timesteps, followed by frozen projection.
It uses unchanged fixed-grid functions from the preceding P1-C-M Python reference.
Its loader intentionally does NOT import/run the unrelated Shapely geometry work.

These are independent reference checks, not Judas C++ acceptance. Their original
PLIC implementation must not replace Judas's already-passing transport.

Primary mechanism anchors:
- Robinson-Mosher, Schroeder, Fedkiw, JCP 2011 author manuscript, §§4.1–4.3,
  Eqs.8–12: massive fluid duals, explicit normal interface impulses, transpose
  body reactions at matching spatial locations.
- Batty, Bertails, Bridson, 2007, §§2–3: mass-weighted variational projection.
- PETSc MatSetNullSpace documentation: singular-system compatibility is distinct
  from obtaining a least-squares answer after removing incompatible forcing.

Our sign conventions, resolved 2D geometry, SVD solve and test specification below
are an explicit specialization/derivation. They are NOT a verbatim full timestep
from those papers, nor the original van der Eijk–Wellens Crank–Nicolson update.
Do not import either paper's unrelated advection or constitutive choices.

## 3. Component inputs, geometry and mass

Use double precision and a 2D unit-depth domain. Default reference domain is
[0,1]²; MAC spacing is Hx=1/Nx, Hy=1/Ny and the fine mass grid has spacing H/2.

Input geometry consists of actual unions of whole MAC cells: fluid cells and
body-labelled solid cells. Faces are resolved grid segments. This is the authored
geometry, NOT a voxelization of arbitrary shapes presented as exact cut geometry.
The cup is a U-shaped union; its interior and exterior use the SAME physics.

Geometric region/body IDs and prescribed boundary types are legitimate input
classification. They must not select a different reaction law for cup/ship/floaters.

A grid may be rigidly rotated by Q and translated by an origin O. Fluid component
velocities use grid-axis scalars; body translational velocities use world axes.
Keep geometry/lever-arm arithmetic local. Do not form small differences by
subtracting enormous absolute coordinates.

Fluid masses come from fine-grid cell mass:

    m_a = V_a * [rho_g + (rho_l-rho_g)*C_a],  rho_l>0, rho_g>0.

For each scalar MAC velocity, sum the fine cells in its staggered dual volume.
At resolved solid/domain boundaries, EXCLUDE solid/outside pieces. Retain the
actual FLUID HALF-DUAL MASS and its velocity unknown at the boundary. Do not
replace that fluid trace with body velocity and accidentally throw away inertia.
Faces with no fluid on either side have no fluid degree of freedom.

Each component's dual volumes partition the fluid mass separately. Do not add
x- and y-dual totals and call that the total mass. No ghost-cell mass in totals.

For the actual P1-C-M adapter, use its actual mass and momentum arrays; do not
recompute a different convenient density. Resolved-body fixtures initialize fine
mass analytically on their fluid cells. That does NOT imply current transport can
advect through body masks.

Each dynamic body b has:

    q_b = (Vx, Vy, omega)
    H_b = diag(m_b,m_b,I_b), with m_b>0 and I_b>0.

For uniform-density cell-union bodies, compute COM and I by summing rectangular
pieces, including each piece's intrinsic inertia and parallel-axis term. The
reference geometry_body() gives the exact construction. Supports/guides are
explicit constraints; do not emulate an immovable body by giving it huge mass.

The component consumes geometry/connectivity, positive masses, provisional
momenta or velocities, body COM/mass/inertia, dt, prescribed wall normal velocities
and outer pressures. It returns jointly projected velocities, fluid momenta,
pressure/interface impulses, support reactions and numerical diagnostics.

## 4. Assemble the actual operator

Let u contain active fluid scalar face velocities, including boundary traces, and
q all body velocities. Let:

    w = [u;q]
    H = block_diag(M_fluid, H_body_0, H_body_1, ...).

### Cell incompressibility rows D

One row per fluid cell; +A on right/top, -A on left/bottom face DOFs. Adjacent
fluid cells share ONE face unknown. Use integrated divergence D*u, without
cell-volume division in this formulation. In 2D A is edge length times unit depth.
Cell multipliers pi are pressure impulses pi=dt*p.

### Fluid/body normal compatibility rows

For a boundary face f:

    e_f = Q*(positive axis unit vector)
    s_f = +/-1, its outward sign from the fluid cell
    n_f = s_f*e_f
    r_f = Q*(face_local - body_COM_local)

Define:

    S_f*u = A_f*s_f*u_f
    J_f*q = A_f*[n_x,n_y,cross(r_f,n_f)]*q_b

where cross((a,b),(c,d))=a*d-b*c.

Append:

    -S_f*u + J_f*q = 0.

Do not tie tangential fluid velocity to body motion: this is inviscid free slip.

The corresponding multiplier tau_f is an interface pressure impulse. Fluid receives
-A_f*n_f*tau_f, and body receives +A_f*n_f*tau_f with torque
A_f*cross(r_f,n_f)*tau_f at the SAME point. Use the transpose of the assembled
constraint, not separately coded force signs or a second buoyancy channel.

### Prescribed walls, pressure boundaries and supports

A prescribed wall has no dynamic q contribution:

    -S_f*u = -A_f*(n_f dot V_wall).

A prescribed-pressure OUTER boundary has no normal-velocity constraint. Its known
impulse -dt*A_f*n_f*p_external is applied to the fluid provisional momentum before
projection. Internal gas/liquid interfaces are not outer boundaries: both phases
have positive mass and participate in pressure. Do not impose p=0 internally.

A support/guide appends an explicit row selecting a body velocity component,
with a prescribed value (zero in the supported reference fixtures). Its multiplier
is an external generalized impulse, NOT a fluid force. These are fixture supports,
not an implementation of Judas rigid contacts.

Ignoring support rows in this display:

    B = [ D    0 ]
        [-S    J ]

    B*w_new = b.

All bodies participate in the same B/H system.

## 5. Provisional update, solve and output

For this frozen component, provisional velocities are inputs. The fixture helpers
may generate them with the explicit first-order force update:

    u_f* = u_f^n + dt*(e_f dot g) + known_external_impulse_f/M_f
    q_b* = q_b^n + dt*(g_x,g_y,0) + dt*H_b^-1*(F_ext_x,F_ext_y,T_ext).

Apply each force once. No fluid-acceleration low-pass filter. Pressure is solved
rather than prescribed from Archimedes. Existing transport snapshot tests may
supply P/M directly, with no additional gravity.

Solve the mass-weighted projection:

    H*(w_new-w*) = B^T*lambda
    (B*H^-1*B^T)*lambda = b-B*w*
    w_new = w* + H^-1*B^T*lambda.

The matrix B*H^-1*B^T is positive semidefinite, not automatically nonsingular.
The complete mixed KKT system is indefinite. Do not put plain CG on that system.

### Selected correctness solver: equilibrated dense SVD

For this bounded small component, implement the reference's rank-revealing solve:

    Z = B*H^-1/2
    R_ii = 1 / norm(row_i(Z))
    Cmat = R*Z
    d = R*(b-B*w*)
    Cmat = U*Sigma*V^T.

Retain singular values above:

    tau_rank = 64*eps_double*max(rows,columns)*sigma_max.

With retained U_r,Sigma_r,V_r:

    z = U_r^T*d
    delta_y = V_r*Sigma_r^-1*z
    lambda = R*U_r*Sigma_r^-2*z
    w_new = w* + H^-1/2*delta_y.

This is a numerical rank policy, not a physical mass cutoff or a rigorous universal
roundoff bound. Log rank, nullity, singular extremes and threshold. No diagonal
jitter, pressure/mass floors or regularization chosen to pass a fixture.

Explicitly compute:

    incompatible = d - Cmat*delta_y.

If norm(incompatible)/max(1,norm(d)) > 1e-10, reject the input as incompatible;
do not return the least-squares velocity as a physical success. Check the original
unscaled constraints as well. Do not silently drop inconsistent rows or edit b.
Reject malformed/nonfinite inputs, nonpositive masses/inertia and structurally zero
rows rather than dividing by zero. This task's valid fixtures have nonzero rows.

Use a mature C++ SVD, not a hand-written linear algebra library. Prefer an existing
Eigen JacobiSVD (3.4 or compatible newer version); an already available LAPACK
SVD is also suitable for the identical algebra. Keep any dependency prototype-only,
record exact version/build instructions and preserve its notices. For Eigen,
EIGEN_MPL2_ONLY is the documented option to exclude LGPL modules. Dependency
availability is an implementation issue, not permission to change the method.
Do not call the Python reference from the C++ pressure path.

## 6. Invariants and gauges

For delta=w_new-w* and K(w)=0.5*w^T*H*w:

    K_new-K_star = lambda^T*b - 0.5*delta^T*H*delta.

For b=0 this is non-increasing kinetic energy. With moving walls, lambda^T*b is
prescribed-constraint work. Known external-pressure forcing was already included
in w*: account for it separately when comparing against pre-force state.

This is the PROJECTION energy identity at fixed H/geometry, not a full timestep,
advection or moving-body energy theorem.

For linear/angular budgets, include prescribed-wall and support reactions. Do not
claim fluid+body momentum stays constant while an external support pushes on it.
Cell-pressure impulses cancel globally; interface impulses cancel at their shared
locations. The angular budget is the lumped MAC/body budget about a declared local
origin, not a claim about continuum angular momentum through arbitrary advection.

Build pressure gauges from the complete coupled operator. A free piston joins
chambers mechanically even if fluid connectivity is disconnected. In the supplied
sealed-piston fixture there is ONE common pressure gauge, and the differential
pressure is physical. Do not pin one pressure independently in each chamber.

Only gauge-invariant quantities may be compared without a common gauge convention.
Cell pressure p=pi/dt and boundary pressure tau/dt need not coincide in a dynamic
boundary half-dual: that fluid has real inertia.

## 7. Implement the 42 reference cases and five negative controls

Port the fixtures and independent oracles in reference/projection_checks.py,
including dimensions, parameters and individual checks. Use the generic C++
assembler/solver for every case. Analytical force/velocity formulas belong only
in oracles, never the solver. Preserve case identities and report actual counts.

Required reference families:

| Family | Exact obligation |
|---|---|
| All-fluid projection | Closed/open 8x8 grids; positive-density contrasts 1,1e3,1e6; idempotence; expected gauge counts. |
| Sharp hydrostatic jumps | 12x12; densities ratio 1,1e3,1e6; piecewise analytical pressure and near-zero velocity. |
| Submerged bodies | Free and supported, density ratios .001,.5,1,2,1000; supported pressure force 272.5 N; neutral at equilibrium stays at rest; correct initial lighter/heavier response directions. |
| Asymmetric partial immersion | Supported L-shape; analytical Fy=340.76125 N and torque=9.72241071428571 N m. |
| Resolved cup | Generic U-shape; net fluid Fy=-815.45625 N; support load including cup weight=1428.58125 N; near-zero fluid/body velocities. |
| Open piston | Multi-cell 12x6; six masses .01,.1,1,80,1e3,1e6 kg; 100 N for .01 s; exact U=1/[m+1000*(5/12)+1*(5/12)]. |
| Sealed piston | Same real multi-cell coupling; pressure difference 100 Pa; no piston motion; one common gauge. |
| Ambient pressure | Constant 1234 Pa; closed body gets zero net pressure force/torque and no motion. |
| Common free fall | Body/fluid and prescribed walls share U+dt*g; no artificial pressure correction. |
| Covariance | Whole-grid/body rotation; far absolute origin; Galilean shift with transformed prescribed wall data. |
| Free slip | Tangential 1.7 m/s channel flow survives; only normal velocity constrained. |
| Multiple bodies | Two free bodies of different masses/inertias in one system, not independent one-body projections. |
| Arbitrary gravity | Fixed-grid hydrostatics with g=(3.1,-9.81),(-7,2),(9.81,0), matching analytical outer pressures. |
| Pressure refinement | MAC 6/12/24; p=500*sin(pi*x)*sin(pi*y), u*=dt*analytic_grad(p)/rho. Exact independent pressure values, not a RHS made by multiplying the discrete matrix by the desired solution. |

The source defines detailed masks, inertia, force signs and support conditions.
Do not invent different fixtures to reproduce only their headline values.
Random fields may be exported from the supplied seeded reference if needed for
exact cross-language input comparison; do not assert identical RNG streams from
unrelated random libraries. Keep analytical checks independently calculated.

Negative controls, test-only:
1. Halve interface half-dual masses: piston result must fail its unchanged oracle.
2. A single delayed explicit piston reaction: must fail the light-body result.
   This is an intentionally wrong calculation, not evidence about old Judas code.
3. Independently pin pressures in both sealed chambers: detect constraint leakage.
4. Prescribe incompatible sealed-wall flux: require an explicit error.
5. Remove rotational reaction after a solve: detect angular-momentum imbalance.

A negative control PASS means the wrong algorithm was detected, not accepted.

## 8. Connect the new component to actual P1-C-M output

In addition to the 42 component cases, add SIX all-fluid periodic snapshot adapters,
using the same C++ projection implementation and the actual compiled transport:

- fine 24x24 / MAC 12x12;
- density ratios 1,1e3,1e6;
- 12 transport timesteps, cumulative Courant .4;
- sharp circular fraction fixture from adapter_reference_checks.py;
- common-co-motion case: prescribed a=(.37,-.21), transported velocity initially a;
- nonuniform snapshot case: the existing discrete-streamfunction advector and
  initial transported velocity from its coarse MAC field.

For each snapshot, transfer actual staggered M and P into the pressure component:

    H_fluid = actual post-transport dual masses
    u* = actual post-transport component momentum / those masses.

Periodic seam faces are single shared DOFs. Assemble the ordinary wraparound D;
there are no external boundary rows or body DOFs in these six cases. The supplied
adapter_reference_checks.py independently exercises precisely that layout.

Apply ONE projection to each snapshot. Check:
- mass/fraction arrays unchanged byte-for-byte by projection;
- closed periodic component momentum conserved;
- constraints and energy identity;
- idempotence on a second projection;
- common co-motion unchanged;
- one common pressure gauge.

Do NOT feed projected velocities back into the prescribed advector or change the
55 original transport fixtures. These are adapter/component checks, not a newly
approved Navier–Stokes timestep or a moving-body trajectory.

For resolved-body fixtures, use analytical fine-grid mass in the fluid mask. Do
not pretend the existing all-fluid advection transports through solids.

## 9. Numerical gates and evidence quality

Retain all original P1-C-M gates. In particular:

    fraction tolerance = 256*eps_double = 5.6843418860808015e-14.

For projection, keep reference checks and their scales. Core residuals:

    r_constraint = ||B*w_new-b|| /
                   max(1,||B*w*||,||b||)

    r_impulse = ||H*delta-B^T*lambda|| / max(1,||H*delta||)

    r_energy = |K_new-K_star-lambda^T*b+0.5*delta^T*H*delta| /
               max(1,K_star,K_new,|lambda^T*b|,0.5*delta^T*H*delta)

Each <=1e-9. Use the reference's external-reaction momentum budget and mass
partition checks at <=1e-9 as well. Report raw values and per-cell divergence,
normal-velocity mismatches and per-body impulse mismatches; a global aggregate
alone is not sufficient evidence.

These are engineering gates for the specified SI/unit-depth fixtures, not
unit-invariant condition estimates or formal error bounds. Do not relabel them.

Keep the reference's physical gates, notably:
- analytic hydrostatic force/torque and pressure absolute error <1e-7;
- piston velocity relative error <1e-8;
- supported/neutral equilibrium, idempotence and covariance errors <1e-9
  where the reference specifies it;
- sharp density-jump velocity <1e-8 and pressure relative error <1e-9;
- common free-fall correction <1e-10;
- six adapter common-vector errors <1e-8;
- pressure errors decreasing with 6/12/24 refinement, ratios 3.7–4.4 for the
  specified smooth pressure case.

No requirement for bitwise equality between independent C++ and Python SVDs.
Pressure gauges and RNG input must be aligned when making value comparisons.

Positive/negative reference results are supporting evidence, not an oracle for
all possible flows. Check assembly independently too: shared-face signs, edge
normal closure, moment closure, actual mass sums and matching impulse locations.

## 10. Execution, evidence and stopping rule

Suggested order:

    recover P1-C-M baseline
    -> generic resolved-grid masses/DOFs and row assembly
    -> all-fluid SVD projection
    -> dynamic normal-interface rows and support reactions
    -> physical/negative-control families
    -> actual P1-C-M snapshot adapters
    -> complete existing transport regression.

Run small focused fixtures while implementing; fix ordinary coding, build,
indexing and sign errors without requesting a new research decision. Do not keep
reverting a necessary multi-file implementation merely because an unfinished
intermediate state has more red tests.

If a correctly reproduced discrete method contradicts a required physical oracle,
preserve the first failing operation and a minimal counterexample. Stop before
changing the method, widening gates or starting a literature search.

Keep source/results/snapshots under the persistent prototype evidence tree, not
/tmp or disposable build output. Executables/caches belong in ignored build
output; keep supplied reference evidence unchanged and write reruns elsewhere.

Provide a run_p1pf.sh command, a P1PF_METHOD.md equation-to-code map and
P1PF_RESULTS.md plus machine-readable case results. Report:
- actual executed positive/negative/adapter counts;
- unchanged 55-run transport results and preserved fraction CSV comparison;
- matrix sizes, mass ranges, rank/nullity, singular thresholds and conditioning;
- constraint/impulse/energy and external-reaction momentum residuals;
- analytical force/torque/load/pressure/velocity comparisons;
- refined pressure errors, raw extrema and rejected inputs;
- build/dependency details and timings (no 60 Hz claim);
- exact changed files, source fingerprints and git status.

Expected scope of successful evidence: 42 projection cases + 5 negative controls
+ 6 adapters, alongside the preserved 55 transport runs. Count idempotence calls
separately rather than inventing extra physical trajectories.

On PASS, stop and present the candidate for review. Do not infer blanket commit,
push or tagging authorization from the previous checkpoint command. No production
changes or milestone tags. The operator can authorize a separate P1-PF checkpoint.

Success earns a compiled, multi-cell, frozen/resolved-geometry pressure/body
component with simultaneous velocity coupling. It does NOT earn arbitrary cut-cell
transport, body trajectories, general floating equilibrium, swimming, P1-D/E or
full R1. Those scopes stay closed until separately specified.
