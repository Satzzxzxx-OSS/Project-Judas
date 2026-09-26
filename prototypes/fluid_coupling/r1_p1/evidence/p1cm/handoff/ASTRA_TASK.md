# Project Judas — R1 / P1-C-M
## Fixed-grid mass-consistent momentum transport

Implement this task now. Method selection and the reference calculations are supplied below. This is a bounded implementation assignment, not a request for another literature review.

### 1. Scope and starting state

P1-C's frozen-Weymouth–Yue JSGT fraction transport passed its reported 25 runs. Preserve that checkpoint. Add mass and component-momentum transport to the SAME compiled prototype, using the SAME geometric phase-volume fluxes.

This task is called P1-C-M only to distinguish it from the existing fraction gate; it does not rename or authorize P1-D/E. It contains no pressure solve, moving solids, cut cells, triple-interface integration, gravity, viscosity, surface tension, buoyancy, swimming, 3D conversion, or production-engine integration. In particular, do not implement the moving-geometry examples included in the research archive.

Work in `prototypes/fluid_coupling/r1_p1/` and its existing prototype build arrangement. Inspect current git/worktree state first; the last reported production checkpoint is `48385a78afb2930744f2b87ad6aa55452f18ae18`. Do not reset or overwrite newer prototype work. Record hashes and rerun the accepted P1-C command before editing.

### 2. Evidence supplied — what it establishes

Use the attached handoff package:

- `evidence/original/Judas_R1_compatibility_research.zip`: original research and its history, preserved unchanged.
- `evidence/rechecked/compatibility_checks.py`: revised, independently executed Python reference.
- `evidence/rechecked/compatibility_results.json`, `run_output.txt`, `RUN_METADATA.json`, and `requirements.txt`.
- `EVIDENCE_RECHECK.md`: rerun results, limitations, and the counterflow correction made before this handoff.
- `SHA256.json`: package integrity manifest.

The revised reference ran 18 fixed-grid cases over 216 timesteps: fine grid 24x24, staggered grid 12x12, density ratios 1, 1,000 and 1,000,000, two prescribed advectors, and constant/smooth/random transported vectors. Worst normalized mass error was 1.2424e-16, component-momentum error 3.2546e-16, staggered mass-update error 3.1991e-16, and constant transported-velocity error 5.6852e-12. The fraction tolerance was 256 double epsilon.

An additional analytic counterflow check passes: mass transfers +0.25 and -0.25 with donor velocities 2 and -1 have net mass zero but momentum transfer +0.75. Combining the mass transfers before donor selection incorrectly gives zero momentum transfer.

Run the revised reference once if the supplied environment supports it. A missing Python dependency is not a missing physics specification. Its PASS is supporting evidence, NOT acceptance of your C++ implementation. Reuse Judas's accepted transport; do not replace it with the Python PLIC implementation.

Published anchors:

S1. Pal, Fuster, Zaleski (2021), *A novel momentum-conserving, mass-momentum consistent method for interfacial flows involving large density contrasts*, §§3.2 and 3.4, especially Eqs. 21, 25–26, 32–33, the frozen source-velocity statement, and Eq. 40 / Figs. 4–5.
https://arxiv.org/pdf/2101.04142

S2. Huang, Valle, Lidtke, Hendrickson, Weymouth (2026 preprint), *Sharp-interface Simulations of Energetic Multiphase Flows with Large Density and Viscosity Ratios*, Eqs. 5–7. Independent corroboration of synchronized directional mass/momentum updates; NOT validation of JSGT or moving-body coupling.
https://arxiv.org/html/2606.02467v1

S3. Basilisk `vof_advection`: binary `cc` is constructed before directional sweeps. This supports frozen-WY sequencing only; do not import its embedded-boundary approximation.
https://basilisk.fr/src/vof.h

The JSGT branch combination and integrated-mass equations below are our explicit derived specification. They are not presented as verbatim equations from those papers or a faithful reproduction of van der Eijk–Wellens. No full Navier–Stokes stability theorem is claimed.

### 3. State layout and units

Use double precision and a two-dimensional, unit-depth domain. Fractions live on a uniform fine grid with spacings hx, hy; momentum components live on a staggered MAC grid with spacings Hx=2hx, Hy=2hy. Fine-grid counts must be even.

