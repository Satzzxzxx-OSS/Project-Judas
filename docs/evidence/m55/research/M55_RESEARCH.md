# M55 research — dynamic surfaces over conserved liquid reservoirs

Research date: 3 October 2026.

## Status and purpose

This is a technical recommendation, not an implementation report or authorization to advance milestones. The latest completed checkpoint supplied in this conversation is M53, `16f7d59fa3289e7e7c43aaeb7e88471be6fe348f`. M54 has been specified, but its implementation/completion has not been inspected here. Before M55, map this design to the actual accepted M54 interfaces rather than assuming proposed API names exist.

The user requirement is explicit: no particles or dynamic 3D grid filling the deep interior. Store conserved liquid amount and occupied volume cheaply; simulate the useful free-surface motion and detached liquid. Preserve scoop/carry/pour, draining/refilling, arbitrary uniform and central radial gravity, and script-driven swimming.

## Decision

Recommend a **conservative surface finite-volume model with subgrid geometric storage, semi-implicit gravity-wave integration, and M54-owned volume transfers/parcels**.

Each surface region has local elevations and tangential flow, not vertical stacks of dynamic cells. Basin depth enters geometric capacity and the reduced physical response, not an allocation of interior particles. The dynamically active cells partition the SAME reservoir volume; they are not additional water.

M55 should deliver modest-resolution interacting waves, wetting/drying, coherent rigid-body interaction, scoop/pour integration, and one real radial-surface proof. Accurate deep-water dispersion, violently tumbling-container slosh, overturning sheets and whole-planet streaming are not prerequisites for that deliverable.

This is a proposed synthesis. No source establishes that this entire combination is already implemented or fast on Judas's hardware.

## 1. What the primary sources actually establish

**Narrow-band FLIP is not the requested model.** Ferstl et al. (2016) remove interior particles but retain simulation of the inner liquid on a regular grid. That is a useful optimization of a volume solver, not an implicit reservoir with no interior dynamic grid. [S1]

**Heightfield/particle hybrids are relevant, but are not automatically conservative.** Chentanez and Müller (SCA 2010) demonstrate surface flow combined with noninteracting detached particles. Their discussion explicitly admits that coupling and overshoot reduction do not conserve volume. Reuse the representation idea, not those repairs as a conservation guarantee. [S2]

**Subgrid hydraulic storage is an established technique.** HEC-RAS documents terrain-derived volume/elevation data per computational cell and hydraulic data per face. This separates detailed containment from the coarser grid on which flow is evolved. It supplies a strong analogue for extending M54 capacity data locally. [S3, S4]

**There is a relevant semi-implicit wet/dry literature.** Casulli's 2009 method derives a mildly nonlinear surface-elevation system from mass balance, including wet/dry transitions and subgrid terrain. The published properties belong to that method under its assumptions; a home-grown approximation does not inherit its guarantees automatically. The full original paper was not accessible here, so its publisher abstract is not the sole implementation reference. [S5]

**Topology cannot be replaced by an area/volume table.** Casulli (2019) specifically treats false cross-flow between disconnected fine regions inside a coarse cell. M55 must preserve barriers/connectivity as well as capacity. [S6]

**Curved semi-implicit surface flow has directly relevant recent research.** Tavelli and Zanotti's May 2026 preprint formulates shallow-water dynamics on two-dimensional manifolds, using an implicit surface-gradient/continuity treatment. It is a supplementary reference for curvature and metric terms, not a ready-made Judas tile-seam implementation or established benchmark on this machine. [S7]

**Shallow-water waves are not general deep-water waves.** Jeschke and Wojtan (2023) combine low-frequency shallow-water flow and Airy surface waves. Their author page includes important corrections to flow signs and dispersion equations. A future implementation must read the errata. This is an upgrade path, not justification for including an FFT decomposition project in the first M55 pass. [S8]

**Positivity and equilibrium preservation need deliberate numerics.** Audusse et al. (2004), and later wet/dry reconstruction research, provide alternatives to post-update clipping of negative depths. Do not casually combine pieces of different schemes and claim their proofs still apply. [S9, S10]

