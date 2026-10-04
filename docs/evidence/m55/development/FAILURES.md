# Preserved development failures and corrections

These are development records, not replaced historical M54/FTFT results.

- `core-first.log`: expected an untouched distant cell to be perfectly unchanged after an implicit wave step; the implicit solver couples the connected domain. The corrected assertion checks propagation/volume rather than an explicit-solver assumption.
- `transfers-first.log`: an empty dynamic receiver's amount-only policy intentionally accepts one local cell, not its whole basin at once. The corrected test supplies a spatial/connected allocation consistent with the documented policy.
- `radial-bake-first-failure.log`, `radial-refined-topology-failure.log`, `rotated-topology-first-failure.log`: topology certification on nonconforming quadrature fragments falsely rejected connected authored geometry. Certification now uses original physical topology before numerical refinement, with conservative unsupported-cell rejection.
- `aperture-rim-first-failure.log`: sampling bucket rim corners queried solid wall boundaries. Opening exchange now probes the actual interior of the opening polygon.
- `storage-profile-before.log`, `radial-performance-first.log`: rebuilding refined capacity curves and repeatedly tetrahedralizing wall subtraction caused severe costs. Cached exact local cubic storage and one final union tessellation replace those redundant operations.
- `derivative-sweep-rejected.log`: an attempted global derivative accumulation lost precision. It was rejected. The retained method uses bounded interval-local polynomial coordinates and a positive simplex-spline recurrence.
- `normal-physics-storage-profile-before.log`: corrected application probe advances ordinary physics each liquid step. Its earlier fixture-only liquid updates accumulated unapplied forces; that earlier setup is not a valid normal-loop performance result.
- `solid-tessellation-profile-before.log`: sequential wall re-tessellation grew one cell to about 29,000 tetrahedra. Batched union subtraction, cached stationary geometry and local query broadphase remove this growth without omitting solids or reducing cadence.

Follow-up core/application/final records retain the actual results. Human visual/interactive review remains separate.

- `splash-fixture-first.log` / `splash-fixture-trace.log`: manually registered owners did not set RuntimeWorld's authored-liquid shortcut flag. The corrected fixture invokes LiquidSystem's ordinary update directly; the emission method itself was not changed.
- `radial-offscreen-catchup-before.log`, `radial-native-before.log`, `render-phase-before.log`: slow geometry queries/rendering caused real outer-loop catch-up, including on the native desktop driver. Optical plane/bounds caching, a local tetrahedron BVH, cap-only extraction and cached face adjacency address measured redundant work. These failing performance probes remain distinct from final measurements.

The splash follow-up also exposed a second fixture assumption: a component-free legacy scene object is not necessarily a live runtime entity. Synchronization correctly released the manually registered owner as a retained parcel (32 m³), which the first assertion mistook for a splash. The corrected fixture authors a normal liquid component so its owner is registered; the actual bounded splash is about 5 L, and reception returns that exact amount. `splash-owner-first-failure.log` and `splash-owner-followup.log` retain this correction.

- `m55-storage-profile.log` / `m55-radial-storage-profile.log`: separated phase timing isolated clipping and exact capacity construction, rather than the pressure solve, as the remaining fixed-step costs. These probes contain temporary diagnostic output; the instrumentation was removed.
- `m55-native-flat-bvh.log` / `m55-native-radial-bvh.log`: adding a top-level optical cell BVH improved flat rendering but did not by itself resolve radial costs. The radial result is not claimed as acceptable final performance.
- `m55-radial-wall-alignment.log`: decimal float wall transforms extended microscopically inside intended cavity boundaries, forcing static clipping. Exact binary-representable wall placement removes those unintended slivers without shrinking the lake or changing water quantity. Dynamic clipping remained expensive.
- `m55-native-radial-vertex.log`: the first existing-vertex fan omitted explicit incident-face rejection. Floating-point near-zero incident tetrahedra survived initial clipping and became degenerate during radial subdivision; the normal bounded asset reader correctly rejected the bake. Incident faces are now skipped geometrically before tetrahedralization. Core volume/union/displacement checks and actual bake/load follow-up cover the correction. No reader tolerance was relaxed.

- `../final/transitions-timeout.log`: the disposable transition probe replaced a module without declaring the scene's existing `radial` inspector property. M40 correctly isolated the resulting properties error, so the probe never ran. The corrected probe declares the same boolean property. This is validation fixture authoring, not an engine transition/physics failure; only the transition/export/stale-bake tail is rerun.

- `m55-native-radial-after-cup.log`, `m55-native-radial-after-cup-cache.log`,
  `m55-native-radial-final-interaction.log`: heavier real-cup interaction revealed
  costs hidden by an initial lake-only loop. Exact geometric caches and sparse
  changed-aperture staging reduce the measured work. Final timings, including
  tails and the remaining moving-solid cost, are reported separately.
- `../followup/api-missing-enumeration-first.log`: the liquid-only cookbook selector
  exits before creating the full runtime-enumeration artifact. The API checker
  correctly failed on a missing file. The corrected invocation executes the full
  21-example cookbook and obtains fresh live enumeration; no binding changed.

- `../followup/package-world.png`: the historical fixed-only TestHarness runs
  simulation without JS update/presentation camera or runtime HUD hooks. Its
  blank image is not valid visual startup evidence for this modern project.
  The optional normal-loop capture in LiquidSurfaceLoopTests uses Application's
  actual frame lifecycle and moved project resources; the actual exported binary
  also passes the separate normal-loop scene/reload/session probe. No runtime
  semantics were changed to accommodate the harness.