Keep the existing P1-C fine-grid resolutions and face velocities unchanged for its regression runs. The new momentum grid at half that resolution does not require changing the old fraction fixtures.

Let fine cell (i,j) occupy [i*hx,(i+1)*hx] x [j*hy,(j+1)*hy]. For coarse indices (I,J):

- Cell-centred coarse region contains fine i={2I,2I+1}, j={2J,2J+1}.
- x-component dual volume, centred at (2I*hx,(2J+1)*hy), contains fine i={2I-1,2I}, j={2J,2J+1}.
- y-component dual volume, centred at ((2I+1)*hx,2J*hy), contains fine i={2I,2I+1}, j={2J-1,2J}.

Use periodic indices in periodic tests. At physical open boundaries, intersect these dual volumes with the domain; use their actual constituent fine cells and actual volume, including half-width boundary dual volumes. Do not count ghost cells as physical mass.

Each component's dual volumes partition the domain separately. Compute global mass once from fine cells, global x-momentum from x-dual volumes, and global y-momentum from y-dual volumes; do not add the two dual mass arrays together as total fluid mass.

Choose positive constant phase densities rho_l and rho_g. Zero-density vacuum is not supported in this task. Do not introduce density floors or arbitrary minimum masses.

Distinguish the prescribed ADVECTING field a=(ax,ay) from the TRANSPORTED component velocities wq=Pq/Mq. Freeze the advecting face field for a whole timestep and both sweep orders. Do not feed unprojected output momentum back into it between stages.

### 4. Coarse-to-fine advector, where used

Add the explicit first-order divergence-preserving prolongation for new MAC-input tests:

- Split each coarse face into two fine faces carrying that coarse normal velocity.
- On the interior fine x-face of a coarse cell, use the mean of its coarse left/right normal velocities, copied to both fine y positions.
- On the interior fine y-face, use the mean of coarse bottom/top velocities, copied to both fine x positions.

For this construction, each fine cell's discrete divergence equals its parent's coarse divergence, up to arithmetic error. Test that identity directly.

Existing P1-C tests continue accepting their original prescribed fine-face arrays. Do not substitute a prolonged coarse approximation and compare it against an unchanged analytic-flow oracle as though the advector were identical.

Enforce the existing fine-grid cumulative Courant gate:

    dt * (max(abs(ax))/hx + max(abs(ay))/hy) < 0.5

### 5. Fine-grid mass and flux contract

For a fine cell a with volume Va=hx*hy:

    ma(C) = Va * [rho_g + (rho_l-rho_g)*Ca]
    ca = 1 if Ca^n > 0.5 else 0

Freeze ca once per timestep, shared by both complete sweep orders.

For each directional stage, obtain Ql_f from the actual existing geometric face-flux operation on the CURRENT branch fraction field. Store it; do not independently reconstruct another liquid flux for momentum.

Use signed, timestep-integrated transfers:

    Q_f  = dt * prescribed_normal_velocity_f * fine_face_area
    Fm_f = rho_l*Ql_f + rho_g*(Q_f-Ql_f)

No additional dt or face-area factor is applied to Fm_f.

With D_a,d = net_outward(Q_f)/Va in direction d:

    C_out = C_in - net_outward(Ql_f)/Va + ca*D_a,d
    S_a,d = Va * [rho_g + (rho_l-rho_g)*ca] * D_a,d
    m_out = m_in - net_outward(Fm_f) + S_a,d

The mass formula follows algebraically from the approved fraction formula. It must agree with ma(C_out) at every stage. The gas-density part of S is required; do not retain only (rho_l-rho_g)*ca.

Directional intermediate states have source terms and need not separately conserve total mass. Those terms cancel over both directions for the prescribed discretely divergence-free field. Measure the actual divergence residual rather than assuming exact cancellation.

### 6. Staggered restriction and momentum flux

For component q and dual volume K:

    M_K = sum(ma over its fine cells)
    S_K,d = sum(S_a,d over the SAME cells)