**Even recent 'optical layer' methods can retain a volume solver.** Adaptive Optical Layers (Eurographics 2026) adapts the near-surface region of tall-cell grids while retaining elongated interior cells and a projection step. It remains outside the selected no-interior-grid direction. [S11]

## 2. The precise M54/M55 ownership contract

For one material of constant density, define system quantity as:

    total volume = sum(reservoir-owned volume)
                 + sum(container-owned volume)
                 + sum(detached-parcel-owned volume)

An actively resolved reservoir additionally has a partition:

    reservoir.volume = sum(cell.volume)

The reservoir total is the control/transaction total; local cell volumes are its spatial allocation. Do NOT add both numbers to the system total. They must be synchronized through one transaction path, not independently writable APIs.

Internal face transport changes cell allocation without changing the reservoir total. Cross-reservoir, container and parcel exchanges update source and destination allocations AND their owner totals once. The accounting precision is separate from geometric and equation-solve residuals.

For a cell i, let C_i(s,t) be the available liquid volume below surface coordinate s, clipped to the basin and excluding relevant solid volume at time t. Then:

    V_i = C_i(s_i,t)
    s_i = inverse(C_i, V_i, t)

For a simple flat cell, C_i(s)=A_i*max(s-bed_i,0). For irregular terrain, use the geometry-derived curve/integration. For radial geometry, use the appropriate spherical-volume measure.

M54's equilibrium inverse of the whole basin remains useful for initialization and diagnostic reference. Once a surface is active, it must NOT flatten all cell surfaces back to that equilibrium every step. Local transport is precisely what allows waves and delayed redistribution.

The old research report's suggestion to make every surface displacement have zero mean is insufficient. See section 8 and the accompanying independent counterexamples.

## 3. Data M55 actually needs from geometry

A whole-basin capacity curve does not tell a local wave solver where to send water. The surface bake needs:

- A local capacity relation for each connected computational region.
- Actual cell area/metric and world-to-surface mapping.
- Face lengths, wetted aperture/cross-section versus level, and bed/barrier information.
- Explicit adjacency and boundary conditions.
- Correspondence with M54 basin/connection identities and source fingerprints.

Disconnected fine regions must not be fused into one cell just because they share a bounding square. Split the computational region, refine, or reject an unsupported bake. A narrow wall remains impermeable until its real top/opening is reached.

Start with fixed resolution per surface and compatible neighboring patches. Do not make arbitrary adaptive meshing and resolution transitions mandatory for M55. Render resolution can be independent of simulation resolution.

## 4. Conservative update and the recommended integrator

Orient each interior face once, from cell a to cell b. Store one signed discharge Q_f in cubic metres per second. Every accepted step uses the same integrated face transfer with opposite signs:

    delta_f = dt * Q_f
    V_a -= delta_f
    V_b += delta_f

More generally:

    V_i(next) = V_i(now) - dt * outward_face_discharge_sum(i)
                          + accepted_external_volume_change(i)

This is the discrete conservation structure, not a complete wave solver. Gravity/surface gradients evolve flow; velocity advection and friction require their own defined approximation. Closed boundaries have zero normal discharge; open/spill boundaries use explicit receiving ownership.

For constant local gravity, a simplified pressure step has the form:

    u_f(next) = u_f(predicted)
                - dt * g * (surface_b(next)-surface_a(next))/face_distance

Substitution into continuity produces a **two-dimensional surface-elevation solve** involving cell storage and face flows. This is not a 3D pressure solve below the surface. The published semi-implicit methods provide the appropriate nonlinear storage/wet-dry treatment. The line above alone is not enough to implement dry cells safely.

Recommended initial implementation properties:

- Staggered face-flow storage and monotone cell capacity relations.
- Semi-implicit gravity/continuity step; a documented conservative low-order transport step initially.
- Lake-at-rest preservation on non-flat terrain.
- Nonnegative cell volume through the numerical method, not `max(V,0)` afterward.
- Named physical/numerical damping, not scene-specific correction constants.
- Explicit residual/convergence reporting. A failed solve must not commit half a transfer or silently delete water. Use bounded retry/substeps or report failure while retaining coherent state.

