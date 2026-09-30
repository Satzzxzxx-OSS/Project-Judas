# FTFT9 hydrostatic sampler component evidence

Command (repository root):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target judas_fluid_hydrostatics_tests -j2
build/judas_fluid_hydrostatics_tests
```

Initial compiled helper run: 73 checks, zero failures; `focused.log` and `SOURCE_SHA256.json` preserve that checkpoint. Current component acceptance is the 80-check production-library run in `columns-final.log/json`; earlier failures and sources remain beside it.

## Declared numerical approximation

- Boxes use a deterministic 4×4×4 equal-subvolume quadrature. Spheres use three equal-volume radial shells and 24 antipodal angular pairs per shell. Compound children are partitioned into a nonoverlapping box union before quadrature; overlap is not counted twice.
- A particle contributes a vertical interval of length `(mass/restDensity)^(1/3)`. The lateral lattice has width equal to the maximum particle spacing; its origin is the first retained particle projected into a body-derived tangent/up basis. The body axis with largest projection perpendicular to up supplies the tangent (deterministic first-axis ties). Thus the entire frame follows rotated/translated geometry. Particle-order/reference dependence remains a finite-resolution approximation.
- Each resolved column unions its own vertical intervals, preserving disconnected gaps. The occupied fraction is intersection length with the quadrature footprint divided by footprint height. Column fractions are averaged with positive weights `width²*(1-columnDistance²/reach²)^2`, reach = body bound radius + two maximum particle spacings. Lower nearby columns vote zero; missing columns are extrapolated from resolved neighbours. The vertical search window is reach + footprint half-height.
- A column footprint overlapping the queried body's actual box/sphere/compound geometry is excluded from surface reconstruction: the absence of particles in that solid cannot establish a lower liquid surface. Sphere/footprint closest-distance and box/footprint separating-axis intersections use the same stored solid geometry. This geometric occlusion is a reconstruction boundary, not a choice of physical reaction law.
- Current velocity and latest-step acceleration use an independent continuous spatial kernel: `Vp*(1-distanceSquared/reachSquared)^2` within the same geometric reach. There is no temporal smoothing and no mass-ratio classification. The initial interval-group average was superseded after the discontinuity below was reproduced.
- Each immersed sample supplies `-rho * wetVolume * (gravity - fluidAcceleration)`; force and moment about the body origin are accumulated from the same sample. No rigid mass appears in this mechanism. The caller adds ordinary body gravity and configurable drag.
- The construction is geometrically covariant when the fixture, body orientation and gravity are rotated together. With zero gravity the body-local Y direction provides the reconstruction axis; it does not create buoyancy by itself.

This is a geometric approximate field extension, not pressure reconstruction or exact exterior momentum conservation. Surface/shoreline/overhang details smaller than the lateral reach are smeared; isolated particle intervals have an effective thickness derived from their volume. Sphere footprint and tilted box footprint intersections are quadrature approximations. Sample counts are fixed numerical defaults, not tuned per fixture. The tests below do not establish integrated container, swimming or production stability/performance.

## Independent checks

Analytical box/sphere/overlapping-union volumes; symmetric centroids; 50% box immersion despite an actual particle hole; Archimedes force; neutral and heavy net-force ordering; 100× volume scaling; two spacings (.125/.25 m); arbitrary rotated/translated equivalence; no force in stationary zero gravity/common free fall; prescribed moving-liquid velocity; off-COM compound load torque; explicit exclusion-mask behaviour; retained gap between disconnected layers; deterministic repeated queries. Hemisphere centroid uses its analytic `-3r/8` result with a declared .012 m finite-quadrature tolerance.

Timing in the log includes constructing all synthetic fields and running the 73 checks. It is not a production world-step benchmark.

## Spatial bulk-flow repair

`bulk-before` preserves a concrete defect in the initial interval-selected average: moving the query by 20 micrometres between equally fully immersed states changed acceleration by 4.905 m/s² and velocity by .5 m/s because a newly touching row received full weight. The two new continuity checks failed while the original 73 checks still passed. Physical exit 1 is retained.

`bulk-after` records the approved averaging-only correction: 77/77 checks pass, acceleration variation 8.2016e-5 m/s² and velocity variation 8.34465e-6 m/s for the same displacement. Occupancy, interval union, quadrature and force law are unchanged. Raw logs and passing current-source fingerprints are in `bulk-after.json`; original 73-check fingerprints in `SOURCE_SHA256.json` refer to the preserved initial source (`bulk-before` cpp/header and `initial-FluidHydrostaticsTests.cpp`).

## Per-column occupied-volume repair

`columns-before` preserves the cross-column union defect: two equal-area columns with .5/1 occupancy returned 1 instead of .75; three columns with 1/3, 2/3, 1 occupancy returned 1 instead of 2/3. `columns-candidate` preserves the first mean-column attempt's 15 failures: solid exclusion holes were incorrectly treated as low liquid surfaces. `columns-occlusion` preserves three remaining covariance failures caused by query-centred bin ties splitting float-rotated lattice rows. No assertions or tolerances were weakened.

The approved final operation adds explicit solid-geometry footprint occlusion and anchors the column lattice to actual particle geometry. `columns-anchored` records 80/80 standalone production-helper checks; `columns-final` records the corresponding production-library execution. The original 77 checks remain unchanged, including flat pool force, both spacings, particle holes, common freefall, zero gravity and rotated/translated equivalence. The additional independent column means and rotated column fixture pass. These are component results; integrated physical immersion still requires the separate actual-world acceptance gate.