Boundary mass transfers are sums of fine-face transfers on K's boundary. Internal fine faces cancel. There is no separate PLIC or density reconstruction on the MAC grid.

First-order donor-cell velocity is the selected momentum reconstruction for this task. No higher-order limiter or viscosity is needed to implement it.

For EACH fine-face segment f belonging to a dual face:

    w_donor = current-stage P_left/M_left if Fm_f > 0
              current-stage P_right/M_right if Fm_f < 0
    Fp_f = Fm_f * w_donor

Zero transfer gives zero momentum transfer. Then sum Fp_f over the dual face.

IMPORTANT: upwind each segment before summing. Opposite transfers through two segments may cancel in net mass while transporting nonzero net momentum. The original reference used net-first upwinding; the rechecked reference and its analytic test correct that issue.

Use one shared signed face momentum transfer for adjacent dual volumes, with opposite incidence signs.

For the source term, freeze the velocity from the START of the timestep:

    w_K^n = P_K^n / M_K^n
    P_K,out = P_K,in - net_outward(Fp_f) + w_K^n*S_K,d

The flux donor velocity is stage-current; the source velocity is timestep-frozen. Both roles are intentional. Recover stage velocity using the corresponding NEW stage mass before the next direction.

### 7. Execute both complete orders and combine conserved quantities

Starting from identical Cn and Pn, run:

    branch xy: x fraction/mass/momentum stage, then y stage
    branch yx: y fraction/mass/momentum stage, then x stage

Each branch uses the same prescribed advector, frozen ca and frozen w_K^n. Both directions use the full approved timestep, as in the existing JSGT sweep-mean identity; this is not a new half-step Strang scheme.

Keep the existing shared-face final JSGT fraction update authoritative. Check it against 0.5*(Cxy+Cyx).

Combine:

    M_mean = 0.5*(Mxy+Myx)
    P_new  = 0.5*(Pxy+Pyx)

Check M_mean against restriction of mass from final JSGT C. They must agree within the recorded floating-point/discrete-divergence budget. Use the mass corresponding to final C when recovering final velocity. Do not force consistency by changing C or rescaling P.

    w_new = P_new / M_new

Never average branch velocities independently. With branch masses (1,9) and velocities (1,0), the correct combined velocity is 0.1, not 0.5.

This construction has three established algebraic checks:

- A common transported vector U preserves P=M*U under matching stage updates.
- Shared face momentum transfers telescope; frozen source terms cancel with discrete total divergence.
- For positive masses, energy of the combined state cannot exceed mean branch energy, because P^2/(2M) is convex.

The last statement concerns COMBINATION ONLY, not every directional stage or a full fluid simulation.

### 8. Tests to implement and run

Use one compiled transport/momentum implementation for all fixtures. Analytical oracles and negative controls stay outside that implementation.

A. Existing P1-C regression
Run all 25 accepted fraction runs without altering geometry, prescribed fine velocities, timestep policy, bounds tolerance or analytical oracles. Check that adding momentum does not change their fraction results beyond existing numerical tolerances.

B. Local accounting and counterflow
At every directional stage compare restricted post-stage mass with the independently assembled flux-plus-source update. Check x/y indexing, signed transfers, and all physical boundary dual volumes. Include the +0.25/-0.25 counterflow witness, the unequal-branch-mass averaging witness, and a check that source cancellation uses timestep-frozen velocity.

C. Reference-family reproduction
Run the 18-case periodic family at fine 24x24 / MAC 12x12 for 12 steps each, with density ratios 1, 1e3 and 1e6. Use both prescribed uniform and discrete-streamfunction nonuniform divergence-free advectors, and constant/smooth/seeded-random transported fields. Exercise the new prolongation. Compare invariants, not bitwise agreement between independent PLIC implementations.

D. Genuine common co-motion
Use a constant advector U and set transported velocity equal to U. Translate a sharp density interface, checking no spurious relative velocity at density ratios 1, 1e3 and 1e6. Also retain constant-vector/nonuniform-advector checks, clearly labelled manufactured transport tests rather than self-consistent flow solutions.

