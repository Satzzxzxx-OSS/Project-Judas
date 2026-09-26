# P1-C-M: fixed-grid mass-consistent momentum transport

This implements the user-supplied integrated-mass/JSGT specification, preserved in `evidence/p1cm/handoff/ASTRA_TASK.md`. Published anchors and their notation caveats are preserved unchanged in the handoff. This is a derived Judas transport method, not verbatim paper equations or a complete Navier-Stokes method.

## Connection to the accepted fraction operator

`jsgtStep()` remains the authoritative fraction update. Its optional `TransportRecord` exposes the four actual signed, timestep-integrated phase-volume flux arrays and original/intermediate/final fraction states. Momentum consumes these arrays without another PLIC or phase-flux calculation. `normalAt()` and `geometricFaceFlux()` gained periodic index handling only when opt-in flags are set; legacy fixtures retain their original indexing, velocities, timestep policy and arithmetic.

Both branch predictors and complete orders are already computed by JSGT: x then y uses original-x and y-after-x transfers; y then x uses original-y and x-after-y. All directions use the full timestep. The advecting Grid u/v arrays remain prescribed and untouched by momentum.

## Mass records

For each stored phase transfer Ql, `advanceMomentum()` forms Q=face_velocity*dt*face_area and Fm=rho_l*Ql+rho_g*(Q-Ql). `MassStageRecord` stores Fm. The source is V*(rho_g+(rho_l-rho_g)*frozen_colour)*D, including rho_g. Every fine cell is checked against m_out=m_in-net(Fm)+source with mass independently evaluated from its post-stage fraction. No mass floor is used.

## Staggered restriction and physical boundaries

`DualGrid` partitions fine cells into the specified shifted 2x2 groups. Periodic indices wrap. Open-boundary groups contain only actual cells, resulting in half-width/half-height boundary dual volumes. `owner` maps every fine cell to exactly one dual volume per component; construction rejects overlaps and gaps. Global mass is summed once over fine cells. Component momenta each use only their respective dual partition.

`fineFaces()` traverses unique fine-face segments. Internal segments within a dual volume cancel. At shared dual boundaries, `segmentMomentum()` selects a donor from the sign of EACH fine-segment Fm before any summation. A shared signed momentum transfer is applied with opposite incidences. Open-boundary donors use explicit prescribed incoming component velocities; outgoing transfers use the physical interior donor. Exterior cells never contribute physical mass.

## Directional updates and branch combination

The donor velocity is current branch P/M; the source velocity is frozen Pn/Mn from the full timestep start. After each directional stage, mass is restricted from the matching new fractions before the next donor velocity is evaluated. Both branch momenta start from the same Pn and use the same frozen source velocity and colour fields.

Final momentum is 0.5*(Pxy+Pyx). Final normalization uses mass restricted from authoritative JSGT Cnew. Mean branch mass is compared with that mass, never forced into agreement. Wrong-old-mass and independent-velocity-average values are diagnostics only and do not feed accepted state.

## Numerical gates and energy

Local fine/dual mass ledgers and final restriction use the supplied local scale and 1e-12 gate. Global mass/component-momentum budgets include accumulated signed boundary transfers and use initial unsigned quantities, accumulated unsigned boundary transfers, and gas reference scales with the supplied 1e-11 gate. Common vectors use 1e-8*Uref. Combination energy uses the supplied positive-mass convexity test with 1e-12 normalized tolerance. Fractions retain 256 epsilon; no fraction or velocity repair is added.

Closed periodic full-step energy growth above 1e-11 of the declared reference scale is an investigation gate chosen before running the new fixtures. Combination convexity does not establish full-step stability. Raw energy, branch-mean energy, velocity extrema, common-vector errors and budget diagnostics are reported.

On any rejected momentum step, callers stop. Fraction evaluation has already occurred, but no MomentumState is committed; the inconsistent candidate pair must not be continued.

## Coarse advector input

`prolongMAC()` implements the specified copied outer-face / averaged interior-face prolongation. It returns the maximum fine-minus-parent divergence difference. Original P1-C fixtures never call this operation and retain their exact prior prescribed fine-face arrays.

## Evidence boundary

The unmodified accepted sources and prior results are under `evidence/p1cm/baseline/`. Before editing, the accepted executable reran all 25 cases with identical non-timing metrics. The supplied archive and hash-verified contents remain under `evidence/p1cm/handoff/`; supporting Python evidence is under `reference_rerun/`. That loader executes unchanged fixed-grid functions only because Shapely is unavailable; no excluded moving-geometry work was executed.

All current C++ evidence is under `evidence/p1cm/current/`, with fraction CSVs also retaining their existing `results/` paths. This task adds no pressure, solids, gravity, viscosity, buoyancy, surface tension, player systems, production changes, tags, commits or pushes.