A positivity limiter applied to shared outgoing fluxes can preserve volume; clipping an already negative cell cannot. Which limiter/reconstruction belongs in the chosen full method must be settled as part of its discretization rather than bolted on after tests fail.

## 5. Depth, wave speed and what 'top inch' does not mean

The user is right that deep water need not be populated by dynamic samples. However, a literal one-inch slab with a fake rigid floor underneath would model a different body of water.

Shallow-water theory uses the actual effective depth H and predicts long-wave speed:

    c = sqrt(g*H)

Linear gravity-wave theory for a flat bottom instead gives:

    omega^2 = g*k*tanh(k*H)

Here k is wave number. The shallow-water approximation is a wavelength/depth assumption, not a rule that the computer must simulate only a thin physical sheet. [S8]

An explicit solver can therefore require smaller steps as H grows, even with an unchanged number of cells. Semi-implicit treatment removes the explicit gravity-wave stability bottleneck. It does NOT guarantee depth-independent accuracy, iteration count or execution time. It also does not make shallow-water waves obey deep-water dispersion.

M55's honest claim should be:

- No vertical dynamic grid/particle population scales with depth.
- Baseline surface-flow model is a declared hydrostatic/depth-reduced approximation.
- Timestep choice is not forced solely by an explicit sqrt(gH) wave CFL limit.
- Wave damping/phase behavior is measured for the demonstrated scale.
- Accurate short deep-water/capillary waves can be added as another surface contribution later without changing the volume ledger.

Do not adopt silent effective-depth caps solely to achieve a frame target. A deliberately reduced wave-response model is possible, but must be named and validated as such rather than presented as the unchanged physical equations.

## 6. Rigid bodies: displacement, forces, and no double application

An object entering water removes available SPACE, not liquid mass. It must not call a 'spawn water volume' operation to produce a splash.

Represent its occupied intersection consistently in C_i(s,t) and, where needed, face apertures. As a body enters, unchanged liquid volume occupies reduced available space, driving local elevation/flow changes. Rasterization/clipping is an approximation and requires a focused enter/exit test to catch duplicate displacement.

The exact moving-boundary discretization is the largest remaining integration risk. A capacity change is necessary but not by itself a complete robust fluid-solid method. It must be reconciled with boundary flow and the selected timestep; do not claim general high-speed conservative coupling from this design sketch.

Keep one generic buoyancy/drag owner. Use the authoritative dynamic surface for submerged geometry and fluid-velocity inputs, so objects respond to the water that is visibly there. Do not add a second full M55 buoyancy force on top of M54's force. If wave-induced reaction is approximated, state that explicitly.

Do not claim exact combined momentum/energy conservation from hydrostatic buoyancy plus approximate drag. Require bounded, credible behavior and no spontaneous energy explosion in the chosen fixtures.

A moving-surface normal is NOT automatically opposite gravity. Keep local gravity, actual wave normal and solid contact normal distinct. Use the wave normal for surface geometry/optics; do not redefine Judas gravity from it.

## 7. Cups, pouring and detached liquid

Retain the M54 physical opening/connectivity rule. Scoop against the **local current surface**, not merely the whole reservoir's equilibrium level. The accepted scoop volume removes liquid from the local donor cell(s) and reservoir ownership once and adds it to the container once.

For a pour:

    container loses dV
    -> parcel owns dV during flight
    -> receiver gains dV on actual interception

The receiving local surface cells receive both volume and a documented momentum/disturbance contribution. Do not put the liquid in the receiver immediately and also keep a massive parcel in flight.

Use the same parcel mechanism for a limited splash extension. Every physical droplet carries volume and has a budgeted emission/rejoin transaction. Its ejection velocity should be tied to the disturbance with a stated energy/impulse budget, not arbitrarily huge kicks. Density converts volume into mass where required.

