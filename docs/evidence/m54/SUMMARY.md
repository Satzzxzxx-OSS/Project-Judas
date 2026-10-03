# M54 candidate — conserved reservoirs and containers

Starting HEAD/main/origin/main: `16f7d59fa3289e7e7c43aaeb7e88471be6fe348f`.
No commit, push or tag. Human visual/interactive acceptance is pending.

## Architecture and change scope

- RuntimeWorld owns LiquidSystem. A material quantity has one owner: reservoir,
  vented rigid container or detached parcel. Double m³ and kg/m³, paired bounded
  transfers, deterministic owner order. Geometric error never changes ledger volume.
- Authored `.judascavity` tetrahedral physical geometry is independent of render
  appearance. Real clipping/integration produces monotonic capacity tables.
  All vertex-coordinate knots remain; cubic-between-knot residual bounds drive
  refinement. O(log N) inversion, no deep-water particle population.
- Optional GravityField equilibrium capability follows normal GravityContextMap
  selection. Uniform plane and actual constant-magnitude radial equipotentials;
  no liquid type checks, world-Y assumptions or live rebasing. Unsupported gravity
  suspends container geometry/flows without losing its owned quantity.
- Static authored basin graph: saddle threshold, bounded Torricelli-style transfer,
  equalisation guard, separate node ledgers on opening/closing. No automatic
  watershed extraction. Small containers recompute gravity-relative clipping and
  lip capacity as they rotate. Explicit opening connectivity transfers the deficit
  relative to external head; removal carries the quantity, rather than copying
  overlap each frame.
- Pouring debits the source and emits ballistic volume parcels. Existing solid
  ray queries bound receiver sampling; containers require opening crossing.
  Receiving volume credits the destination and retires the parcel. Unsupported
  solid landings retain parked conserved volume.
- Implicit bulk point queries and real box/compound submerged volume/centroid
  supply analytic buoyancy and bounded drag to ordinary PhysicsWorld bodies.
  Dry mass/inertia are unchanged; M54 does not yet raise a static surface for
  moving-solid displacement. No exact CFD momentum-conservation claim.
- Renderer owns raw GL and persistent transient-surface buffers. Cached occupied
  surfaces use normal per-camera mask/frustum policy; parcels are coarse visual
  volume markers. Presentation does not own quantities.
- Normal inspector components/bake/curve/preview, asset IDs, CPU async resources,
  scene properties, prefab overrides/spawn, Play/Stop, scene replacement and M38
  export. Bake fingerprints include source geometry/settings/gravity; stale data
  rejects startup/export. Conditional M54 bytes preserve old canonical schema-5
  fingerprints. Effective baseline includes contributing asset content, not paths.
- Public Entity.liquid / LiquidVolume / liquid queries, accounting and diagnostics;
  explicit transfers belong in fixedUpdate. Types/reference/inventory/cookbook and
  live enumeration stay synchronized. No private demo dispatch.

Two shared corrections were required: parsing liquid validation after normal body
fields, and preserving repeated compound-box/cavity entries in generic prefab
property maps. Historical PBF remains available and unchanged for old content.
Its coupling has only an explicit M54 ownership guard to prevent applying both
representations to the same body. No P1 prototype integration or navigation change.

## Ordinary current demonstration

`projects/liquid_reservoir_demo/liquid_reservoir_demo.judasproj`

- `Scenes/lab.judas`: tapered MAIN, 3392 L full capacity / 1000 L initial; storage
  and receiving basins; separate 1800 L graph-water spill experiment; real 25.088 L
  empty bucket prefab with five physical walls and an authored opening; buoyant box.
- `Scenes/radial.judas`: actual oblique planetary gravity, 4000 L capacity / 2000 L
  initial water and storage; ordinary buoyant box. Curved equipotential bake.
- Both have fixed double absolute origin `(1e12,-2e12,3e12)` with precise local
  float physics. Normal JS CharacterMotor/view/input, conservation HUD, pause,
  registered scene changes and empty-bucket spawning. Pickup/carry/tilt uses
  ordinary forces/torque; game scripts never manufacture fill or teleport water.

Controls: WASD/left stick, mouse/right stick, Space launch; Q/E transfers 800 L
out/back; F/H fills/drains spill experiment; G pickup/drop; wheel carry height;
hold P physical tilt/pour; T throw; N empty bucket; 1/2 scenes; R reload; Esc pause.

## Focused quantitative results

Final narrow logs: `final/liquid-followup.log`, `final/application-followup.log`.

- Geometry/ledger/gravity: **67 checks, zero failures**.
- Ordinary project/resource/physics/prefab/lifecycle: **41 checks, zero failures**.
- Independent irregular-volume residual: `1.19164318524e-7 m³`, within fixture
  `1e-6 m³` tolerance. Independent spherical cross-section check agrees within
  separately declared radial geometry bound.
- 1000 → 200 → 1000 L transactions and identical restored equilibrium pass in
  actual JS fixedUpdate; source/destination bounds and repeated transactions pass.
- Repeated paired ledger error: **0 m³**, tolerance `1e-12 m³` in that fixture.
- Scoop/pour: MAIN `0.974912 m³`, bucket residual `1.9261685667e-9 m³`, receiver
  `0.0250879980738 m³`, detached zero, TOTAL `1 m³`, error
  **`4.4408920985e-16 m³`**. Carry and repeat scoop preserve owned quantities.
- Spill connect/redistribute/disconnect retains separate residuals; total 1.8 m³.
- Prefab empty spawn, volume transfer, destruction retention, stale tokens,
  authored reset and Stop all pass.
