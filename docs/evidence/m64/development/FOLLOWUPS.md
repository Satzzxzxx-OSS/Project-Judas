# Genuine development observations and corrections

- Original first-build error is retained in `first-build.log` (test compared a
  handle as a scalar). QuickHull output initially had inward faces under the
  selected winding convention; the cooker now normalizes outward winding before
  calculating mass. Independent tetrahedron/cube mass fixtures pass.
- Concave sweep evaluation initially used the nearest nonapproaching patch as if
  it separated the whole mesh. Two disconnected patches independently reproduce
  the miss; each swept-BVH candidate now advances independently. Normal bounded
  convergence policy remains; exhaustion is an error, not a miss.
- `audio-one-sided.log`: audio fixture wall missed a back-facing ray because the
  wall was authored one-sided. Corrected current wall is explicitly two-sided;
  opening versus material obstruction then passed. No audio special case.
- `scene-name-rejection.log`: the reconstructed scene test used a basename rather
  than the normal registered `Scenes/*.judas` identifier. Corrected content/test
  requests the registered path; scene transitions then passed.
- `preflight/probe.log`: forcing X11 inside the sandbox failed SDL initialization.
  The following `preflight-native` run used authorized desktop execution. Earlier
  development launches with automatic driver selection are not native-GL proof.
- `query-before.log` and `query-after.log`: a mesh sphere/capsule evaluator did an
  unnecessary full-face containment pass. Containment is now convex-only. The
  identical 12,800-triangle workload falls from ~220/218 ms to ~1.4/1.7 ms for 200
  queries. Point samples also use direct point/triangle geometry, not degenerate
  segment/triangle work. Solver/cadence/mesh quality were not reduced.
- `overlapping-compound-stress.log`: the first 12-body performance configuration
  placed asymmetric compounds 3 m apart despite their >3 m combined width.
  Initial interpenetration created expensive tumbling/impact work (~349 ms median
  step). Retained as a stress observation, not a quiet-contact benchmark. The
  nonoverlapping representative comparison uses 6 m spacing, the same bodies,
  geometry, gravity, contact/solver settings and timestep.
- The first lab reused a 169-node drape and larger soft beam, costing ~24–28 ms
  fixed steps (M56: deformable rigid contacts dominated). Current lab uses small
  original 5x5 cloth / 2x2x2 soft assets and real rigid fracture cells, as required
  for this integration demonstration. Material/iterations/substeps are unchanged;
  no M62/M63 algorithm or old project was reduced. All original observations and
  the subsequent measurements are retained.

- `final/` clean build reached 73% without compiler warnings/errors, then the
  historical runner's 1,200-second build timeout expired before any tests ran.
  No owned compiler processes remained. `final-resumed/` continues the same clean
  build tree with a 3,600-second build-only allowance, frozen implementation, and
  a fresh output directory. Only the M64 runner changed; the resumed source
  snapshot records that tooling change. Production/async suites still run once.

- The single production/async run preserved in `final-resumed/` completed
  132 production suites: 130 passed, one primitive impact-timing regression
  failed two exact-motion assertions, and save-storage exited before checks
  because the M64 adapter used an invalid scratch-directory basename. Async
  passed 12 cases / 246 checks, with frozen-source/protected-path checks passing.
  Storage is rerun only with its required `m61-storage*` basename.
- The new oriented-child path normalized float quaternions even for identity
  children, perturbing the proven primitive contact/anchor arithmetic. The
  identity path now retains the original arithmetic/prepared transforms exactly.
  The original 89-case / 3,197-check failure log remains unmodified.
- `backside-before.log` records a real two-sided concave/convex crease error:
  the backside kept front-side active-edge classification. Reversed authored
  winding changed the same box's normal from (-0.98149,-0.162137,0.101922) to
  (-0.447214,-0.894427,0), and separation from -0.17882 to -0.0410227 m.
  Backside classification now uses the adjacent physical face on that side;
  convex/concave roles exchange, coplanar diagonals stay inactive, and boundaries
  remain active. A small deterministic reversed-winding fixture is permanent.
  Queries still use geometric SAT axes, not solver edge suppression.
- These narrow corrections occur AFTER the frozen broad gate. Its results and
  source snapshots are retained; only affected checks and final desktop/export
  proof are rerun. The package proof also records real frame intervals/FPS and
  identifies the visible shipping executable accurately. No full-suite repeat.