E. Momentum accuracy under refinement
On a periodic unit square, rho_l=rho_g=1, prescribe a=(0.37,-0.21) and transport smooth component fields, for example:

    wx = 0.73 + 0.20*sin(2*pi*x) + 0.10*cos(2*pi*y)
    wy = -0.42 + 0.15*cos(2*pi*x) - 0.05*sin(2*pi*y)

Initialize exact averages over each dual rectangle using analytic sine/cosine integrals, not point samples. Evolve to T=0.5. Evaluate the same integrals shifted by a*T as the independent reference. Require decreasing weighted L1 error at fine 24/48/96, MAC 12/24/48. First-order momentum advection is deliberate; do not claim second order.

F. Open-boundary budget
Use x in [0,1], periodic y, a=(0.3,0), T=0.5, initial liquid slab x in [0.8,1], gas elsewhere, gas inflow, and transported velocity a. Analytic retained/exported liquid areas are 0.05/0.15. Net fluid-mass loss is 0.15*(rho_l-rho_g); net x-momentum loss is 0.3 times that mass loss. Include gas inflow and actual half-width boundary dual volumes in the budgets. Do not set outflow to zero or include ghosts in totals.

G. Negative controls and covariance
In test-only copies, demonstrate failure when dividing new momentum by OLD mass and when averaging branch velocities instead of conserved quantities. Check axis exchange/sign reversal of equivalent fixtures. Do not manufacture positive evidence by disabling the new code path.

### 9. Numerical gates and energy interpretation

Keep P1-C's fraction tolerance exactly 256*double epsilon = 5.6843418860808015e-14. Record raw extrema; do not repair them. Preserve its existing roundoff endpoint interpretation for geometry.

For these nondimensional momentum fixtures, use:

- Local mass accounting/restriction residual: <=1e-12 of the local scale.
- Global mass/component-momentum budget residual: <=1e-11 of the global scale.
- Common-vector error: <=1e-8*Uref, with Uref=max(1,max(abs(initial transported components))).
- Branch-combination energy inequality residual: <=1e-12 of the energy scale.

Local scale is max(rho_g*dual_volume, abs(M_before), abs(M_after), sum(abs(boundary mass transfers)), abs(source)). Global scale uses initial unsigned mass or momentum magnitude, accumulated unsigned boundary transfers, and rho_g*domain_volume (times Uref for momentum). Energy scale is max(mean branch energy, rho_g*domain_volume*Uref^2). Log raw and normalized errors. These are declared numerical acceptance tolerances, not a theorem about all arithmetic.

Require all masses positive and all states finite. No mass floor, velocity clipping, alpha clipping, post-step redistribution, or artificial damping.

Record full-step kinetic energy and component extrema too. Numerical diffusion from donor-cell advection is expected; no exact energy-conservation claim is authorized. Material growth in closed, divergence-free advection needs investigation before acceptance; do not hide it or mistake the combination inequality for a complete stability proof.

### 10. Work discipline and delivery

Implement small connected pieces: flux/mass records, staggered restriction, directional momentum stages, branch combination, then the fixtures. Ordinary coding/build/indexing errors may be fixed directly; an initial failed assertion is not a reason to abandon the task.

If a failure contradicts the specified method rather than its implementation, preserve a minimal counterexample with the first incorrect operation and return it. Do not switch algorithms or start another literature search. Do not rank methods only by how many unrelated tests happen to be green.

Keep source snapshots, logs and reference hashes in a persistent prototype evidence directory outside `/tmp` and outside disposable build output. Preserve prior failure evidence. Changes to the accepted fraction core need a clear implementation reason and a before/after regression check; no reconstruction tuning.

Produce one reproducible build/run command and a results report containing actual execution counts, per-fixture results, conservation/consistency extrema, raw fraction and velocity bounds, energy diagnostics, refinement tables, timings, exact changed files and source fingerprints. Report skipped tests honestly.

Stop when P1-C-M passes or a demonstrated method contradiction blocks it. Do not continue into the next stage. No production changes, milestone tags, commits or pushes are authorized by this local prototype task; leave the tested work ready for operator review.

Success means: the compiled Judas prototype transports component momentum using its actual validated phase-volume transfers, and passes the specified independent checks. It does not mean fluid pressure, moving-body coupling, buoyancy, swimming or full P1 is solved.
