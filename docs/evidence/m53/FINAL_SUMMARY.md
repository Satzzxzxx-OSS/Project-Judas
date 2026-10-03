# M53 candidate — navigation and expanded Spring Range

Starting HEAD/main/origin-main remains `3c52b13765a0721fa6a1fea0038505326359326b`.
Nothing was committed, pushed or tagged. Human acceptance is pending.

## Architecture / scope

RecastNavigation v1.6.0, zlib, pinned/vendored unchanged (47 upstream files verified;
full license included in exported runtime notices). Recast is editor/tool-only;
Detour/TileCache/avoidance are runtime. RuntimeWorld owns mutable state; async
ResourceManager shares immutable layer data. Renderer alone draws debug lines.

Explicit per-surface frames, 62 stable navigation areas and 64 lifetime profile IDs,
collision-source baking, area/block/source-exclusion modifiers, tiled `.judasnav`
assets, complete/partial/failed queries, guidance-only agents, incremental box/cylinder
obstacles and script-owned link handoff. No navigation transform integration and
no native EnemySystem/AI rules. Scene/prefab normal property path and conditional
canonical schema-5 fingerprints; project format 1 and scene format 3 remain unchanged.
Stale bakes are rejected in runtime/export; authored runtime routes are transient.

Spring Range retains its actual M52 shooter and hinged targets. Expanded 48×64 m
arena, corridors/corners/ramp, one weighted region, independent landing surface,
one toggled navigation/physical blocker, six independent navigator prefabs and
runtime spawning. `enemy.js` owns chase, destination cadence, two-hit health, defeat
and scoring. Steering feeds CharacterMotor; sensor children use authoritative poses.
A hover crossing is explicit project velocity/gravity-scale behaviour, not a teleport.

## Validation

- One clean Release build: 344.196 s, zero compiler warnings.
- Production: **98 suites passed once**. Async: **12 cases / 246 checks**, zero failures.
- Initial navigation: 74 checks; game: 21; current CharacterMotor: 32;
  current shooter: 36; presentation regression: 44. All passed.
- Real-VM cookbook: **161 checks**, zero failures; 19 unique example files
  (contact example also executes as a sensor fixture).
- API drift/live enumeration and TS 5.9.3: **18 exports / 171 public symbols /
  109 native operations / 13 callbacks**; three negative drift probes passed.
- Actual inspector bake/Play/Stop passed; authored scene identical after Stop.
- Queued reload -> registered alternate scene -> return passed in normal standalone
  and a moved exported package; old entity/navigation handles rejected safely.
- Narrow follow-up: **77 navigation checks / 26 game+lifetime checks**, zero failures.
  Direct destruction invokes all six navigation cleanup callbacks safely.
  Offset/root-pivot checks use the normal motor settings API and do not move entities.
- Actual sixth-agent link crossing completed through CharacterMotor (`x=20.9815`).
- Source change produced a clear stale-bake export rejection.
- Final package: **20 registered assets / 1 scene / 8,347,357 bytes** (~7.96 MiB).
  Re-export 0.047 s; current binary launched from `/tmp`, outside package/repository CWD.

The production gate precedes the narrow navigation-only teardown/modifier/foot-progress
corrections. Affected Release targets, navigation/game/API/example/editor/export paths
received narrow reruns. The production and async suites were not repeated.
Original failures and corrected follow-ups remain in development/lifetime-followup.

## Performance (Release, current machine)

| Measurement | Result |
| --- | --- |
| Arena bake | 88 tile layers, 180 polygons, 34.50 ms |
| Landing bake | 1 layer, 1 polygon, 1.59 ms |
| Runtime with blocker | 89 layers, 186 polygons; one authored blocker cached per surface |
| 1000 simple-floor paths | 1.091 ms total (~1.09 microseconds/query) |
| 1 advisory agent update | mean 0.0030 ms; p95 0.0032; max 0.0150 |
| 10 agents | mean 0.0427 ms; p95 0.0707; max 0.1239 |
| 100 agents | mean 1.5661 ms; p95 1.6992; max 2.5624 |
| Actual game fixed/scripts/presentation | 1.289 ms/step; 46 bodies after runtime spawn |
| Game rendered offscreen 640×360 | mean 6.284 ms (~159 FPS); p95 12.490; max 24.710 |
| Draws | 75/frame including shadow/UI submissions |
| First render (reported separately) | 315.981 ms initialization/shader stall |

Agent benchmark measures the actual service on stationary observed agents, including
cadenced repaths/avoidance; not 100 complete game physics/render instances. Frame
samples exclude five warm-up frames and explicitly preserve first-render cost.
Headless/controller/audio-device warnings are environment limits, not listening or
human interaction proof. No universal realtime or desktop-resolution FPS guarantee.

## Boundaries / limitations

Locally 2.5D Recast, not continuous whole-planet navigation. Separate oriented surfaces
and explicit links work; no streaming/live rebasing, flying/swimming-volume navigation
or runtime full-world baking. Collider sphere/terrain and projected modifiers are
finite conservative bake approximations. Static surfaces and current Linux native
layer payload format. Authors must choose profile clearance for their mover.
Navigation raycast is horizontal traversal; sample returned endpoints for surface height.
Links follow explicit/authored transforms, not automatically simulated body motion.
Public runtime toggles cover agent/obstacle/link. Bounded query buffers return partial
routes; incremental carving/repath has latency; avoidance is advisory, not perfect
crowd separation or motor-to-motor physical blocking. No post-M53 AI work.

## Human checklist / controls

WASD/left stick move, mouse/right stick look, left mouse/fire shoot, Space launch,
V first/third person, Escape pause, R reload, B barrier, N spawn navigator.

1. Move through both camera modes; shoot original spring/hinge targets.
2. Watch six navigators route around walls/corners; move elsewhere and observe repaths.
3. Put walls between you and agents; observe multiple-agent steering/avoidance.
4. Shoot navigators twice; confirm independent defeat and score.
5. Toggle B; check navigation updates and the separate physical barrier.
6. Spawn N; confirm ordinary prefab independence.
7. Observe the landing navigator's scripted hover link (look toward the east opening).
8. Pause/resume and R/reload; confirm clean handles/round/navigation reset.
9. Inspect/edit enemy.js chase cadence or authored guidance speed without C++ rebuild.
10. In editor select navigation surfaces, preview/bake, inspect areas/routes/carving.
11. Test the moved exported standalone equivalently.

Project: `projects/shooter_game/shooter_game.judasproj`.
Authoring: `docs/NAVIGATION.md`; API: `docs/judasjs/navigation.md`.
Final playable package: `/tmp/Judas_Spring_Range_M53_Final`.
Exact file list: `CHANGED_FILES.md`; candidate hashes: `SOURCE_SHA256.json`.
Protected historical evidence/prototypes and accepted fluid work remain unchanged.
ROADMAP.md remains absent.

Proposed commit after approval: `Add M53 navigation and navmesh system`.