Foam/bubbles may be massless effects; clearly distinguish them from transported water. Reaching a particle budget must retain/coarsen the liquid or suppress emission, not erase it. Small residual volumes need an accumulator or other explicit owner.

M55 need not recreate SPH for detached droplets. Ballistic parcels with ordinary collision/gravity are enough for the initial transport proof; sheets, merging droplets and full waterfall fluid dynamics can remain unsupported.

**Small vessels are in scope conceptually.** A small fixed basin uses fewer surface cells and the same ownership rule. For moving cups, preserve M54's orientation-aware equilibrium clipping and real transfers. Fully dynamic sloshing while the cavity rotates is a distinct moving-boundary/remapping problem. Do not attach a flat wave patch rigidly to a tipping cup and call it physically correct. M55 can honestly demonstrate scoop/carry/pour with quasi-static internal cup liquid while deferring violent slosh, without introducing deep particles.

## 8. Radial geometry and volume correction

For supported central gravity, equilibrium surface coordinate can be radius or consistent potential. Neighbor transport must use actual metric lengths/areas and transform tangential vectors between frames. A rotated planar grid is not a curved planetary surface.

For illustration, a cell spanning solid angle Omega with constant floor radius b and surface radius r contains:

    V = Omega*(r^3-b^3)/3        when r > b
    V = 0                      otherwise

For a varying floor, integrate this expression over the wet angular domain. The formula is geometry, not a complete curved hydrodynamic discretization.

Consequences:

- Equal positive/negative radial height offsets do not generally preserve volume.
- Wet/dry changes break simple arithmetic-mean elevation corrections even on flat terrain.
- Reconcile surface geometry through C_i and conserved fluxes, not global subtraction of a mean.
- Keep disconnected pools separate; do not solve an accounting discrepancy by secretly moving liquid across a dry ridge.

Start with one resolved planetary lake/cap. If it uses multiple patches, internal seams need one shared signed volume flux and consistent vector transport. Co-located vertices alone prove visual continuity, not conservation.

The curved depth-reduced dynamics still has a validity range: deep water comparable to the planetary radius and arbitrary non-conservative/changing gravity are not covered by a thin-shell approximation. Unsupported fields should be explicit, never fallback world-Y.

## 9. Fixed simulation, presentation and underwater rendering

Expose separate semantics for:

- authoritative surface/submersion/flow samples at simulation time;
- presented/interpolated surface samples for rendering/camera effects.

Reuse one data source and its time snapshots. Do not repeat the M52 camera/model timing bug by rendering interpolated waves while visual effects or attached presentation sample another phase. Rendering cannot write back authoritative volumes.

A minimal underwater feature can use the actual liquid entry/exit boundary along a view ray and water path length L. Homogeneous transmittance has the form:

    T_channel = exp(-extinction_channel * L)

A game approximation may blend attenuated scene color toward an in-scattering tint. Full volumetric light transport is unnecessary, but camera DEPTH alone is not optical path length. A nearby object and distant object should not receive the same attenuation merely because the camera is one metre underwater. [S12]

Mask the actual water-crossed part of the image near the waterline rather than tinting the entire screen when the camera centre barely dips. Normals/normal-map detail and optional simple Fresnel/refraction can improve readability without adding dynamics. Reflections, caustics, acoustic simulation and advanced post-processing should not gate the surface solver.

## 10. CPU, GPU, sleep and scheduling

Prefer modest CPU-authoritative surface updates initially, with Renderer-owned GPU drawing. This avoids making M55 contingent on graphics-backend migration or synchronous GPU readback for scooping/buoyancy. Compute shaders entered core OpenGL in 4.3, whereas the reported Judas baseline is 3.3. GPU acceleration can be a future backend capability, not an assumed primitive. [S13]

Use fixed simulation steps and previous/current surface snapshots. Resource jobs may bake/prepare immutable data. Parallel independent updates need explicit deterministic joins before transactions/query consumers; fixed dt alone is not proof of determinism.

