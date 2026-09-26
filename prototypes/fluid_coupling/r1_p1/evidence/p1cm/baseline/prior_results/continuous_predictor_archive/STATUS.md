# R1-P1 status — STOPPED before P2

This is an incomplete prototype checkpoint, not a P1 pass. The attached checkout was accessible at `/home/conner/Documents/GitHub/Project-Judas` and was at `48385a78afb2930744f2b87ad6aa55452f18ae18`. All new files are under `prototypes/fluid_coupling/r1_p1/`; no runtime/editor/test source or project build configuration was changed. No commit or tag was created.

## Latest P1-C gate (2026-09-26): JSGT

Option B was intentionally implemented as **Judas Symmetric Geometric Transport (JSGT)**. It is not described as faithful reproduction of the authors' COSMIC Eq. 15. Uniform aligned/oblique/slotted/reversal tests pass with decreasing error at 24/48/96, and a 32² full rotation passes. Bounds and conservation remain within the unchanged numerical tolerances.

The mandatory nonuniform divergence-free strain fixture fails at step 0, y predictor, cell (6,18): Cn=1/8 is fully exported by the geometric strip, then the explicit continuous-C divergence correction subtracts 1/256, giving **-0.00390625**. The failure precedes final flux averaging. No limiter or predictor replacement was added. See `P1C_RESULTS.md`, `METHOD.md`, and `results/jsgt_*`.

**P1-C remains FAILED/INCOMPLETE. Integration is blocked.** No moving solids, triple-line integration, pressure/body coupling, P1-D/E, or P2 were resumed.

The component measurements below are historical results from earlier isolated component runners. They are not results from the current P1-C runner and do not establish integrated P1 acceptance.

## Implemented component checks

- `hydrostatic.hpp`: static 2D finite-volume pressure solve with PCG, no-flow side/bottom and atmospheric top boundary. Pressure is compared cellwise with `rho*g*(H-y)`.
- `vof.hpp` + `main.cpp`: polygon clipping and PLIC donor fluxes. The current runner implements JSGT and stops at the nonuniform predictor failure described above; `wySequentialReference()` is comparison-only. No production or moving-body transport path is integrated.
- `cutcell.hpp`: exact axis-aligned rectangle occupancy, face apertures, and boundary patch geometry. These helpers are not yet connected to VOF transport or the pressure solve.
- `coupled_projection.hpp`: graph-form pressure/body Schur operator with one translational body, pressure patch impulses computed independently for fluid and body from the same patch list. Only a synthetic one-cell algebraic case currently exercises it.

## Measured results

| Component | Resolution | Main result | Runtime |
|---|---:|---|---:|
| Static hydrostatic column | 24² | L1 pressure `4.97e-11`, L2 `5.89e-11`, max `1.46e-10`; max corrected speed `2.48e-12`; PCG residual `8.93e-7`, 109 iterations | 0.00074 s |
| Static hydrostatic column | 48² | L1 `1.50e-10`, L2 `1.69e-10`, max `3.42e-10`; max corrected speed `3.42e-12`; residual `2.87e-6`, 222 iterations | 0.00509 s |
| Static hydrostatic column | 96² | L1 `5.44e-10`, L2 `6.04e-10`, max `9.73e-10`; max corrected speed `4.71e-12`; residual `8.21e-6`, 445 iterations | 0.0441 s |
| Free-surface translation, prescribed uniform velocity | 24² | liquid volume `0.06` unchanged; geometric L1 `1.633e-3` | 0.00254 s |
| Same | 48² | volume error `1.39e-17`; geometric L1 `9.966e-4` | 0.0147 s |
| Same | 96² | volume error `1.39e-16`; geometric L1 `4.099e-4`; fraction range `[-2.22e-13, 1+1.0e-11]` | 0.0956 s |
| Prescribed-velocity piston kinematics | 24²–96²; three offsets each | analytic and rasterized solid sweep both `0.1`; liquid volume error at most `2.99e-14`; PLIC/solid overlap at most `3.16e-14` | 0.0068–0.48 s per case |
| Synthetic coupled projection | one pressure cell | pressure `689.655 Pa`; constraint residual `0`; fluid/rigid patch impulses `-68.9655/+68.9655 N·s`; PCG residual `0`, 1 iteration | negligible |

The static pressure solve is near machine-precision against its linear oracle; absolute PCG residual grows with system size while remaining below its specified relative stopping tolerance. Translation L1 error decreases with refinement. The piston check prescribes a uniform velocity field: it does not solve the moving-wall boundary condition or pressure/body coupling.

## Required P1 work still absent

- JSGT has passed the uniform/rotation transport gates but **fails boundedness** on the first nonuniform y predictor. P1-C has not passed and is not connected to moving-body occupancy, cut cells, pressure, or body coupling.
- Body occupancy, face apertures, and boundary patches are not connected to the transport or pressure operators. There is no moving-solid VOF/COSMIC body transport.
- The coupled pressure/body operator is only verified algebraically in one synthetic cell. It is not assembled from the grid/cut-cell geometry and does not include the free-surface VOF solve.
- Consistent momentum transport over MAC momentum control volumes is not implemented.
- P1-A's static hydrostatic check does not exercise a full velocity-prediction/projection timestep.
- P1-B is only the prescribed-velocity piston kinematic oracle, not the required pressure-coupled moving-wall fixture.
- P1-D and P1-E were not run. There are no floating-body density-ratio or dynamic-tank results.

## Decision

P1 has **not passed** and P2 is not justified. The immediate blocker is the nonuniform directional predictor, not missing integrated fixture machinery. Stop here for operator review; do not add a limiter or resume integration. No production code was changed.