- Unsupported-gravity regression: retains volume, rejects stale geometry/flow;
  returning inside the existing selected region resolves and clears error.

Conservation tolerance is `max(1e-12,64*epsilon*max(1,expected)*(owners+parcels+1))`
m³. Capacity interpolation tolerance and radial curved-geometry bound are different
quantities. The demo's 0.01 L interpolation tolerance is not a promise of 0.01 L
error against an exact mathematical sphere.

## Candidate validation (one broad gate)

- **One clean Release build, zero compiler warnings**.
- **100 production suites pass**, including existing gravity, fluids/containers,
  physics, animation/motors and navigation. **Async 12 cases / 246 checks pass**.
- Cookbook: **169 checks**, including 9 new liquid checks, zero failures.
- API: **20 exports / 185 public symbols / 119 native operations / 13 callbacks**;
  TypeScript 5.9.3, 20 example files, three negative drift checks, live VM enumeration.
- Actual editor bake and Play/Stop: authored state identical, no JS faults.
- Queued reload + lab/radial replacement: registered sources, restored quantities,
  invalid prior-world tokens, prefab destruction retention and radial query/refill.
  Same probe passes exported and moved outside the repository.
- Real demo moved to `/tmp/Judas_Liquid_Reservoir_M54`, launched from `/tmp`.
  18 packaged assets, 8,567,423 bytes (~8.17 MiB), export ~0.046 s in this run.
- Changed source bytes deliberately reject stale export. Baked asset IDs survive
  ordinary rebake. No binaries/packages/caches belong to the source change list.

The broad gate precedes the final liquid-only unsupported-gravity correction.
It was not repeated. Narrow incremental builds and liquid/application, cookbook,
API, editor/lifecycle/export checks cover that correction. Original source
snapshots, genuine failures and corrected follow-ups remain preserved. See
`development/FAILURES.md` and `final/RESULTS.json` for exact commands/results.

## Lightweight Release performance

Measurements are on this machine, offscreen GL / software rendering at 640×360,
not human desktop acceptance or a stable hardware FPS guarantee.

| Work | Measured |
|---|---:|
| Irregular MAIN bake | 42.484 ms, 1025 samples, 6 cells, 0.01 L tolerance |
| Radial MAIN bake | 1090.803 ms, 553 samples, 2029 cells, curved bound 0.0062481 m |
| Radial storage bake | 114.066 ms, 414 samples, 192 cells, curved bound 0.00914312 m |
| 1000 inversions | 0.026508 ms; amortized ~0.000027 ms each |
| 10 m / 100 m depth, 10k inversions | 0.07058 / 0.08071 ms, two samples each |
| 1 / 10 / 100 rotating container solves | 0.25702 / 2.71250 / 22.77640 ms |
| 2000 paired transfer + resolve operations | 26.4644 ms |
| Lab full fixed step + scripts | 5.3803 ms |
| Radial full fixed step + scripts | 2.2579 ms |
| Lab rendered frame mean / p95 / max | 12.651 / 21.697 / 49.554 ms (~79 FPS mean) |
| Radial rendered frame mean / p95 / max | 10.850 / 27.402 / 27.907 ms (~92 FPS mean) |

Lab measured state: six reservoirs, one container, one retained destruction
parcel, 33 rigid bodies, 51 draw calls. Radial: two reservoirs, zero containers/
parcels, 12 bodies, 23 draw calls. No deep bulk fluid particles in either scene.
100 container solves are a stress measurement and still cost ~23 ms; this is not
an unlimited-container performance claim. Depth does not multiply explicit state.

## Current limitations

- Static authored tetrahedral basins; no dynamic free surface/waves, automatic
  watershed, channels, sloshing or general nonconservative equilibrium solver.
- Radial curvature is bounded approximation; split basins across gravity zones.
  Containers assume small locally uniform gravity and freely vented air.
- Detached receiver sampling is coarse, bounded transport, not full liquid CCD;
  unsupported landings retain parked parcels and do not spread across dry ground.
- Box/compound hydrostatics; overlapping authored boxes overestimate displacement.
  No dry mass/inertia update for contained water or dynamic surface displacement.
- New ledger is session runtime state, not included in existing saved-world deltas.
- Human bucket controls/visuals and interactive feel still require operator review.

## Human checklist

1. Reload lab. Q: confirm 1000→200 L and correct lower surface; E restores both.
2. F/H: open spill redistribution, drain below saddle and inspect residual pools.
3. G/wheel: dip bucket opening, verify MAIN loss equals BUCKET gain, lift/carry.
4. Over receiving tank, hold P: watch container/detached/receiver quantities and
   constant TOTAL. Drop/throw ordinary buoyant body; inspect implicit-water support.
5. Confirm deep bulk is a surface/volume region with no particle fill.
6. Scene 2: drain/refill curved water, inspect local radial gravity/body response.
7. Reload and Play/Stop: no stale state, authored amounts restored.
8. Repeat in the moved standalone package.

## Review artifacts

- Current guide: `docs/LIQUID_RESERVOIRS.md`.
- Public API: `docs/judasjs/liquid.md`, `docs/judas.d.ts` and cookbook liquid example.
- Exact changed files: `CHANGED_FILES.txt` (includes individual evidence files).
- Final implementation/docs/project/baked bytes: `FINAL_SOURCE_FINGERPRINTS.json`.
- Protected-path/source integrity: `INTEGRITY.json`.

Proposed commit after operator approval:
`Add M54 conserved liquid reservoirs and containers`.
ROADMAP remains absent. M55/dynamic-surface work was not started.
