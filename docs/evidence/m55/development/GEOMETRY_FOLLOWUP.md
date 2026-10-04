# M55 geometry and storage follow-up

The first swimming-sized integration probe stalled in moving-solid storage table construction. Narrow profiling measured 109 tetrahedra / 16,905 samples in 4.806 s and 552 tetrahedra / 14,198 samples in 17.015 s for individual changed cells. These probes were terminated, not counted as passed. Repeated geometric clipping during dense adaptive table refinement was the measured cause.

The correction computes each tetrahedron's linear-coordinate capacity with a positive simplex-spline recurrence. Local storage uses its actual piecewise-cubic intervals between physical vertex coordinates. This replaces repeated polygon clipping and approximation refinement; it does not lower liquid cadence, change the cavity, cap depth, or edit owned volume. Independent clipping/inversion checks cover the replacement.

The larger radial bake also exposed a topology validator defect: tiny faces introduced by radial quadrature refinement were interpreted as physical internal walls. Topology is now validated on clipped original authored cavity tetrahedra, before numerical refinement. The numerical geometry still supplies the curved storage approximation. Disconnected/nonconvex coarse cells are explicitly rejected. Refinement fragments no longer define authored topology.

Final results are recorded separately after rerunning the affected checks. These records are development evidence, not candidate acceptance.

## Reduced physical tessellation and query work

The later desktop probe isolated capacity construction from clipping. Convex clipped fragments now fan from an existing boundary vertex instead of inserting a centroid. Incident faces are skipped explicitly: their floating-point near-zero tetrahedra must not be refined as physical storage. This reduces subdivisions of the same physical union, rather than lowering grid resolution, water depth, cadence or coupling. The actual bounded resource decoder was exercised after rebaking.

Optics cache actual tetrahedral planes and local BVHs; a small per-camera top-level bounds tree rejects unvisited cells. Immutable body-loading envelopes also cache bounds and plane equations. Raw GL remains in Renderer. Native before/follow-up logs are separate from offscreen and synthetic-clock checks.

## Post-candidate interaction follow-up

The native post-scoop/carry/pour radial probe initially measured 18.489 ms median
fixed-step time (21.681 p95, 22.907 maximum). The pressure solve was not the main
cost: the last liquid step spent 7.514 ms in storage, 3.977 ms in opening/container
work and 2.640 ms in body loading. Exact face-head area memoization, staging only
changed apertures, cached cap-edge topology and avoiding non-overlapping interval
tree recursion remove repeated work without changing the geometry or arithmetic
model. The final same probe measured 13.364 / 15.797 / 16.116 ms. It retained
28,727 available tetrahedra, the same 65 m³ lake and 192 cells; no lower cadence,
volume edit, skipped interaction or relaxed decoder/solver tolerance was used.

The single clean Release/production gate preceded these narrow cache changes.
Its fingerprints/results remain preserved. The affected core (52), M54 (67),
normal application (20 per scene), cookbook/API, real-loop, editor and moved-export
checks were refreshed in `../followup/`; the broad gate was not repeated.
