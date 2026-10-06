# Skate-game consumer audit — investigation journal

Chronological. Entries are not rewritten after later discoveries; corrections are
added as new entries.

## 2026-10-05 — J01 Freeze and isolate

- Authoritative repository: `/home/conner/Documents/GitHub/Project-Judas`
- HEAD = main = origin/main = `a9c6cd780b8c93db6355b791c6fdb34afec7cf53`
  ("Add M60 streaming audio and environmental acoustics").
- `git ls-remote origin main` also reported `a9c6cd78…` (remote not fetched into the
  authoritative repo, so its refs were not touched).
- `git status`: clean, "up-to-date with origin/main". 34 local tags (none created by us).
- Clone: `git clone --no-hardlinks` into
  `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/Judas`, branch `skate-audit`
  at `a9c6cd7`. Remote renamed `source-local`, push URL set to `DISABLED_NO_PUSH`.

## J02 Toolchain/environment facts

- 8 cores, 15 GiB RAM, Wayland session (XWayland `:0`). No xdotool/Xvfb/screenshot
  utility, no Node.js, no game controller attached (`/proc/bus/input/devices` has
  no gamepad). Blender 5.2.1 available. ffmpeg available.
- Consequence: interactive editor use cannot be automated by mouse here. Editor
  evaluation is from source/docs + launching it; gameplay evaluation uses in-game
  scripted "autopilot" input (game JS) + console telemetry + the engine's
  `JUDAS_TERRAIN_SCREENSHOT` (writes one PNG after 600 fixed steps, any scene)
  + `JUDAS_PROFILE`/`JUDAS_PROFILE_OUTPUT`.

## J03 Build

- `cmake --build build --target judas` (Release) — an accidental concurrent "all"
  build and an over-broad `pkill` killed the ICU ExternalProject mid-build; ICU
  sub-build had to be deleted and rebuilt. My error, not an engine issue.
- Observation: building the four developer executables still compiles a large
  number of `*_tests` objects (to be confirmed which dependency pulls them in).

## J04 Documentation read-through (before writing any game code)

Read: JUDASJS.md, SCRIPTING.md, judas.d.ts, judasjs/{cookbook,practices,lifecycle,
physics,character,animation-ragdolls,entities,input,effects-camera,scenes-state,ui},
PHYSICS_QUERIES.md, JOINTS.md, COLLISION_EVENTS.md, MATERIALS.md, M49, ANIMATION.md,
POSE_COMPOSITION.md, RAGDOLLS.md, M52, M38, PROFILER.md.

Early facts that shape the design:
- Authored rigid collider shapes: box, sphere, compound-of-axis-aligned-boxes,
  radial terrain. No triangle-mesh, convex-hull, heightfield-on-plane, cylinder
  or authored capsule bodies. Compound children cannot be rotated.
  => every curved transition must be authored as many separately rotated boxes.
- Skinned import: one mesh node, one skin, ≤48 skin joints, ≤128 nodes, glTF only
  (no FBX). No root-motion handling. No IK, no raw bone access from JS.
- Per-body friction/restitution exist (`body.friction`), no named physics material
  asset, no anisotropic friction, no rolling friction.
- CharacterMotor: query-only capsule, up derived from gravity (not surface).
- Script properties: number/boolean/string only; no entity-reference type.
- JS cannot set collision layers/masks of ordinary bodies at runtime.

## J05 Asset inspection (Blender)

- Skateboarder.fbx: Mixamo rig, 65 bones, 6 mesh objects (~50k tris). Body mesh
  alone references 49 vertex groups. => cannot import as-is (48-joint, single
  mesh node limits). Needs content surgery: join meshes, fold 40 finger bones
  into hands, drop *_End bones => 22 bones.
- Push/Cruise/Walking clips: root motion baked into Hips (Push travels ~4.7 m
  over 70 frames). Judas has no root-motion extraction => bake in place.
- Skateboard.fbx: 5 static meshes, real-world scale (0.76 m deck).
- Ramp packs: small quarter-pipe (needs scaling), wooden kicker over barrels,
  half-pipe with stairs (cm units, ×0.01), modular room, props. None ship
  collision proxies (as expected) — Judas cannot derive any from meshes.

## J06 Content pipeline (game-side, Blender)

- `Tools/bake_rider.py`: joined 6 meshes → 1; folded 40 finger + 3 end bones into
  parents (65 → 22 joints); removed linear Hips travel (root motion) from Push/
  Cruise/Walk; rescaled cm keys and applied the 0.01 armature scale; authored 8
  static pose clips (Ride, Crouch, Air, Grind, Manual, Grab, LeanLeft/Right);
  exported one GLB with NLA tracks → named glTF animations.
- Result: imports first time. `animation.info` lists 11 clips and 24 joint keys
  (full hierarchy paths like `Rider/mixamorig:Hips/...`). Colon-bearing Mixamo names work.
- KHR_materials_specular reported as "optional extension ignored" (fine).
- `.judasmeta` sidecars written by the generator are accepted (IDs are md5 of path).

## J07 Architecture lab #1 (`Scenes/lab.judas`, 6 profiles × 3 chassis)

