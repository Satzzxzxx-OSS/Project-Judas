# P1-PF: frozen, resolved-geometry pressure/body component

This is a separate gate after the committed P1-C/P1-C-M transport checkpoint
`0d73f47206cc34674bb9fbbce2cb438f38ce9f22`. It does not implement a timestep with
moving geometry or establish P1-D/E. Body pose and all masses are fixed during
projection; body velocities are unknowns in the same solve as fluid velocity.

## Lineage and exact specialization

The supplied `evidence/p1pf/handoff/ASTRA_TASK.md` is the specification.
Robinson-Mosher/Schroeder/Fedkiw (2011), §§4.1–4.3/Eqs.8–12, supplies the explicit
massive interface-velocity/transpose-reaction formulation; Batty/Bertails/Bridson
(2007), §§2–3, supplies the variational projection interpretation. The B/H signs,
resolved two-dimensional geometry and equilibrated SVD are the supplied derived
specialization. They are not a verbatim paper timestep or a recovered COSMIC scheme.
The primary citations and supplied supporting results are preserved unchanged in
`evidence/p1pf/handoff/`. No Ghasemi fictitious-domain coupling is imported.

## Equation-to-code map

| Operation | Exact definition | Code |
|---|---|---|
| Fluid regions | Fluid/solid labels author actual whole-MAC-cell unions; fine density is x-major, twice the MAC resolution | `pf::Config`, `pf::FrozenMAC` |
| Fluid mass | Sum fine mass rho*Hx*Hy/4 over the component dual, excluding solid/outside pieces; retain boundary half-duals | `FrozenMAC` face construction |
| Body mass/inertia | Rectangular piece masses, COM, intrinsic `(Hx²+Hy²)/12` and parallel-axis terms | `pf::geometry_body` |
| Cell divergence | D has +A on right/top and -A on left/bottom, sharing each fluid face | `FrozenMAC` cell rows |
| Interface constraint | `-A*s*u + A*(n_x*Vx+n_y*Vy+cross(r,n)*omega)=0` | `FrozenMAC` surface rows |
| Walls | Same fluid row; RHS `-A*dot(n,Vwall)`, no body columns | `FrozenMAC` wall rows |
| Supports | One selected body component equals prescribed zero | `FrozenMAC` support rows |
| Outer pressure | Known impulse `-dt*A*s*p_external` on the scalar fluid DOF before projection | `FrozenMAC::provisional` |
| Provisional force | Fluid `dt*dot(e,g)`; body `dt*g + dt*force/H`, each once | `FrozenMAC::provisional` |
| Whitening | `Z=B*diag(1/sqrt(H))`, `R_ii=1/norm(Z_i)`, `C=R*Z`, `d=R*(b-B*wstar)` | `pf::project` |
| Rank | LAPACK thin DGESVD; retain `sigma > 64*eps*max(rows,cols)*sigma_max` | `pf::project` |
| Correction | `z=U_r^T*d`, `dy=V_r*(z/sigma)`, `w=wstar+dy/sqrt(H)` | `pf::project` |
| Multipliers | `lambda=R*U_r*(z/sigma²)`; cell multipliers are dt*p | `pf::project` |
| Compatibility | Reject `norm(d-C*dy)/max(1,norm(d))>1e-10`; no repaired RHS | `pf::project` |
| Body pressure impulse | Body-column entries of the SAME interface rows transposed against lambda | `FrozenMAC::surface_force` / projection output |
| External reactions | Wall/support row transpose impulses in linear/angular budget | `FrozenMAC::momentum_budget` |
| Actual transport adapter | Existing `jsgtStep` and `advanceMomentum`; actual final staggered mass restriction and momentum, followed by shared periodic D | `p1pf_adapters.hpp` |

Fluid scalars are grid-axis components; rigid translation is world-axis. Lever arms
use local coordinates rotated by Q; they never subtract large absolute positions.
The angular budget is the lumped MAC/body budget about the declared local origin.
It is not a continuum-advection angular-momentum theorem. Both liquid and gas have
positive mass. Internal liquid/gas interfaces have no artificial atmospheric row.
Slip is normal-only. The projection returns both generalized velocities and `momenta=H*velocity`; fluid
outputs use these actual stored momenta. No pose update, mass floor, pressure clamp, analytic buoyancy,
added-mass coefficient, reaction cutoff or independent force reconstruction exists.