Visibility controls DRAWING, not water existence or required physical simulation. A calm region can sleep under explicit conditions: no sources, no relevant interaction, no incoming flux/disturbance, and a retained coherent state. It must wake when neighboring flow or external activity requires it. Do not freeze offscreen draining, pouring or floating objects.

Avoid full adaptive mesh refinement in the first implementation. Fixed surface resolution, bounded parcels, cached geometry and physically conditioned sleeping offer a simpler measured starting point.

Measure actual Release application costs on the development machine, including warmup separately, fixed-step catch-up, geometry updates, rendering, queries and transfers. Compare 10 m/100 m depth with the same surface resolution, interactions and settings; report wave fidelity as well as cost. These research sources do not establish a Judas FPS result.

## 11. Independent arithmetic/numerical checks performed here

The accompanying `sanity_checks.py` was executed in an isolated Python/SciPy environment. It is NOT a Judas implementation, full nonlinear solver, hardware benchmark, or M54/M55 acceptance test.

1. Three planar one-square-metre cells at a wet/dry shoreline: zero-sum height offsets changed geometric volume from 1.5 to 1.4 m3. This directly disproves a universal zero-mean-displacement conservation rule.
2. Two spherical sectors, radius 10 m, one steradian each, displaced +0.1/-0.1 m: the volume increased by 0.2 m3 despite zero mean radial displacement. The exact expansion agrees with numerical arithmetic.
3. A closed 128-cell fully wet linear surface-wave model was advanced for 600 steps at 1/30 second with depths 1, 10 and 100 m. Maximum volume drift was below 1e-12 m3; all states stayed finite with no energy increase. The deepest case exceeded explicit gravity-wave Courant one, but showed much stronger numerical damping. This demonstrates why stability is not a substitute for accuracy or 'looks good' acceptance.

No wet/dry NUMERICAL algorithm, moving-body coupling, curved-wave dynamics or rendering was validated by that experiment. The shoreline and spherical calculations above are geometric counterexamples only.

## 12. Recommended milestone boundary and acceptance

M55 should mean **dynamic interacting surfaces over accepted M54 reservoirs**, not 'replace every fluid feature and simulate every ocean phenomenon.'

Suggested ordered implementation gates within the ONE milestone:

1. A calm irregular basin remains at rest, with conserved volume and valid wet/dry boundaries.
2. A local disturbance propagates/reflects and changes the authoritative surface sampled by physics.
3. Drain/refill modifies local volume without erasing existing waves or resetting the whole mesh.
4. Existing M54 scoop/carry/pour works against that live surface, including conserved parcel landing.
5. Ordinary moving bodies displace water without mass injection, double buoyancy or explosive reaction.
6. The radial-cap case exercises real curved geometry and no world-up assumptions.
7. Presented surface/underwater transition uses the correct interpolation timeline.
8. The real interactive demo, export, and scripting/API checks pass at practical measured cost.

Use small independent analytical fixtures for accounting, rest, shoreline and displacement, then a compact flat-pool/small-container/radial-lake interactive project. One final production run if implementation changes warrant it; focused checks during development and narrow follow-ups. No reopening protected P1 or old fluid benchmarks merely for ceremony.

Document failures rather than papering them over. If the chosen wet/dry/moving-solid combination fails the small proof, stop expanding to the full demo until the mechanism is resolved. Do not accumulate arbitrary repairs until one hand-tuned scene behaves.

## 13. What remains uncertain

- The exact current M54 implementation and geometry/transfer APIs have not been supplied after authorization.
- Published schemes do not automatically validate Judas-specific geometry, curved patches or moving-body coupling.
- Actual CPU budget/conditioning and how much damping looks acceptable need small implementation measurements and human review.
- Fully dynamic moving-container slosh and accurate short deep-water waves are deliberately not promised by the initial model.
- No drop-in, production-ready, Judas-compatible library implementing this whole combination was established. Algorithm references are not an excuse to vendor an entire unrelated hydrology application.

These are bounded implementation risks, not a reason to commission another broad literature survey.

## Primary sources

