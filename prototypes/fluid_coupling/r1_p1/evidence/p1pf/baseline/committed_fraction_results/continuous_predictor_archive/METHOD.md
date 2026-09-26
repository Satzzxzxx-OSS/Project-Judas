# R1-P1 method and current validation boundary

## Declared method change: JSGT

The active liquid/gas transport prototype uses **Judas Symmetric Geometric Transport (JSGT)**. This is an intentional symmetric nonlinear geometric splitting derived from the full directional COSMIC predictors. It is **not claimed to be the recovered or faithful implementation of van der Eijk–Wellens Eq. 15**.

Van der Eijk & Wellens, JCP 474 (2023), 111796, DOI https://doi.org/10.1016/j.jcp.2022.111796, remains an architectural inspiration for moving-body/free-surface transport and pressure/body coupling. Their predictor structure informs JSGT. The final nonlinear geometric flux composition is Judas's explicitly selected method change. P1-C acceptance, if earned, belongs to JSGT, not to COSMIC as published.

The current executable contains **only liquid/gas fraction transport with prescribed MAC velocities**. No moving-body occupancy, triple-interface geometry, cut-cell pressure, momentum transport, or coupled body solve is active. Those components remain unvalidated and cannot be resumed before the uniform and nonuniform P1-C gates pass.

## Exact active transport algorithm

Let C be liquid area divided by full cell area V = dx*dy. R(C) constructs a bounded PLIC polygon of that area in each cell. I_x(R(C)) integrates the liquid polygon over the upwind donor strip of width |u_face|*dt; I_y is analogous. Fluxes carry signed liquid **area**, already integrated over dt, positive along the corresponding positive grid direction. Each shared face is evaluated once.

1. Reconstruct R(Cn), and compute both original-direction face flux arrays.
2. Compute full predictors from the same Cn:
   X = Cn - (Fx_right(Cn)-Fx_left(Cn))/V + Cn*dt*(u_right-u_left)/dx.
   Y = Cn - (Fy_top(Cn)-Fy_bottom(Cn))/V + Cn*dt*(v_top-v_bottom)/dy.
3. Reconstruct R(X) and R(Y) separately. The continuous-C directional dilatation terms are retained; they are not silently replaced by the binary sequential Weymouth–Yue correction.
4. Compute final fluxes:
   Fx = 0.5 * [I_x(R(Cn)) + I_x(R(Y))].
   Fy = 0.5 * [I_y(R(Cn)) + I_y(R(X))].
5. Update Cnew = Cn - [(Fx_right-Fx_left)+(Fy_top-Fy_bottom)]/V once.

There is **no PLIC reconstruction of averaged scalar fractions**, no alpha clamp, no limiter, and no post-update repair. Invalid states are recorded and rejected, not fed into later stages. The old sequential Weymouth–Yue routine remains comparison-only and is not called by the acceptance runner.

## Scope of the boundedness argument

For uniform translation the directional dilatation terms vanish. Writing X and Y for full conservative geometric sweeps, JSGT satisfies the exact nonlinear identity

Cnew = 0.5 * [X(Y(Cn)) + Y(X(Cn))].

A full directional sweep is bounded for directional CFL <= 1: retained and incoming liquid occupy complementary translated strips of the destination cell. The convex mean of the two sweep orders is therefore bounded, up to geometric floating-point error. The runner checks the composition identity as a diagnostic; that diagnostic never changes the accepted state.

This argument also applies to row-constant u and column-constant v, as used by prescribed rotational advection. It does **not** prove boundedness when the directional divergences are individually nonzero, even when their sum is zero. Such flows have an independent mandatory validation family. No extrapolation to moving boundaries or coupled physics is made.

## Numerical geometry and unchanged tolerances

PLIC and strip intersection arithmetic is performed in each donor cell's unit square. The physical interface normal is mapped to (nx*dx, ny*dy), and strip width to |u|*dt/dx or |v|*dt/dy. The resulting normalized liquid area is multiplied by dx*dy. This is a change of coordinates, not a limiter: it avoids subtracting global O(1) shoelace products to measure O(h^2) cell areas.

Normals use the existing centered axial fraction gradient. This is not the full weighted Youngs stencil; earlier text calling it Youngs was inaccurate. The existing zero-gradient fallback normal (0,-1) is retained and is an axis-bias limitation of this prototype reconstruction.

The material bounds threshold remains 256*double_epsilon = 5.6843418860808015e-14. Every strict excursion is recorded separately. The existing PLIC routine represents roundoff-scale values outside [0,1] by the empty/full limiting polygon; it never changes the stored fraction. This finite-precision representability effect is included in the volume residual. Values beyond the threshold stop before reconstruction. Volume/update residual acceptance remains 1e-11; no threshold was widened.

## Independent transport evidence

`transport_oracles.hpp` does not call PLIC, polygon clipping, or transport flux routines:

- aligned blocks: analytical rectangle intersection lengths;
- oblique rectangles: exact piecewise-linear vertical integration over polygon/cell breakpoints;
- existing nontrivial disk with internal rectangular slot: analytic circle-chord integral minus the independently integrated rectangular slot;
- reversal: exact forward/reverse translated shape;
- rotation: exact rigidly rotated slot geometry, one full revolution;
- nonuniform incompressible strain: u=x-1/2, v=-(y-1/2), exact map x'=1/2+exp(t)(x-1/2), y'=1/2+exp(-t)(y-1/2).

The strain field has directional divergences +1 and -1, with zero total divergence to floating-point accuracy, and preserves area analytically. The nonuniform constant-field checks use a dyadic 32x32 grid so the affine field has exactly zero discrete divergence and the bit-exact constant assertion is meaningful. The deforming-liquid case remains at 24x24 and has dt=1/32 and directional CFL=0.375. An independently derived interior-cell witness is described in the results if the run confirms its failure. No special rules for this fixture exist in JSGT.

## Deferred triple-interface reconstruction

The inactive contact-cell component retains the unconstrained Choi–Bussmann/Ghasemi fraction-error minimization: solid PLIC; accessible polygon; trial triple point; liquid-area placement; 3x3 liquid/gas fraction error; both branches searched. Source: A. Ghasemi, *Computational Simulation of the Interaction Between Moving Rigid Bodies and Two-Fluid Flows*, UMass Dartmouth, 2013, Sec. 3.4; Choi & Bussmann (2007), DOI https://doi.org/10.1002/fld.1317.

The earlier 90-degree requirement is withdrawn. Prescribed wetting/contact-angle physics is outside P1. No Ghasemi fictitious-domain coupling is imported. This reconstruction is not invoked by the present transport-only executable and supplies no P1-C evidence.

## Explicit exclusions

No production integration, solid motion, cut cells, coupled pressure, P1-D/E, P2, 3D, rigid rotation/6DOF, AMR, PBF, analytic buoyancy, surface tension, mass cutoffs, or fixture-dependent physical rules.
