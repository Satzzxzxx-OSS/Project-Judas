# Follow-up: skate game on latest Judas main (`19a53a4`)

Branch `skate-latest` in the clone = latest main + the game (`git merge` of `skate-audit`,
no conflicts; game-only paths). New engine work since the frozen checkpoint: M61 save
slots, PR #1 (D-01/D-02/D-03), M62 deformables, M63 fracture, M64 mesh/convex collision.

## What changed in the game

| Audit finding | Engine change used | Game change | Result |
|---|---|---|---|
| E-03 box-only colliders | M64 cooked static triangle meshes (`judas_collision_cook`) | Every ramp (roll-in, funbox, kicker, landing, platform bank, big QP, mini half-pipe ×2, east bank, streamed street kicker) is one cooked smooth surface + side skirts, ~3× finer curves; flat geometry stays boxes. `colliders=boxes` keeps the old build for A/B | Park 164 → 78 objects, 156 → 70 bodies; seam "normal jumps" route 5→0, mini 14→8, backflip 3→1; identical scores/bails in all A/B scenarios; energy unchanged (chassis already compensated seams); fixed step 1.02 → 0.81 ms median |
| E-04 no collider introspection | `Entity.collider` | Rails read their grind line from their box collider; `rail.js` no longer duplicates length/height | 8 park rails + streamed street rails detected as before |
| E-05 no game persistence | M61 `saves` slots | Pause menu SAVE RUN / LOAD RUN (one quick slot); rules mirrored into `this.state`; `restore()` lifecycle | Works in the streamed (composed) project: save at z=-10.5 m, load restored position, 6.3 m/s velocity, score 1383 and timer 176.0 s |
| D-03 log noise | PR #1 | — | 0 "missing localization key" lines at startup (was ~16) |
| D-01/D-02 | PR #1 | — | Not exercised by the game (grinds use velocity snapping) |

## New observations

- M64's cooker validation is strict and useful. It caught real topology errors in my generator:
  T-junctions where a quarter-pipe's vertical lip met the side skirts. That was fixed by triangulating around the lip.
- The supplied artist half-pipe could **not** be cooked: it failed three checks in turn.
  - "inconsistent adjacent winding" — fixed with Blender recalculate-normals;
  - T-junctions — 63, repaired by script (`Tools/fix_tjunctions.py`);
  - "nonmanifold collision edge" — stopped there.
  
  Diagnostics name the error class but not the triangle/edge location, and the editor has no repair step.
  For real art the documented route is still a separate clean physical source (LOW–MEDIUM workflow finding).
- The runtime still prints "World state: disk saves disabled for additive compositions" at startup even though
  M61 slot saves work in that same project. That refers to the legacy `.judasstate` path, but it now reads as
  "you can't save" (LOW diagnostic wording).
- Save while a save is committing throws `save/scene service busy` (documented one-operation rule); handled in JS.
- M62 cloth and M63 fracture were not added: they need offline-authored `.judasdeform` assets and would be new
  features (breakable props, cloth flags) rather than simplifications.
