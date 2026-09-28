# Actual-engine cache validation

Build and run from the repository root:

```
cmake -S . -B build
cmake --build build --target judas_contact_cache_tests judas_contact_geometry_tests -j4
build/judas_contact_cache_tests
python3 docs/evidence/ftft4/geometry_oracle.py \
  --binary build/judas_contact_geometry_tests \
  --output docs/evidence/ftft4/performance/geometry-final
build/judas_contact_geometry_tests --player
build/judas_contact_geometry_tests --invalid
```

`ContactCacheTests.cpp` compares actual prepared/cached results with the retained
uncached production calculation. This is equivalence/invalidation coverage, not
an independent physical oracle. The unchanged 823-case rational oracle supplies
the latter separately.

The cache test exercises:

- All 823 represented-input fixture rows / 887 primitive pairs with prepared and
  uncached contact paths; manifold classification, signed separation, normals,
  anchors, feature witnesses and presentation points agree bitwise.
- 1,802 direct prepared/uncached bounds comparisons across scales 0.001–1000,
  translations through 1,048,576, exact orientation changes and compound offsets.
  It preserves the original bound-expression association and compares binary32
  endpoints bitwise, including signed zero.
- Exact orientation matching rejects a one-ULP quaternion change. A deliberately
  stale preparation hint is rejected and recomputed, with explicit telemetry.
- A read-only `PhysicsWorld` inspection hook returns caches already maintained
  by real creation/ResetBody/Step paths. The hook does not prepare or refresh them.
  Tests compare current and previous keys/bounds with actual poses and uncached
  geometry, and check the real broadphase proxy contains both relevant poses.
- Static door translation, actual and one-ULP rotations, destruction and reuse of
  the same slot with different shape types, and 512-body vector growth.
- Free rotating integration, actual penetration correction, compound children,
  and 32 contact points sharing one static support. Solver frame preparation is
  bounded by two frames per contacted body (velocity and post-pose phases).
- Eight actual player sweeps pin previous/current poses or follow both linear
  trajectories. Independently derived face distances distinguish an old departing
  wall, its new pose, an approaching wall, and a quarter-turn orientation change.

No thresholds from the geometry oracle are changed. Tests use the established
player query's binary32/sampled tolerance for that separate path. Cache memory,
allocations, hits and rebuilds are recorded rather than excluded from evidence.
No sleeping, impact target, solver iteration count or contact-selection policy
is altered by these tests.
