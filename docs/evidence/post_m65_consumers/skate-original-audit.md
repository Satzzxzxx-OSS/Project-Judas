# SKATE_GAME_ENGINE_AUDIT — Project Judas as a skateboarding-game engine

Parallel real-game consumer audit of a **frozen** Project Judas checkpoint.
The game was built as an ordinary Judas project with public JudasJS only; no engine
source, binding, editor or engine documentation was modified. Classifications follow
the brief: A game · B docs · C editor/workflow · D engine bug · E missing generic primitive ·
F performance · G intentional/acceptable.

Human feel/visual/listening acceptance is **not** claimed anywhere in this report.

---

## 1. Executive summary

**A substantial Skate/THPS-style prototype was built on Judas as it exists, and it works.**
It has:
- **Park and street:** a 164-object park and a 400 m streamed downhill street (5 M59 regions).
- **Skating:** pushing, carving, braking, slope physics, ramps, quarter-pipe and vert air.
- **Tricks:** ollies, five flip tricks, four grabs, spins, manuals.
- **Grinds:** rails, ledges, coping and streamed street rails.
- **Bails:** ragdoll bails with respawn.
- **Game loop:** score/combo system, six goals, S-K-A-T-E letters, a gap, run timer, HUD, a pause menu with English/Spanish, results screen, music and SFX.
- **Export:** a relocatable standalone package.

**What Judas got right for this game:**
- The **public query + rigid-body velocity/impulse API** was enough to build a stable skateboard chassis with no wheel or vehicle primitive.
- **Presentation-phase interpolation** (`presentationUpdate` / `presentedTransform`) made board, rider and camera follow cleanly.
- **Skinned import, crossfades and additive layers** worked first time with prepared content.
- **Sensors/contact events, runtime UI, localization, audio, the M56 profiler, M38 export and M59 streaming** all composed without fighting each other.
- M59 survived **15 m/s and airborne region-boundary crossings**, even with a 12 m load radius.

**What hurt**, in order of evidence strength:
1. **D-01 — Slider joints are numerically unstable** once the attached body is more than ~1–2 m from the anchor along the slider axis. A minimal 1-body repro diverges to >50,000 m/s from zero initial error. This killed constraint-based grinding and would affect any long-travel slider.
2. **E-03 — Static collision geometry is boxes/spheres/axis-aligned compounds only.** Every curved transition is a chain of rotated boxes. Facet seams create normal discontinuities: a bare rigid board stops dead at the first quarter-pipe seam. The workaround (raycast plane-fit + velocity-level constraint) works, but authoring and feel pay for it everywhere.
3. **E-01 / E-02 — No runtime access to resolved joint transforms, and no runtime IK/target constraints.**
   - Planted feet were only achievable by solving IK *offline* in Blender and baking poses.
   - At runtime the board/feet relationship can only be synchronised through the crossfade fraction.
   - Body-vs-board adaptation (slopes, landings, ollie pitch, manuals, grabs) is not expressible.
4. **F-01 — Resting ragdolls never sleep.** Each costs ~1.1–2 ms per fixed step. 20 resting ragdolls push the fixed step to 42 ms (catch-up cap every frame, frame p95 423 ms).
5. **C-01 / C-02 — The editor has no bulk/procedural authoring.** It has single selection, no search, no curve/ramp or collider-from-mesh tools, and per-bone typed ragdoll mapping. The park was produced by an external generator; hand-authoring ~200 rotated collider boxes is not realistic.

Most remaining issues were game implementation (class A) and were fixed in the game. Several of my own reasonable first approaches failed for reasons that were mine, not the engine's; they are preserved in §8 and the journal.

---

## 2. Frozen Judas checkpoint

| Item | Value |
|---|---|
| Authoritative repository | `/home/conner/Documents/GitHub/Project-Judas` |
| HEAD = main = origin/main (at start) | `a9c6cd780b8c93db6355b791c6fdb34afec7cf53` — "Add M60 streaming audio and environmental acoustics" |
| `git ls-remote origin main` | `a9c6cd780b8c93db6355b791c6fdb34afec7cf53` |
| Working tree at start | clean ("up-to-date with origin/main"), 34 tags |
| Frozen audit commit | `a9c6cd7` (never updated during the audit) |

## 3. Clone / isolation proof

| Item | Value |
|---|---|
| Isolated clone | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/Judas` (`git clone --no-hardlinks`) |
| Local branch | `skate-audit` from `a9c6cd7` |
| Remote | renamed `source-local` (fetch = authoritative path), **push URL = `DISABLED_NO_PUSH`** |
| Build | `build/` in the clone, Release, targets `judas judas_editor judas_export judas_scene_author` |
| Engine source changes in clone | **none** (`git diff a9c6cd7 -- src docs tools scripts CMakeLists.txt third_party cmake assets` is empty; every committed path is under `projects/skate_game/`) |

Local experimental commits (clone only, never pushed):

| Hash | Purpose |
|---|---|
| `adc42a1` | Playable park prototype (chassis, tricks, grinds, HUD, audio) |
| `24b7b66` | Velocity-level chassis, scenarios, IK-baked poses, board/feet sync |
| `bd0485a` | Streamed downhill street (M59), editor autotest evidence |
| `783191d` | Profiling, joint-grind lab + slider repro, UI evidence, README |
| `59a579c` | Copy of this report inside the project |

Authoritative repository state at handoff: see "Repository safety" in §20.

---

## 4. Game / project architecture

```
projects/skate_game/
  skate_game.judasproj   input map (26 actions), tags/layers, audio groups, en/es locales, world manifest
  Scenes/park.judas      startup scene (root): park colliders, visual meshes, skater, rider, HUD, audio
  Scenes/street-0..4     M59 regions (downhill street)
  Assets/scripts/        game JS (below)        Assets/models/  rider.glb, board.glb, generated OBJ visuals
  Assets/ui, localization, audio (generated WAVs), textures, materials, world/street.judasworld
  Tools/                 content generators + Blender bakes + measurement/profiling tooling