## Solver and dependency

The selected mature implementation is the installed LAPACK `dgesvd_` supplied by
OpenBLAS, rather than Eigen. This is an expressly permitted implementation of the
same thin-SVD algebra. The compiled LAPACK version is also queried and recorded. Workspace is queried; nonconvergence and malformed/nonfinite
inputs are errors. The original matrix constraints are checked after solving.
No CG, regularization, diagonal jitter or per-chamber pressure pins are used.

Build requires CMake >=3.16, a C++17 GCC/Clang-compatible compiler, Bash, Python 3
standard library, and an LP64 LAPACK library exposing `dgesvd_`. On Debian-family
systems `liblapack-dev` or the installed OpenBLAS LAPACK runtime supplies it.
CMake finds `lapack`/`liblapack.so.3`; other installations may pass
`-DP1PF_LAPACK=/absolute/path/to/the/library`. The optional target is enabled by
`JUDAS_BUILD_P1PF=ON`; production CMake is untouched. The original transport target
has no LAPACK dependency. Current installed OpenBLAS version is 0.3.32+ds-5;
`dependencies/OpenBLAS-COPYRIGHT` preserves its installed copyright/license notices.
Run metadata records compiler, resolved library, hashes and thread settings.

## Checks and diagnostics

The five normalized gates retain the reference's 1e-9 engineering tolerances:
constraint, impulse equation, projection energy identity, external-reaction
momentum budget and fluid mass partition. Compatibility uses 1e-10. The physical
force/pressure/velocity/refinement gates remain exactly as supplied. The original
fraction gate remains 256 double epsilons; no fraction repair is performed.

At fixed H the measured identity is
`Knew-Kstar = lambda^T*b - 0.5*delta^T*H*delta`.
Known external pressure is already in wstar. Homogeneous projection cannot add
energy; prescribed wall/support work is accounted separately. This is not a
full advection/geometry/body energy claim.

`p1pf_validation.hpp` ports all 42 fixture identities and independent physical
expectations. All use the same assembler/solver. Seeded random INPUT arrays were
exported with NumPy using the reference's seeds and order; the C++ solver never
loads Python answers. `prepare_p1pf_reference.py` reproduces their provenance.
The five intentionally wrong controls are test-only. The wrong double-pressure
pin uses the same reduced Schur system as the reference and a LAPACK least-squares
solve with epsilon cutoff. It is never a pressure gauge policy in the component.

CSV evidence includes every cell constraint, normal mismatch, body impulse,
fluid face mass/momentum, sparse entries of the assembled matrix, analytical
comparisons, pressure extrema, rank/singular threshold and solve timing. Matrix
residual checks test algebra; hydrostatic integrals, piston inertia, cup load,
force/torque and smooth-pressure refinement supply independent physical checks.

The six adapters use actual compiled transport, not the supplied Python PLIC.
Each uses 12 prescribed-advector steps and one projection plus a separately counted
idempotence call. Fractions, masses and prescribed advectors are checked unchanged
by projection. Projected velocity is never fed back to the transport advector.

## Reproduce and limits

From the repository root:

```sh
bash prototypes/fluid_coupling/r1_p1/run_p1pf.sh
```

This runs the original 55-case transport gate, builds/runs the new component,
then verifies and records evidence. OpenBLAS/OMP thread counts are fixed to one
for reproducibility. Supporting Python reruns use NumPy/SciPy and remain separate;
those packages are not C++ runtime requirements.

No body trajectory, covering/uncovering, cut-cell transport, tiny-cell treatment,
triple-interface integration, arbitrary mesh, 3D, wetting, viscosity, swimming,
production integration or real-time performance is established. Resolved solid
fixtures initialize density analytically; transport is not run through their masks.
The frozen component earns only the stated component checks.