[S1] Ferstl, Ando, Wojtan, Westermann, Thuerey (2016), *Narrow Band FLIP for Liquid Simulations*. TUM author/project page. https://www.cs.cit.tum.de/en/cg/research/publications/2016/narrow-band-flip-for-liquid-simulations/

[S2] Chentanez & Müller (2010), *Real-time Simulation of Large Bodies of Water with Small Scale Details*, Eurographics/ACM SIGGRAPH Symposium on Computer Animation. Author-hosted full paper; see especially the volume limitation in Results and Discussion. https://matthias-research.github.io/pages/publications/hfFluid.pdf

[S3] USACE HEC-RAS, *2D Modeling Advantages/Capabilities*, sections on implicit finite volumes and detailed hydraulic property tables. https://www.hec.usace.army.mil/confluence/rasdocs/r2dum/6.0/introduction/hec-ras-2d-modeling-advantages-capabilities

[S4] USACE HEC-RAS, *Hydraulic Property Tables*, cell storage and face geometry. https://www.hec.usace.army.mil/confluence/rasdocs/hecras/beta/technical-reference/hydraulic-property-tables

[S5] Casulli (2009), *A high-resolution wetting and drying algorithm for free-surface hydrodynamics*, IJNMF 60, 391-408. DOI 10.1002/fld.1896. Publisher/author-repository abstract inspected; full original article not obtained. https://onlinelibrary.wiley.com/doi/abs/10.1002/fld.1896 ; https://iris.unitn.it/handle/11572/79798

[S6] Casulli (2019), *Computational grid, subgrid, and pixels*, IJNMF 90, 140-155. DOI 10.1002/fld.4715. Publisher abstract on connected subgrids and cell/edge clones. https://onlinelibrary.wiley.com/doi/abs/10.1002/fld.4715

[S7] Tavelli & Zanotti (2026), *A semi-implicit two dimensional solver for a covariant formulation of the shallow water equations*. arXiv PREPRINT v1, 25 May 2026; not represented here as an established peer-reviewed production solution. Full text consulted. https://arxiv.org/abs/2605.25544 ; https://arxiv.org/html/2605.25544v1

[S8] Jeschke & Wojtan (2023), *Generalizing Shallow Water Simulations with Dispersive Surface Waves*, ACM TOG 42(4), article 83. DOI 10.1145/3592098. Author page includes essential ERRATA. https://visualcomputing.ist.ac.at/publications/2023/GSWSDSW/ ; https://research.nvidia.com/publication/2023-08_generalizing-shallow-water-simulations-dispersive-surface-waves

[S9] Audusse, Bouchut, Bristeau, Klein & Perthame (2004), *A Fast and Stable Well-Balanced Scheme with Hydrostatic Reconstruction for Shallow Water Flows*, SIAM JSC 25(6), 2050-2065. DOI 10.1137/S1064827503431090. https://epubs.siam.org/doi/pdf/10.1137/S1064827503431090

[S10] Bollermann, Chen, Kurganov & Noelle, *A well-balanced reconstruction for wetting/drying fronts*, arXiv 1412.3580. https://arxiv.org/abs/1412.3580

[S11] Narita & Kanai (2026), *Adaptive Optical Layers: Efficient Tall Cell Grids for Liquid Simulation*, Computer Graphics Forum 45(2), e70357. DOI 10.1111/cgf.70357. Author project abstract inspected. https://graphics.c.u-tokyo.ac.jp/projects/Adaptive-Optical-Layers/

[S12] Pharr, Jakob & Humphreys, *Physically Based Rendering*, fourth edition, section 11.2, Transmittance. https://pbr-book.org/4ed/Volume_Scattering/Transmittance

[S13] Khronos, *Khronos Releases OpenGL 4.3 Specification with Major Enhancements* (2012), compute shader and shader-storage additions. https://www.khronos.org/news/press/khronos-releases-opengl-4.3-specification-with-major-enhancements

---
Research recommendation only. M55 implementation and acceptance remain separate.

Milestone 55 — Judas learns that water has a surface.