Profiles: 9-facet QP r=3, 24-facet QP r=3, kicker+landing, funbox crest, 15° speed
slope into wall, 45° bank. Chassis: R = dynamic box + 4 raycast springs, R2 = bare
frictionless dynamic box, M = CharacterMotor capsule + script velocity.

First run: R ≡ R2 — **my bug**: I re-assigned body velocity computed from a value read
before the spring impulses, cancelling them. Fixed.

Second run (`/tmp` logs preserved in audit/evidence/lab):
- R2 (bare box): hits the first QP facet and stops (6.86 → 1.21 m/s in 0.1 s). Box
  corner/edge vs facet seam is an inelastic impact. **Rejected.**
- R (springs): climbs QP (E 24.4 → 19.8 J/kg at apex), but on descent bottoms out
  under centripetal load and the box end catches a facet (4.94 → 0.37 m/s).
- M (motor): climbs QP losing ~25% energy (sweep-and-slide clip at every facet seam,
  `collided=1`), returns smoothly. `supported` is **false for the entire uphill
  portion** because the motor treats any velocity component along gravity-up as
  "departing" (CharacterMotor.cpp `departing` rule). The capsule stays gravity-upright,
  so board pitch on transitions must be purely visual. **Rejected as primary chassis**
  (kept as evidence; motor is designed for walking characters).

## J08 Spring suspension energy gain → isolation

On the real park, the spring chassis gained energy on the QP (24.5 → 38.8 J/kg on the
return). Isolation lab (`Scenes/springlab.judas`, `lab_spring.js`, logs in evidence):
- body-axis springs, point impulses, undamped vertical drop: stable 3.3 s, then a
  rocking instability pumps E 6.65 → 59 J/kg.
- **world-axis** springs (central or point impulses): bounded, slightly dissipative
  (E0 6.65, max 8.46, final 4.88 / 2.80).
=> Engine impulse integration is not the energy source; the body-axis "follower"
spring force combined with rotation is (a known non-conservative formulation).
Classified A (game implementation). Spring suspension **rejected**.

## J09 Retained chassis: velocity-level ground constraint (rigid body)

Dynamic box (rider+board mass 75 kg, friction 0) + 4 wheel raycasts per step.
While grounded: plane-fit normal from wheel hit points (smooths facet seams);
transport last step's tangential velocity to the new tangent plane (speed preserved),
add gravity along slope, rolling resistance/drag/push/brake in JS, normal velocity
is a P term holding ride height (pull-down limited to 0.6 m/s so crests can launch).
Solver velocity is used when it deviates (>0.8 m/s) from the prediction (collision).
Result on park run: monotonic energy loss only; QP 6.58 m/s → apex 2.28 m (ideal
2.2 m); funbox crest launches at 7.9 m/s and rolls over at 5.5 m/s.

## J10 Park + game loop first runs

- First park run faulted the skater script: `TypeError: invalid audio settings or unknown
  group` — audio groups must be registered in project settings (`audio-groups`). The
  emitter log line said "Invalid clip/settings/group or 128 voice limit" (one message
  for four different causes).
- Box render primitives map 0..1 UVs per face regardless of size (Renderer.cpp:405) →
  textures stretched across long ramp facets. Workaround: generator batches all static
  park visuals into OBJ meshes with metric UVs (also cut ~150 render entities to 6).
- Startup logs "Localization: missing localization key …" during the documented async
  catalog window; keys are not missing (they resolve a few frames later).

## J11 Scenario suite (position-triggered autopilot; evidence in audit/evidence/scenarios)

Game bugs found and fixed along the way (all class A): pop counted two launches (gap
detection failed); vert takeoff used world-up because wheel rays miss at the lip
(vert air not detected → attitude assist fought the spin); grind snap ignored rail
ends; board collided with rail end faces (board mask now excludes Rail; grinding is
snapped); scenario spawned inside the funbox (scenario error).
Results: kicker gap + kickflip (gap awarded, 1383), handrail 50-50 (0.8 s), vert air
Indy + BS 180 with vert-assist (560), manual → ollie → flat-bar 50-50 (1960 combo,
goal), unfinished flip → bail → ragdoll, 10 m/s wall slam → ragdoll.
Ragdoll entry inherits velocity from the rider's presentation-rate pose writes
(hips 5.15 m/s vs board 8.8 m/s on a vert landing; 11.2 m/s pre-impact on the wall
slam) — useful, but implicit; the game now sets mapped body velocities explicitly.

## J12 Feet/board study

- My first static poses were wrong: edits were applied in armature space (the FBX
  armature object is rotated 90°), and a crouch that only lowers the hips drags the
  feet through the deck (content bug).
- Fix: offline IK in Blender (IK constraints, then visual transform baked) keeps feet
  planted (toe_y = +0.096 in Ride/Crouch/Grind/Manual/Lean, hips 0.82 → 0.60–0.72 m);
  Air/Grab feet raised by 0.18/0.22 m; Grab hand IK to the deck edge.
  (`audit/evidence/feet_measurements.txt`).
