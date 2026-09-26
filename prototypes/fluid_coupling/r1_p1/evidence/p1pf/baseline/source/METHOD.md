> Preserved P1-C fraction checkpoint: the active executable now also runs P1-C-M. See `P1CM_METHOD.md` and `P1CM_RESULTS.md`. Statements below about executable scope describe the earlier fraction-only checkpoint. Its full original source/report is preserved in `evidence/p1cm/baseline/`.

# R1-P1 method: frozen-WY Judas Symmetric Geometric Transport

## Scope and lineage

The current executable validates **P1-C liquid/gas VOF transport only**, with prescribed MAC velocities. Its declared method is:

**JSGT = symmetric geometric flux averaging + frozen Weymouth-Yue directional predictor correction.**

This is not van der Eijk-Wellens Eq. 14 unchanged, and is not claimed as their exact COSMIC Eq. 15 implementation. Their architecture and directional predictor structure inspired the prototype. Judas selected the nonlinear flux composition; the current predictor correction is the published frozen Weymouth-Yue construction. Acceptance belongs to this explicitly defined method.

No momentum, energy, pressure, moving-solid occupancy, cut cells, triple-line reconstruction, or rigid-body motion is evolved by this executable. P1-C acceptance does not establish integrated P1 or P2.

## Discrete state and exact operations

C_i is liquid area divided by cell area V=dx*dy. R(C) constructs a PLIC polygon. Each signed face flux I_d(R(C)) is liquid area within the upwind strip of width abs(u_face)*dt. It already includes timestep and transverse face length. Positive flux points in the positive coordinate direction. Each shared face has one stored value.

Define Lx(A)=(Ix_right(R(A))-Ix_left(R(A)))/V, and similarly Ly. Define dx_step=(u_right*dt*dy-u_left*dt*dy)/V and dy_step=(v_top*dt*dx-v_bottom*dt*dx)/V. These divergence terms use total velocity flux, not liquid flux.

For every timestep:

1. Validate input bounds and cumulative Courant condition.
2. Compute c_i = 1 if Cn_i > 0.5, otherwise 0, once from Cn. Freeze the entire field.
3. Reconstruct R(Cn), evaluate original x and y liquid face fluxes.
4. Compute X=Cn-Lx(Cn)+c*dx_step and Y=Cn-Ly(Cn)+c*dy_step.
5. Check both predictors before reconstructing R(X) and R(Y) separately.
6. Evaluate crossX=Ix(R(Y)), crossY=Iy(R(X)).
7. Form finalX=0.5*(Ix(R(Cn))+crossX), finalY=0.5*(Iy(R(Cn))+crossY).
8. Apply Cnew=Cn-divergence(finalX,finalY)/V once. This is the only accepted-state update.

No averaged scalar field is reconstructed. No alpha clamp, fraction repair, or extra limiter was added. The 0.5 binary coefficient is the numerical WY colour quadrature, not a physical/body/fixture classifier.

## Sweep-order identity and boundedness conditions

Define Wd(A;c)=A-Ld(A)+c*dd_step, with the SAME original frozen c even for second sweeps. The diagnostic computes both complete orders:

- WxY=Wx(Wy(Cn;c);c)
- WyX=Wy(Wx(Cn;c);c)

Both are checked individually. Expanding their mean gives exactly:

0.5*(WxY+WyX) = Cnew + c*(dx_step+dy_step).

Consequently the mean equals the accepted JSGT flux-divergence update for discretely divergence-free velocities. The raw difference, discrete divergence, and divergence-adjusted difference are all reported. Diagnostic states never replace Cnew.

The sufficient implementation gate is STRICT:

dt*(max_faces(abs(u))/dx + max_faces(abs(v))/dy) < 0.5.

This is not asserted to be the sharp necessary stability bound. The harness doubles a fixture's step count until the condition holds, preserving duration, geometry and velocity. This also preserves reversal timing and dyadic constant-field timesteps. The operator checks the condition on every step; a later violation is rejected, not silently repaired. Current fixtures have time-independent velocity magnitudes (reversal only changes signs).

For bounded PLIC donor regions and nonoverlapping directional strips, each complete frozen-WY order is bounded under this cumulative restriction and discrete incompressibility. Their convex mean is bounded. In particular, cells whose frozen coefficient is zero transport liquid conservatively, initially at most half full; cells whose coefficient is one obey the complementary argument for gas. This establishes an admissibility argument, not an accuracy/convergence theorem.

## Conservation and diagnostics

For final averaged fluxes define B=sum(right-left boundary flux)+sum(top-bottom boundary flux). B is positive for net liquid leaving the domain.

- Per-step residual: volume_new-volume_old+B.
- Cumulative residual: volume_current-volume_initial+sum_accepted_steps(B).
- Each predictor/virtual-order residual additionally subtracts its frozen-colour dilation volume. Directional intermediate states need not separately conserve global volume.