```

**Entities (runtime roles):**
- **Skater** (id 10). The physics: one dynamic box `0.2 × 0.08 × 0.48 m`, 75 kg (rider+board), friction 0, layer `Board`, and `skater.js`.
- **Deck** (11). Body-free board mesh.
- **Rider** (12). Body-free skinned mesh with an Animation component and a 13-bone Ragdoll mapping.
- **SFX** (20–25). One AudioEmitter per entity, moved to the skater every frame.
- **Grind sparks** (30) and **Music** (31).
- **Game** (40). HUD document, audio listener, `hud.js`.
- **Rails/ledges/coping.** Static boxes tagged `grindable`, with `rail.js` publishing grind-line data through `this.state`.
- **Letters and gap zones.** Sensors with scripts.

**Scripts (all game behaviour):**

| Script | Responsibility |
|---|---|
| `skater.js` | Chassis: 4 wheel raycasts/step, plane-fit normal, velocity-level ground constraint, push/brake/carve, ollie, flips/grabs/spins/manual, vert assist, grind snapping and balance, landing rules, slam detection, bail/respawn, streaming interest, telemetry |
| `presentation.js` | Board visual (wheel contact drop, flip rotation, lift synced to crossfade), rider placement, clip state, additive lean layer, ragdoll enter/leave, sparks |
| `camera.js` | Velocity-led chase cam, orbit, vert-air hold, sphere-cast obstruction, bail framing, speed FOV |
| `sfx.js` | Rolling loop (volume/pitch from speed), grind loop, one-shots |
| `game.js` | Trick values, combo/multiplier, goals, letters, gaps, run timer (pure rules) |
| `hud.js` | HUD/meters, pause menu, results, music, locale switching (session-persisted) |
| `controls.js` | Logical input facade + "autopilot" programs (timed or position-triggered) used for attract mode and the audit's reproducible scenarios |

**Retained chassis** (journal J09). The skater is a rigid body. Each fixed step:
- Four raycasts find the ground; a plane fit through the hit points gives the surface normal and smooths facet seams.
- While continuously grounded, last step's tangential velocity is *transported* onto the new tangent plane (speed preserved), plus gravity along the slope.
- Rolling resistance, drag, push and brake are applied in JS.
- Normal velocity is a P term holding ride height. Pull-down is limited, so crests launch naturally.
- The solver's velocity is used whenever a collision changed it (walls, kerbs, cones).
- Orientation is aligned to the fitted normal via angular velocity; steering is yaw about the normal.

---

## 5. What was successfully built

- **Park:**
  - 26° roll-in from a 3 m deck;
  - funbox (28° banks, crests) with a ledge;
  - kicker + 5 m gap + landing (gap scoring);
  - five-step stair set with sloped handrail and platform ledge;
  - flat bar; street ledge;
  - 3.2 m quarter-pipe with vertical extension and grindable coping;
  - mini half-pipe (r = 2.2 m);
  - 40° bank + wall; perimeter walls; crates.
- **Streamed street:** 400 m, 6° slope, 5 M59 regions. It has kerb grind lines, a rail, a kicker whose landing lies in the next region, and knockable dynamic cones. Terminal speed is ~15–16 m/s.
- **Skating:** push cycles, carve steering, brake, momentum retention, slope acceleration, airborne attitude assist, vert assist, fakie/switch.
- **Tricks:**
  - ollie/nollie (crouch-to-pop);
  - kickflip, heelflip, pop shove-it, hardflip, varial kickflip, impossible;
  - Indy/Melon/Nosegrab/Tailgrab;
  - FS/BS spins;
  - manual / nose manual with a balance meter.
- **Grinds:** 50-50, 5-0 and boardslide on rails, ledges, coping and kerbs, with a balance meter, sparks, a grind loop and pop-off.
- **Bails:** over-rotation, unfinished flip, sideways landing, wall slam (impact-velocity threshold), lost balance, forced bail. Each triggers ragdoll entry with momentum, then respawn at the last safe spot or the player's checkpoint.
- **Game loop:** score/combo/multiplier with repetition decay, six goals, letters, gaps, 3-minute run, results screen, session best, restart, pause (modal), English/Spanish, music + SFX.
- **Export:** standalone package (`export/JudasSkate/judas`), runs from an unrelated directory.

Scenario evidence: `audit/evidence/scenarios/<name>/{shot.png, run.log, profile.json}`. Each scenario is reproducible with `Tools/scenario.sh`.

| Scenario | Result |
|---|---|
| `kicker_gap` | Ollie + kickflip over the 5 m gap; letter K; "Kicker Gap" awarded; 1,383 points |
| `handrail` | Ollie into 50-50 down the stair handrail (0.8 s), exit, land; 380 |
| `qp_vert` | 10.8 m/s into QP, vert air with Indy + BS 180, land; 560 |
| `manual_flatbar` | Manual → ollie → flat-bar 50-50 (1.43 s) → land; 1,960 combo (goal) |
| `flip_bail`, `ragdoll_settled` | Late kickflip → "flip unfinished" bail → ragdoll → settled pose |
| `bail_wall` | 10 m/s into perimeter wall → "slammed" → ragdoll into wall |
| `mini_ramp` | 7 m/s back-and-forth in the mini half-pipe |
| `downhill`, `downhill_tight` | 400 m streamed street, 15 m/s, airborne region crossing, 12 m load radius |
| `street_rail` | 50-50 on a rail that exists only after its region streams in (2.3 s) |
| `ui_pause_es`, `ui_results` | Spanish pause menu, results panel |

## 6. Controls / gameplay

See `projects/skate_game/README.md`. Summary:

| Action | Keyboard | Gamepad |
|---|---|---|
| Push / brake | W / S | RT / LT |
| Steer / air turn | A / D | Left stick |
| Ollie (hold to crouch) | Space | South |
| Flip | J (+direction) | West |
| Grab / manual | K | East |
| Grind | L | North |
| Spin | Q / E | LB / RB |
| Camera | Mouse; C toggles distance | Right stick |
| Checkpoint / restart / pause | T / R / Esc | — |

## 7. Judas systems consumed

| System | Public surface used | Verdict |
|---|---|---|
| Projects/assets/scenes (M30/M36) | `.judasproj`, `.judasmeta`, scene format, prefab-free | Worked; generator-written files accepted by runtime and editor |
| Rigid physics | dynamic box, static boxes, `velocity`, `angularVelocity`, `applyImpulse(AtPoint)`, `mass`, `inertiaWorld` | Worked; integration conservative (J08) |
| Queries (M44) | `physics.raycast`, `sphereCast`, filters (`excludeLayers`, `ignored`) | Worked, fast (~370 calls/s cost ~0.01 ms) |
| Contacts/sensors (M42) | `onCollisionEnter`, `onTriggerEnter`, `relativeVelocity` | Worked |
| Joints (M45) | authored slider joints, `physics.joint`, `setEnabled`, limits | **Unstable (D-01)** |
| CharacterMotor (M49) | lab only | Usable, rejected for skating (J07) |
| Animation/pose (M46/M47) | `crossFade`, `layer(additive, referenceClip)`, `info.transitionFraction` | Worked |
| Ragdolls (M48) | authored 13-bone mapping, `enter/leave`, `body(key).velocity` | Worked; resting cost high (F-01) |
| Presentation (M52) | `presentationUpdate`, `presentedTransform`, `world.setView` | Worked well |
| Runtime UI (M41) / M58 localization | `.judasui`, slider, modal pause, `onUI`, `textKey`, `localization.format/setLocale` | Worked |
| Audio (M34/M60) | emitters, loops, one-shots, `setAudio` volume/pitch, groups, `setAudioVelocity` | Worked |
| Particles (M37) | sparks via `setParticles({rate})` | Worked |
| Streaming (M59) | manifest, `scenes.setInterest`, `scenes.regions` | Worked under stress |
| Profiler (M56) | `JUDAS_PROFILE*`, `profiler.scope` | Worked; decisive for F-01 |
| Export (M38) | `judas_export` and editor export | Worked |
| Session store | locale + best score | Worked |
| Not used | navigation (no AI), liquids (out of scope), cloth/deformation/destruction (**absent in checkpoint — not recreated**) | — |

---

## 8. Approaches attempted and rejected

| Requirement | Approach | Result | Decision |
|---|---|---|---|
| Chassis | **R2** bare frictionless dynamic box on facets | Stops dead at first QP facet seam (6.86 → 1.21 m/s in 0.1 s) | Rejected |
| Chassis | **M** CharacterMotor capsule + JS velocity | Works; ~25% energy lost climbing the QP (slide clip at each seam); `supported=false` whenever moving uphill; capsule always gravity-upright | Rejected for skating (evidence B-01, G-06) |
| Chassis | **R** dynamic box + 4 raycast spring/damper impulses | First run identical to R2 — **my bug** (velocity overwrite). Fixed run climbs QP but bottoms out and catches facets; on the park it **gained** energy (24.5 → 38.8 J/kg) | Rejected after isolation (J08): body-axis follower forces pump energy; world-axis springs are conservative ⇒ class A |
| Chassis | **Velocity-level ground constraint** (§4) | Monotonic energy loss only; QP apex 2.28 m vs ideal 2.2 m | **Retained** |
| Grinding | **Authored slider joints** enabled by JS | Wrong-sign limits (my error) → board thrown back at 4.3 m/s. Correct limits → divergent oscillation, boards released at 7.8–10.4 m/s sideways | Rejected; isolated to **D-01** |
| Grinding | **Velocity-level snap** to authored grind line (`rail.js` data), balance meter | Works on rails, ledges, coping and streamed rails | **Retained** |
| Rider poses | Static clips authored by moving bones in Blender | My edits were in armature space (FBX armature rotated 90°): "crouch" moved hips sideways; crouch without leg solve sinks feet into deck | Rejected (content bug) |
| Rider poses | **Offline IK** (Blender IK constraints, baked) | Feet planted (toe y = 0.096 m in all riding poses, hips 0.82 → 0.60–0.72 m), Air/Grab feet lifted, Grab hand to deck | **Retained** |
| Landing compression | Translate rider down on landing | Feet sink through deck | Rejected → switch to Crouch clip |
| Visuals | Box render primitives with textures | 0..1 UV per face regardless of size → smeared | Replaced by generated OBJ meshes with metric UVs |
| Board/rail collisions | Board collides with rails | Board snags on rail end faces/posts | Board mask excludes `Rail`; grinds are snapped |

## 9. Positive findings

| ID | Finding |
|---|---|
| P-01 | Raycast + rigid-body velocity/impulse API is sufficient for a responsive skateboard chassis; no wheel/vehicle/suspension component is needed. 4 rays/step; skater fixed-step JS ≈ 0.2–0.36 ms. |
| P-02 | `presentationUpdate` + `presentedTransform` gave jitter-free board/rider/camera following (rider = body-free entity rewritten each presentation frame). |
| P-03 | glTF skinned import accepted the reduced Mixamo rig first time. Crossfades of IK-baked poses keep feet within 4–7 mm of a linear estimate. `info.transitionFraction` is enough to sync a prop to a crossfade. Additive lean layers worked. |
| P-04 | Ragdoll entry from the current animated pose works, inherits observed motion, and mapped bodies are ordinary entities (velocity/impulse); the settled pose looks plausible. |
| P-05 | Sensors (letters, gaps, finish line) and collision events (`relativeVelocity` for slam detection) were straightforward. |
| P-06 | M59 streaming: regions prepared on workers (7–28 ms) and integrated in 4–6 ms normally. No fall-through at 15 m/s with a 12 m load radius, and a clean airborne boundary crossing. Game policy is a single `setInterest` call. |
| P-07 | M56 profiler: env-var capture from standalone, per-script-asset attribution, custom scopes. It made the ragdoll and streaming costs obvious without guessing. |
| P-08 | M38 export: 1 s, relocatable package, ran first time from `/tmp`. |
| P-09 | Runtime UI + M58 localization: HUD, slider-as-meter, modal pause, Spanish via `textKey`. A non-modal HUD never steals gameplay input unless an uncaptured pointer is over a control (`RuntimeUI.cpp:116`). |
| P-10 | Audio is easy to drive from game state (volume/pitch per frame, loops, one-shots, groups, Doppler velocity). |
| P-11 | Editor round-trips generator-written scenes losslessly; Play/Stop restores the authored scene IDENTICAL (autotest). |
| P-12 | Script fault isolation: a faulting slot logs asset/entity/phase/stack and the rest of the game continues (seen with the audio-group error). |
| P-13 | Documentation states hard limits honestly (48 skin joints, one mesh node, no root motion, no IK, no joint creation from JS); the JudasJS lifecycle/phase docs were correct for everything used. |
| P-14 | Isolation lab showed rigid-body impulse integration is conservative (world-axis springs bounded, slightly dissipative). |

---

## 10. Engine bugs (class D)

### D-01 — Slider joint diverges when the body's anchor is ≥ ~1–2 m along the axis from the other anchor — **HIGH**

- **Symptom:** A body held by an enabled slider joint oscillates with growing amplitude and explodes (positions in km, velocities up to 52,476 m/s). This happens with zero initial constraint error, gravity only, and no scripts applying forces.
- **Reproduction:**
  - Files: `audit/evidence/slider_repro/` (`build_sliderrepro.py`, `sliderrepro.judas`, `lab_sliderhold.js`, log).
  - Setup: one 75 kg dynamic box (0.48 × 0.08 × 0.2 m), slider axis horizontal, anchors at body centre and at a world or static-body point.
- **Results by separation:**

  | Separation | Result |
  |---|---|
  | 0 m | Holds exactly |
  | 1 m | ±9 mm wobble |
  | 2 m | Diverges (12,310 m/s) |
  | 3, 6, 12 m | Diverge |

  The result is identical for 5 kg vs 75 kg, world vs static-body anchor, and rotated vs identity frames.
- **Expected:** A slider permits free translation along its axis at any coordinate.
- **Systems attempted:** Constraint-based grinding (J17) showed it first: boards sank to the ground while the joint reported `active`, and were released sideways at 7.8–10.4 m/s.
- **Diagnosis (read only):** `JointSolver.cpp:23–25`. The perpendicular linear rows add an angular Jacobian term `n × delta` that grows with anchor separation. For a low-inertia body the sequential-impulse system becomes ill-conditioned and diverges.
- **Workaround:** Don't use sliders with long travel; grinding uses velocity-level snapping instead.
- **Generic:** Yes — doors, drawers, elevators, pistons, rails, cable cars.
- **Engine direction:** A slider formulation stable at any coordinate (e.g. perpendicular error measured at the projected/closest point on the axis rather than through the full separation lever arm), plus a long-travel regression test.

### D-02 — Joint limit engaged while violated produces a near-elastic rebound — **MEDIUM**

- **Symptom:** Enabling a slider whose limit was already exceeded by 6.7 cm reversed a 6 m/s approach into a 4.3 m/s rebound (`jointlab_violated.log`).
- **Expected:** Inelastic stop plus gentle positional correction.
- **Notes:** It may share the cause of D-01 (bias/conditioning).
- **Generic:** Yes (any limited joint enabled at runtime).
- **Direction:** Clamp the stabilisation bias for limit rows (split-impulse or max correction velocity).

### D-03 — "Localization: missing localization key …" logged for keys that exist — **LOW**

- **Where:** Logged during the documented asynchronous catalog-startup window. Seen in this game and in the shipped M52/M59 demos.
- **Effect:** The keys resolve frames later, so the log misreports a content error.
- **Direction:** Report "catalog not yet published" distinctly from "key absent".

---

## 11. Missing generic primitives (class E)

### E-01 — No read access to resolved skeletal joint transforms (sockets/attachments) from JS — **HIGH**

- **Needed for:**
  - keeping the board under the feet when the pose changes;
  - verifying foot/hand contact;
  - attaching the board to a hand during grabs;
  - emitting sparks/particles at a foot or hand;
  - camera targeting a body part, aiming, holding props.
- **Existing systems tried:** `animation.info` (only clip/fade data); ragdoll `body(key)` (only while articulated). `presentation.js` syncs board lift to `info.transitionFraction`. That is the *only* available signal, and it fails for interrupted fades, multiple layers or any runtime pose change.
- **Evidence:** `blend_feet_*.txt`. The fraction-based estimate is accurate (≤ 7 mm) only for a clean two-clip fade of offline-IK poses.
- **Generic:** Every character game.
- **Direction:** A read-only `animation.joint(key)` world/local transform of the resolved final pose (presentation-timed), and/or authored attachment sockets.

### E-02 — No runtime IK / skeletal target constraints — **MEDIUM**

- **Symptom:** Feet can only be kept on the deck by attaching the whole rider rigidly to the board pose.
- **Consequences:**
  - On the 26° roll-in the rider's whole body leans 26°.
  - Landing compression can't bend knees except by switching to a pre-baked Crouch clip.
  - An ollie's nose-pop or a manual's board pitch can't let the body stay upright while the feet follow the board; the game rotates the entire rider about the rear axle.
  - Uneven landings, slope adaptation and hand-to-board grabs on a flipping board are not expressible.
- **Evidence:** A first crouch authored without leg solving put the feet 0.16 m through the deck (J12). Planted feet were achieved only with **offline** Blender IK baked into static clips (`feet_measurements.txt`: toe y = 0.096 m in all riding poses).
- **Generic:** Stairs, slopes, ladders, vehicles, hand placement — any grounded character.
- **Direction:** A two-bone/limb IK target contribution in the M47 pose stack (priority-ordered like `ragdollPhysics`), with JS-settable world targets and weights.

### E-03 — Static collision geometry limited to boxes/spheres/axis-aligned box compounds — **HIGH**

- **Symptom:** Transitions, banks and kickers are chains of rotated box bodies. Compound children can't be rotated, so each facet is a separate body; the park has ~200 collider boxes. Seams produce normal discontinuities: 14 "normal jumps" over a few mini-ramp passes, and a bare rigid board stops dead at the first QP seam (§8).
- **Existing systems tried:** Box chains with plane-fit smoothing (works); finer faceting (24 vs 9 facets halves jitter but multiplies bodies).
- **Missing:** Triangle-mesh/convex static colliders, heightfields on non-radial ground, rotated compound children, and collider generation from render meshes. The supplied ramp/half-pipe art could not be used with matching collision.
- **Generic:** Racing, skate/BMX, terrain, architecture — any game with curved static geometry.
- **Direction:** Static triangle-mesh colliders (queries + contacts), convex hulls, rotated compound children, mesh→collider authoring.

### E-04 — No public collider shape/extent introspection or closest-point query — **MEDIUM**

- **Symptom:** The grind code needs the grind line of a rail, but JS can't read a body's half-extents. Each rail duplicates its collider size into `rail.js` properties (published via `scriptState`).
- **Also missing:** A closest-point/distance query, which would make snapping to arbitrary geometry (rails, ledges, coping, climbable edges) generic.
- **Generic:** Snapping, ledge grabs, climbing, AI cover.
- **Direction:** `physics.closestPoint(point, filter)` and/or read-only collider shape on `Entity`. A reusable polyline/path asset is a lower priority: curved coping would need many segments today.

### E-05 — No game-initiated persistent storage — **MEDIUM**

- **Symptom:** JS has no save/load or profile store. M29 world-state saves are triggered by F6 and restore the whole world. Adding an M59 manifest disables disk saves for the entire game ("disk saves disabled for additive compositions").
- **Impact:** Best scores, completed goals, unlocks and settings can't persist across launches; only the session store survives reloads.
- **Generic:** Every game with progression.
- **Direction:** A bounded JSON profile/settings store owned by the project, independent of world-state snapshots. (In-progress M61 save work was observed in the authoritative tree but not evaluated: out of the frozen scope.)

### E-06 — Runtime physical-material control and hit material identity — **LOW**

- **No runtime friction change:** Friction is authored per body and can't change at runtime. The loose board after a bail had to be damped in JS.
- **No material on hits:** Cast hits carry no material, so surface-dependent rolling sound/feel would need per-entity tags; there is no per-primitive identity.
- **Direction:** Physics material assets referenced by colliders, a material id on `CastHit`/`ContactEvent`, and a runtime material swap.

### E-07 — Joints can't be created or re-anchored from JS — **LOW** (compounded by D-01)

Constraint grinding would need a pre-authored joint per rail per direction, each referencing the skater. Generic for grabbing/carrying/attaching at runtime.

### E-08 — Size-aware UVs for primitive render shapes — **LOW**

Box primitives map 0..1 per face (`Renderer.cpp:405`). There is no world-space/triplanar or per-instance UV scale, so the generator wrote OBJ meshes instead.

### E-09 — Gravity sample for non-motor scripts — **LOW**

Only `character.gravity` exposes the field. The docs urge "no universal world-up", but a rigid-body script has no gravity query, so this game assumes −Y.

---

## 12. Editor / workflow findings (class C)

Editor evaluation is partial. This machine has no GUI automation (Wayland, no xdotool/Xvfb), so the editor was exercised through its built-in autotest (open, debug view, duplicate/undo, Play/Stop verification, save, export) and surveyed from its UI source. Generated content was used for the park; that itself is evidence for C-01.

| ID | Finding | Sev |
|---|---|---|
| C-01 | **No procedural/curved geometry or collider-from-mesh tooling.** There are no ramp/arc/profile tools, no array/duplicate-along-path, and no fit-collider-to-mesh. Each QP is 11 rotated boxes placed by numeric transforms. The park (164 objects + 5 regions, ~200 collider boxes, 6 batched visual meshes) was only feasible with an external generator. | HIGH |
| C-02 | **Single selection, no hierarchy search/filter/folders, no component copy/paste** (`EditorDocument::m_selected`; no Filter/Search/Paste in editor UI). Bulk edits (material of 40 facets, friction of all ramps) are one object at a time. | MEDIUM |
| C-03 | **Ragdoll mapping is typed bone-by-bone.** Joint key strings, Euler-degree frames, ≤32 bones; no skeleton picker, axis visualiser, auto-fit or humanoid template. Correct hinge axes (knee = local X, elbow = local Z) required computing joint frames from the GLB offline (`Tools/glb_joints.py`). | MEDIUM |
| C-04 | **Character import pipeline.** No FBX; ≤48 joints and one mesh node; no bone reduction, root-motion extraction, unit-scale correction or clip-merge. The Mixamo rider needed six Blender operations (join meshes, fold 43 bones, strip root motion, fix cm/0.01 scale, merge 3 FBX clips as NLA tracks, export GLB) before Judas accepted it. | MEDIUM |
| C-05 | **One AudioEmitter per entity.** Six helper entities follow the skater each frame. A missing audio-group registration produced "Invalid clip/settings/group or 128 voice limit" (four causes, one message) and faulted the skater script. | LOW |
| C-06 | `.judasui` is a positional record of ~50 unnamed numbers per element. Editor-only in practice; not reviewable or mergeable by hand. | LOW |
| C-07 | Export packages every scene in `Scenes/`, including test/lab scenes; there is no exclusion list. | LOW |
| C-08 | Evidence tooling: one engine screenshot per run, after 600 *fixed* steps. A modal UI pauses steps, so it can never be captured; there is no runtime screenshot key for testers. | LOW |
| C-09 | No inspector entity-reference property type (documented). Cross-entity references use tags (`deck_visual`, `rider`, `sfx_*`, …: 21 tags). | LOW |

## 13. Documentation findings (class B)

| ID | Finding | Sev |
|---|---|---|
| B-01 | CharacterMotor docs don't state that `supported` is false whenever velocity has an uphill component (the "departing" rule, `CharacterMotor.cpp`). Any rolling/sliding use must do its own ground probing. | MEDIUM |
| B-02 | Ragdoll docs explain hinge/slider frames abstractly but give no guidance on finding an imported rig's joint-local axes (Mixamo bones point along +Y; knee/elbow axes differ). No tool or example for real humanoids — only a 3-segment bar. | MEDIUM |
| B-03 | Joint docs give no travel/stability envelope for sliders (see D-01); "very large joint errors are not suitable" does not cover a zero-error long-travel case. | MEDIUM |
| B-04 | Box render UV behaviour (per-face normalised) is undocumented. | LOW |
| B-05 | The "audio groups must exist in project settings" requirement isn't connected to the emitter failure message in the audio reference. | LOW |
| B-06 | Ragdoll entry inherited ~60–100% of the motion implied by *presentation-rate teleports* of a body-free rider. Docs say teleports aren't continuous motion and entry uses "recent observed motion where available"; what counts as observed motion for body-free entities should be stated. | LOW |
| — | Positive: lifecycle/phase docs, safe-handle rules, presentation interpolation, streaming API and profiler docs matched behaviour. | — |

---

## 14. Performance evidence (M56)

- **Machine:** 8 cores, 15 GiB RAM, Linux/Wayland. 100 Hz display, so frames are vsync-bound at ~10 ms.
- **Concurrent load:** desktop session plus other processes, load average ~2.1 during captures.
- **Build:** Release, captured via `JUDAS_PROFILE=1`. The last ≤120 frames (first 10 skipped) are summarised by `Tools/profile_table.py`.
- **Units:** ms per frame unless noted.
- **Not a hardware claim:** these numbers are not a universal hardware statement.

| Scenario | frame med/p95 | CPU active med/p95 | fixed step med/p95 | rigid physics | JS fixed | JS present. | animation | ragdoll→pose | render submit | bodies | draws | tris |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ordinary skating | 9.98/10.59 | 2.63/3.32 | 0.856/1.004 | 0.084/0.117 | 0.362/0.460 | 0.288/0.418 | 0.016/0.023 | 0/0.001 | 0.467/0.637 | 156 | 29 | 106k |
| high speed (15 m/s street, region boundary) | 9.96/10.62 | 3.65/4.68 | 1.105/1.318 | 0.134/0.214 | 0.357/0.520 | 0.324/0.479 | 0.016/0.022 | 0 | 0.482/0.685 | 189 | 22 | 103k |
| tricks (kickflip gap) | 9.99/10.21 | 2.77/4.05 | 0.844/1.579 | 0.080/0.150 | 0.311/0.568 | 0.295/0.685 | 0.014/0.026 | 0 | 0.471/0.866 | 156 | 26 | 106k |
| grind (handrail) | 9.98/10.39 | 2.93/4.33 | 0.903/1.710 | 0.081/0.196 | 0.270/0.638 | 0.290/0.584 | 0.002/0.026 | 0 | 0.477/0.817 | 156 | 28 | 106k |
| bail (ragdoll entry) | 10.01/10.47 | 3.06/6.42 | 1.476/4.607 | 0.198/**3.560** | 0.319/0.498 | 0.296/0.437 | 0.017/0.031 | 0.016/0.033 | 0.470/0.641 | 169 | 25 | 106k |
| 20 animated spectators | 10.00/11.05 | 3.98/5.54 | 1.362/2.115 | 0.085/0.151 | 0.420/0.766 | 0.347/0.610 | 0.169/0.288 | 0.003/0.007 | 0.774/1.052 | 156 | 69 | 1.11M |
| 5 resting ragdolls | 9.99/12.01 | 6.50/11.86 | **5.556**/10.366 | 2.528/7.624 | 0.410 | 0.385 | 0.067 | 0.071 | 0.600 | 221 | 39 | 357k |
| 10 resting ragdolls | 14.79/19.47 | 14.63/19.33 | **12.207**/17.048 | 9.431/14.504 | 0.527 | 0.501 | 0.125 | 0.154 | 0.707 | 286 | 49 | 608k |
| 20 resting ragdolls | 15.21/**422.64** | 4.67/31.79 | **42.269**/62.545 | 9.798/146.995 | 0.623 | 0.434 | 0.226 | 0.154 | 0.830 | 293 | 69 | 1.11M |
| 40 spectators → ragdoll | 10.99/**675.36** | 4.73/16.80 | **63.652**/93.889 | 4.263/149.607 | 0.598 | 0.494 | 0.349 | 0.128 | 1.087 | 286 (end: 676 bodies, 480 joints, 735 contacts) | 109 | 2.11M |

GPU (delayed timestamps), ordinary skating: main camera 0.66 ms, shadow 0.10 ms, resolve 0.08 ms. With 20 spectators: main camera 2.26 ms.

**Dominant costs:**
- **Ordinary play:** CPU is dominated by script callbacks (~0.6 ms of JS across phases) and render submission (~0.5 ms); physics is negligible.
- **Many ragdolls:** the **contact + joint solver** dominates (`Physics contact and joint solver` p95 139 ms in the 40-ragdoll run). The fixed-step catch-up hits its 8-step cap every frame (`cap_reached=true`, backlog discarded).

**Streaming:** region preparation 7–28 ms on workers. Main-thread integration is 4–6 ms per region normally, up to 10.3 ms per unit and 17.7 ms total when two regions integrate together (tight-radius and respawn cases) — visible frame spikes.

### F-01 — Resting articulated bodies never sleep; ragdoll cost scales poorly — **MEDIUM** (HIGH for ragdoll-heavy games)

- **Cost:** ~1.1–2 ms per resting 13-body ragdoll per fixed step, about 10× the per-body cost of the engine's published 1,500-crate benchmark. Above ~10 resting ragdolls the game enters a spiral of death.
- **Why it doesn't sleep:** `PhysicsWorld` has no sleeping. The distance fidelity policy is C++-installable only and doesn't apply to ragdoll internals.
- **This game:** Fine — one ragdoll at a time, entry spike 3.6 ms p95.
- **Direction (generic):** Island sleeping/quiescence for bodies and articulations, and/or a project/JS-controllable fidelity policy.

### F-02 — Streaming integration units are long under pressure — **LOW–MEDIUM**

Budgets bound dispatch between units, not unit length (documented). A 4–10 ms indivisible unit is a visible hitch at 100 Hz.

---

## 15. Workarounds in use

| Problem | Workaround |
|---|---|
| E-03 faceted transitions | Wheel-raycast plane fit + velocity transport across seams; generator-built box chains |
| E-08 stretched textures | Batch static visuals into OBJ meshes with metric UVs (also 150 → 6 render entities) |
| E-01/E-02 feet & board | Offline IK-baked poses; rider rigidly attached to the deck; board lift driven by `transitionFraction` |
| D-01 slider | Velocity-level grind snapping instead of constraints |
| E-04 rail geometry | `rail.js` publishes duplicated grind-line data via `this.state` / `scriptState(1)` |
| C-09 entity references | 21 project tags used as references |
| C-05 emitter per entity | Six SFX helper entities moved each presentation frame |
| E-06 loose board friction | JS damping after bails |
| Streaming spawn | Skater frozen while its region activates; rail cache rebuilt on region state change |
| Vert landings | Game "vert assist" keeps vert air in the takeoff wall plane (THPS-style policy) |
| Modal UI screenshots | Scenario-only non-modal flag |

## 16. Blockers

None for this prototype: every major requirement has a working path. Under the brief's definition the nearest blocker-class items are:
- **D-01:** blocks *constraint-based* grinding and long sliders generally.
- **E-03:** blocks using authored curved art (the supplied half-pipe/kicker meshes) with matching collision.

## 17. Intentional / acceptable limitations (class G)

| ID | Limitation |
|---|---|
| G-01 | 48-joint / one-mesh-node skin import limit (documented; content reduction is workable, cost noted in C-04). |
| G-02 | Fixed 60 Hz simulation step, compile-time (`SimulationTiming.h`). Adequate here; thin rails at 15 m/s were not tunnelled by the snap approach. |
| G-03 | Composed (M59) worlds disable disk saves (documented). The consequence for progression is in E-05. |
| G-04 | Cloth/deformation/destruction absent from the frozen checkpoint (not recreated). |
| G-05 | One active gamepad; axial (not radial) stick deadzones; no rumble (documented in M35). **Controller hardware was not available on this machine — controller feel untested.** |
| G-06 | CharacterMotor is a gravity-upright walking capsule by design; not suited to board physics. That's acceptable, not a defect. |
| G-07 | No root-motion gameplay (documented). Baking clips in place is standard content work. |

## 18. Recommended priorities

1. **Fix D-01 (+D-02)** — correctness of an existing, documented primitive; generic and cheap to regression-test with the supplied repro.
2. **E-03 static mesh/convex collision (+ rotated compound children)** — the largest feel/authoring cost for any curved-surface game. It also unlocks C-01 (collider-from-mesh).
3. **E-01 joint-transform read API**, then **E-02 runtime limb IK targets** in the existing pose stack — evidence-backed by the feet/board study. E-01 alone removes most hacks.
4. **F-01 sleeping for resting bodies/articulations** (or a JS-controllable fidelity policy).
5. **E-05 project-owned persistent profile storage.**
6. **Editor bulk authoring (C-01/C-02/C-03)** — multi-select, search, component copy/paste, a skeleton picker for ragdolls, and simple profile/arc geometry tools.

## 19. Explicitly rejected engine conclusions

The audit does **not** conclude that Judas needs any of the following:
- **A native SkateboardComponent, wheel/suspension/vehicle subsystem, or trick system.** The JS chassis on public queries/velocities works, and the spring-suspension energy gain was my formulation (J08), not the engine.
- **A native grind-rail component.** Grinding works with velocity snapping. The generic gaps are D-01, E-04 and (optionally) a path primitive — not "rails".
- **A SkateCamera.** The chase/orbit/vert/bail camera is ~70 lines of JS on `setView` + `sphereCast`; no camera primitive was missing.
- **Special physics friction for skating.** Rolling vs lateral grip is game policy, done in JS. E-06 is only about material identity and runtime change.
- **Active physical animation.** Bails are ordinary passive ragdolls. Recovery/get-up was done as visual return + respawn. A real get-up would need content (get-up clips) plus E-01, not a native recovery system.
- **CharacterMotor changes.** Its walking semantics are fine; only B-01 documentation is requested.
- **Faster physics in general.** Rigid physics is ~0.08 ms in ordinary play. The cost problem is specifically resting articulations (F-01).

---

## 20. Project / evidence locations

| What | Path |
|---|---|
| This report | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/SKATE_GAME_ENGINE_AUDIT.md` (copy in `Judas/projects/skate_game/`) |
| Investigation journal | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/audit/JOURNAL.md` |
| Scenario evidence | `audit/evidence/scenarios/<name>/{shot.png, run.log, profile.json, profile_summary.json}` |
| Profile table | `audit/evidence/profile_table.md` |
| Architecture/spring labs | `audit/evidence/lab/*.log`; `Scenes/lab.judas`, `Scenes/springlab.judas` |
| Slider bug repro | `audit/evidence/slider_repro/`; joint-grind lab logs `audit/evidence/jointgrind/` |
| Feet/board measurements | `audit/evidence/feet_measurements.txt`, `blend_feet_ride_air.txt`, `blend_feet_ride_crouch.txt` |
| Editor autotest | `audit/evidence/editor/` (edit/play screenshots, log, editor-saved scene) |
| Export evidence | `audit/evidence/export/` |
| Game project | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/Judas/projects/skate_game` |
| Standalone | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/export/JudasSkate/judas` |
| Source assets | `/home/conner/Documents/GitHub/ClaudeJudasSkateGame/Assets` (unzipped working copies in `work/`) |

**Repository safety (verified at handoff).**

Authoritative `/home/conner/Documents/GitHub/Project-Judas`:
- HEAD = main = origin/main = `a9c6cd7…` (unchanged), and `ls-remote origin main` = `a9c6cd7…`.
- Still 34 tags and the same branches (`main`, `origin/main`).
- Nothing was pushed from the clone; its push URL is disabled.

The authoritative working tree now shows 61 modified/untracked paths:
- They are in-progress **M61 saves** work (e.g. `docs/M61_SAVES.md`, `src/MotorPersistence.cpp`, `src/PhysicsWorld.cpp`).
- Their timestamps are 23:02–00:13, after the clone was taken.
- This audit only read that repository, wrote nothing to it, and ran only the clone's binaries. These changes are the operator's parallel development and were neither touched nor evaluated.

Engine-source changes in the clone: none.

## 21. Human checklist

Run `./build/judas projects/skate_game/skate_game.judasproj` from the clone root (or the standalone package):

1. **Basic skating feel** — W to push off the deck, roll in; does speed build and bleed believably?
2. **Momentum & steering** — carve A/D at low and high speed; brake with S; is turning responsive without sliding out?
3. **Ramps/transitions** — funbox crests, roll-in bottom, 3.2 m QP, mini half-pipe. Watch for jitter at facet seams, unexpected launches, energy gain/loss.
4. **Jumping** — ollie (hold Space, release) on flat, uphill, downhill, off the kicker; land on slopes and while slightly rotated.
5. **Tricks** — J with directions for flips (does the board stay under the tucked feet?), K grabs, Q/E spins, K on ground for manual (balance with W/S), vert air on the QP.
6. **Grinding** — L near the handrail, flat bar, ledges, QP coping, and street rail (north gap in wall). Check the balance meter (A/D) and pop off with Space.
7. **Camera** — high speed on the street, rapid turns, vert air, inside the mini ramp, next to walls; mouse/stick orbit; C toggles distance.
8. **Bail/ragdoll** — slam into a wall at speed, land a flip late, X to bail; ragdoll quality, camera framing, respawn (Space after 1.2 s).
9. **Responsiveness/performance** — input latency, frame pacing, streaming hitches on the downhill street.
10. **Editor/game viability** — open in `judas_editor`, inspect the hierarchy (164 generated objects), Play/Stop, try editing a ramp facet or the rider's ragdoll mapping by hand, and Export project.

Also listen: rolling-sound pitch with speed, grind loop, pop/land/bail one-shots, music pause. Try the language button in the pause menu. If a controller is available, test sticks/triggers/face buttons (untested here).

---

## Priority summary (brief §36)

| ID | Finding | Class | Severity | Generic? | Blocks game? | Suggested engine direction |
|---|---|---|---|---|---|---|
| D-01 | Slider joint diverges beyond ~1–2 m travel (zero-error repro) | D | HIGH | Yes | Blocks constraint grinding only | Separation-independent slider rows + long-travel regression test |
| E-03 | Static colliders limited to boxes/spheres/unrotated compounds | E | HIGH | Yes | No (workaround) | Static triangle-mesh/convex colliders, rotated compound children, mesh→collider |
| E-01 | No JS read of resolved joint transforms / sockets | E | HIGH | Yes | No | Read-only joint pose query + attachment sockets |
| C-01 | No procedural/curved geometry or collider-from-mesh authoring | C | HIGH | Yes | No (external generator) | Profile/arc tools, array/duplicate, fit-collider |
| E-02 | No runtime IK / target constraints in the pose stack | E | MEDIUM | Yes | No (visual limits) | Limb IK targets as pose contributions with JS targets/weights |
| F-01 | Resting ragdolls never sleep; 20 → spiral of death | F | MEDIUM | Yes | No | Island sleeping / JS-controllable fidelity |
| E-05 | No game-initiated persistent storage; composed worlds disable saves | E | MEDIUM | Yes | No | Project-owned bounded JSON profile store |
| E-04 | No collider introspection / closest-point query | E | MEDIUM | Yes | No | `physics.closestPoint`, read-only collider shape |
| C-02 | Single selection, no search/filter/copy-paste | C | MEDIUM | Yes | No | Multi-select, search, component copy/paste |
| C-03 | Ragdoll mapping typed per bone; no skeleton picker | C | MEDIUM | Yes | No | Skeleton picker/axis view/auto-fit template |
| C-04 | Character import needs heavy DCC preprocessing (FBX, 48 joints, root motion, scale) | C | MEDIUM | Yes | No | Import options: bone culling, root-motion strip, unit scale |
| B-01 | Motor `supported` false on uphill motion undocumented | B | MEDIUM | Yes | No | Document "departing" semantics |
| B-02 | No guidance for real-rig ragdoll frames | B | MEDIUM | Yes | No | Humanoid mapping example/tool |
| D-02 | Violated joint limit rebounds near-elastically | D | MEDIUM | Yes | No | Clamp limit-row bias |
| F-02 | Streaming integration units up to ~10 ms | F | LOW–MED | Yes | No | Smaller/splittable integration units |
| E-06 | No runtime friction change / hit material id | E | LOW | Yes | No | Physics material assets + id on hits |
| E-07 | Joints not creatable from JS | E | LOW | Yes | No | Runtime joint creation API |
| E-08 | Box primitive UVs ignore size | E | LOW | Yes | No | World/triplanar UV option |
| C-05 | One emitter per entity; ambiguous emitter error | C | LOW | Yes | No | Multiple emitters/entity; precise diagnostics |
| D-03 | "missing localization key" during async startup | D | LOW | Yes | No | Distinguish unpublished vs absent |

### TOP ENGINE FINDINGS

1. **D-01 — Slider joints are unstable at ordinary travel distances.** A single-body, zero-error reproduction diverges at ≥2 m separation, independent of mass, anchor type and frames. This is a correctness bug in an existing, documented primitive.
2. **E-03 — Static collision is limited to box/sphere primitives (with unrotatable compound children).** Every curved surface becomes a seam-riddled chain of bodies. A rigid board stops dead at the first seam, and supplied curved art can't receive matching collision. Racing, terrain and any curved architecture face the same issue.
3. **E-01 (then E-02) — Animated poses are write-only from JS, and there is no runtime IK target.** Foot/board and hand/board contact could only be faked with offline-IK clips plus crossfade-fraction sync. Any grounded character or prop-holding game hits the same wall.
4. **F-01 — Resting articulations never sleep.** Ragdoll cost is ~1–2 ms per step each, with a fixed-step death spiral at ~20.
5. **E-05 — There is no project-controlled persistence.** Progress can't be saved, and adopting M59 streaming disables world saves entirely.