- Crossfade check with the engine's documented mixer math (lerp T, normalized quaternion
  mix): Ride→Air feet deviate ≤7 mm from a linear board-lift estimate; Ride→Crouch
  ≤4 mm (`audit/evidence/blend_feet_*.txt`). The script syncs board lift to
  `animation.info.transitionFraction` — the only runtime signal available, because
  JudasJS cannot read resolved joint transforms.
- Rider is rigidly attached to the board pose (feet stay on the deck through pitch/roll/
  yaw/transitions by construction). Consequences: on a 26° roll-in the whole body leans
  26°; landing compression cannot bend knees except by switching clips; ollie nose pop
  and manual pitch move the feet with the board (rider rotated about the rear axle).

## J13 Editor autotest on the generated project

`JUDAS_EDITOR_AUTOTEST` run (evidence in audit/evidence/editor): project opens; duplicate →
undo restores IDENTICAL; Play/Stop leaves authored scene IDENTICAL; scene re-save OK
(field order normalized, no data loss); Export project packaged 47 assets.
One autotest line "live cursor + frozen capture … FAIL" — precondition is profiler
collection enabled (JUDAS_PROFILE unset in that run); not an engine/game defect.
Editor survey (labels/source): single selection only (`EditorDocument::m_selected`),
no hierarchy search/filter, no component copy/paste, no curve/ramp/spline tooling,
no collider-from-mesh or fit-to-mesh, ragdoll bones typed by joint key (≤32 bones,
Euler frames). The park (164 objects + 5 region scenes) was authored by generator.

## J14 M59 streaming: downhill street (5 × 80 m regions, 6° slope)

- Normal load radius 120 m: regions prepare in 7–11 ms (workers) and integrate in
  4–6 ms (largest unit 3.3–4.9 ms) at the outer boundary; activation ~100 m ahead.
- Tight radius 12 m at 14.9 m/s: activation 0.6 s ahead of arrival, never fell through.
  Integration units grew to 7.9–10.3 ms, total integration up to 17.7 ms (respawn,
  two regions at once) — a visible frame spike; budgets bound dispatch, not unit length
  (as documented).
- Airborne boundary crossing (street kicker lip in region 2, landing in region 3,
  13.4 m/s): clean.
- Streamed rails: the grind cache had to be rebuilt by game JS on region state change
  (worked: 2.3 s 50-50 on "Street Rail").
- Dynamic cones in snapshot regions: knocked cones caused bails (gameplay), regions
  unloaded/reloaded behind the skater.
- Scenario errors (mine): spawned under the road surface → fell through the world.
- Spawning inside a streamed area needs a game-side "hold until active" policy (the
  skater is frozen while its region activates).

## J15 Profiling suite (M56; evidence/profile_table.md, per-scenario profile.json)

Machine: 8 cores, load average ~2.1 from other desktop processes during captures,
100 Hz display (vsync-bound 10 ms frames). Ordinary skating: CPU active 2.6 ms median,
fixed step 0.86 ms, rigid physics 0.08 ms, JS fixed 0.36 ms. Bail: rigid physics p95
3.6 ms at ragdoll entry. 20 animated spectators: animation 0.17 ms/frame, 1.1 M tris.
Resting ragdolls (13 bodies/12 joints each): 5 → 5.6 ms/step, 10 → 12.2 ms/step,
20 → 42 ms/step (8-step catch-up cap reached every frame; frame p95 423 ms),
40 → 63.7 ms/step (frame p95 675 ms). No body sleeping exists in PhysicsWorld;
the distance fidelity policy is C++-installable only and doesn't cover ragdoll bodies.

## J16 Standalone export

`judas_export` CLI: 64 assets, 9 scenes, 63 MB, 1.0 s. Packaged `judas` runs from /tmp.
All `.judas` in Scenes/ are packaged (lab/test scenes too; no exclusion list).
Runtime prints "World state: disk saves disabled for additive compositions": adopting
the street manifest disabled M29 saves for the whole game (documented policy).

## J17 Constraint-based grinding experiment (authored slider joints) → engine bug

Lab (`Scenes/jointlab.judas`): rail-aligned slider joint (board ↔ world), authored
disabled, enabled by JS at grind start. Wrong-sign limit first (my error): board
engaged 6.7 cm past the limit and was thrown back at 4.3 m/s (from 6 m/s forward).
Correct limits: speed conserved, 30° yaw error corrected in ~0.2 s, but vertical and
lateral oscillation grows; boards released at 7.8–10.4 m/s sideways; aligned board
sank to the ground while the joint reported active.
Minimal repro (`Scenes/sliderrepro.judas`, evidence/slider_repro): one dynamic box on an
enabled slider, zero initial error, gravity only, no scripts acting:
separation 0 m holds; 1 m ±9 mm; 2 m+ diverges (|v| up to 52,476 m/s); mass-independent;
identical with world anchor or static-body anchor; identical with identity frames.
Source reading: slider perpendicular rows add an angular term `n × delta`
(JointSolver.cpp:23) scaled by anchor separation; with a low-inertia body the coupled
system diverges. Classified D/HIGH. Constraint-based grinding rejected; velocity-level
snapping retained.