`results/jsgt_steps.csv` records each step's dt, both maximum directional Courant numbers, their sum, discrete divergence, X/Y/order/final extrema, volumes, outward boundary volume, final residual, both identity diagnostics, acceptance and first failing operation. `jsgt_trace.csv` records all five stages, flux magnitude, strict and material bound excursions, budget residual and available independent geometric error. `jsgt_summary.csv` gives per-run maxima, boundary budgets, refinement errors and timings.

The unchanged material alpha tolerance is 256*double_epsilon = 5.6843418860808015e-14. Every strict excursion is separately logged. Budget tolerance remains 1e-11; raw and adjusted order identity gates use the alpha tolerance. Bit-exact pure gas/liquid assertions remain unchanged. A failed stage stops before its geometry is reconstructed or its candidate is accepted.

## Existing geometric limitations

Geometry uses donor-local unit coordinates. PLIC target area and donor-strip intersections are geometric; no fixture-specific adjustment is present. Normals remain centered axial fraction gradients, not a full weighted Youngs stencil. The existing zero-gradient normal fallback (0,-1) remains an axis-bias limitation; it was not retuned.

The pre-existing PLIC helper represents roundoff-scale fractions outside [0,1] by the limiting empty/full polygon, after the material-bounds gate. It does not overwrite the stored fraction. This representability limitation is explicit; its effects remain in the measured budgets. Material excursions are never passed into it.

## Independent expectations

`transport_oracles.hpp` never calls prototype PLIC or clipping. It supplies rectangle overlap, piecewise-linear polygon integration, circular-chord integration minus a rectangular internal slot, and transformed reversal/rotation shapes.

The unchanged affine strain fixture uses u=x-0.5, v=-(y-0.5), initial rectangle x=[1/4,3/4], y=[1/4,145/192]. Its exact map is x'=0.5+exp(t)*(x-0.5), y'=0.5+exp(-t)*(y-0.5), with unit area Jacobian. Only dt changed to meet the cumulative restriction. The nonuniform pure-phase checks retain their dyadic 32x32 grid and bit-exact assertions.

The added nonlinear field is u=s^2, v=-2*s*q, s=x-0.5, q=y-0.5. It is divergence-free with individually nonzero, spatially varying directional divergences. MAC samples are exact face averages. Its independent exact map is s'=s/(1-t*s), q'=q*(1-t*s)^2. The initial rectangle is [0.25,0.75] x [0.3,0.7], with area 0.2 and duration 0.5.

`nonlinear_transport_oracle.hpp` integrates curved mapped bounds exactly: b(q0,x)=0.5+q0/[1+t*(x-0.5)]^2. Cell x intervals are divided at analytical crossings with horizontal cell edges. Between crossings, the appropriate cell edge or curve is integrated using integral_l^r b dx = 0.5*(r-l)+q0*(r-l)/(D(l)*D(r)). All oracle arithmetic is long double until conversion to a cell fraction. Nonzero-time oracle area/bounds checks are recorded separately. The t=0 rectangle comparison is only an initialization check.

Two extra open-boundary cases exercise nonzero export. The finite rectangle has geometric reconstruction error. The full-height slab provides an exact independent check: [0.75,0.95] translated by 0.2 leaves area 0.05 and exports 0.15. Both retained and exported areas are explicitly asserted, as well as the shared-face budget.

## Historical evidence and deferred components

The failed continuous-C predictor implementation and its reports/results are preserved under `results/continuous_predictor_archive/`. The earlier scalar-average experiment remains under its existing historical filenames. Neither is relabeled as a successful current result.

The inactive triple-interface component retains unconstrained Choi-Bussmann/Ghasemi fraction-error minimization. The former imposed 90-degree condition remains withdrawn. Wetting/contact-angle physics is outside P1; no fictitious-domain coupling is imported. It supplies no evidence for P1-C and was not integrated here.

## Sources

- Weymouth & Yue (2010), *Conservative Volume-of-Fluid method for free-surface simulations on Cartesian grids*, JCP 229, 2853-2865. DOI: https://doi.org/10.1016/j.jcp.2009.12.018. Eqs. 19-21 and Appendix A, Eqs. 41-43. Continuous old-C is analyzed as potentially unbounded; frozen binary correction and cumulative Courant restriction are the adopted directional construction.
- van der Eijk & Wellens (2023), JCP 474, 111796. DOI: https://doi.org/10.1016/j.jcp.2022.111796. Architectural/predictor inspiration; current JSGT differs in predictor coefficient and nonlinear final flux composition.
- Ghasemi (2013), *Computational Simulation of the Interaction Between Moving Rigid Bodies and Two-Fluid Flows*, section 3.4; Choi & Bussmann (2007), https://doi.org/10.1002/fld.1317. Deferred triple-interface geometry only.
