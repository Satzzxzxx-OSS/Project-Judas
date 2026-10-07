# Rooftop Run: Judas engine experiment report

## Setup

- **Engine under test:** `e452751ee98f6c1900a9f6b8dad3fe6bcdecb27b` ("Add M64 mesh and
  convex collision with geometric queries"). I cloned it from `origin/main` (GitHub)
  into `ClaudesEdge/Project-Judas` and confirmed it matched the local repo's HEAD.
  The commit is recorded in `notes/STARTING_COMMIT.txt`.
- **Engine source not modified:** `git status` in the clone is clean. The original
  `~/Documents/GitHub/Project-Judas` was not touched. I built the Release targets `judas`,
  `judas_environment_bake` and `judas_export` (0 warnings).
- **Mid-experiment docs update (`19a53a4`, "Re-alligned Documentation"):** I fetched
  it for reading only and did not check it out. It changes 13 doc files and no source.
  It mostly realigns the save/`restore()`, collider and materials docs to M64. It does
  not cover any of the issues below, so none of the conclusions change.
- **Game:** `ClaudesEdge/RooftopRun/`, outside the engine tree. The exported package
  is `ClaudesEdge/RooftopRun_Package/`.

## What was built

- A course of six rooftops (about 175 m) with five checkpoints and a timed finish.
- Mechanics: momentum-based running, jump with coyote time and a jump buffer,
  variable jump height, vault, automatic ledge grab and mantle, wall-run, wall-jump,
  wall-climb and climb-turn-kick, slide under obstacles (the capsule is resized at
  runtime), landing roll and hard-landing stun, falls with checkpoint respawn.
- Camera: head bob, wall-run tilt, speed-based field of view, landing dip, a
  somersault during rolls, and a 180° turn for the climb kick.
- HUD: timer, speed, move state, hints, checkpoint banners, best time for the
  session, and a pause/finish menu.
- Audio, generated procedurally: wind that rises with speed, footsteps, landings,
  and whooshes.
- A procedural sky, baked into an environment.

## Verification (automated, headless)

The engine's harness can't send input to scripts (see E2). So the game has its own
autopilot that feeds the controller's input object from scripted steps. I ran it
through `JUDAS_TEST_SCRIPT`. Logs are in `notes/runs/`.

| Scenario | Result |
|---|---|
| vault (AC unit, rail wall) | pass |
| gap 1 (4.5 m) | pass |
| wallrun → walljump across 10 m alley | pass; lands on roof 3 |
| climb (3.8 m wall) → ledge → mantle | pass |
| slide under billboard | pass; **noslide** control is blocked at the billboard as intended |
| drop 5.6 m with roll | pass; keeps 8.4 m/s (without roll: hard landing, 1.5 m/s) |
| ramp + final gap → finish | pass |
| **full course** | **finished 23.78 s, 0 falls**, all 5 checkpoints in order |
| standalone exported package | launches, loads the scene, no errors |

I checked the visuals with real-loop screenshots (`JUDAS_TERRAIN_SCREENSHOT`) and one
desktop capture of the finish menu (`notes/runs/*.png`).

**Not verified (needs a human):** real keyboard/mouse/controller input, how mouse look
feels, the pause menu's Escape/click flow, R/Backspace, how the audio sounds, and
overall game feel. The harness can't drive any of these, and the machine has no
input-injection tool. The input code is a direct `input.axis/pressed/held` read
using the same pattern as the shipped demos.

---

## Engine / tooling findings

### E1. Harness screenshots ignore script-owned cameras (tooling bug)
`TestHarness.cpp:220` sets the camera from the legacy `player.GetViewMatrix()` and
then calls `drawScene`. That runs `PresentationScripts`, but the `world.setView` it
produces is never applied to the renderer. The comment at `Application.cpp:135`
("A harness screenshot shows exactly what the real game renders") is not true for
any modern CharacterMotor/JS-camera project.
**Repro:** in a project whose script calls `world.setView` in `presentationUpdate`
(for example `projects/shooter_game`), use a test script containing
`STEPS 60` / `SCREENSHOT 30 out.png` and run
`JUDAS_TEST_SCRIPT=... ./build/judas <project>`. The image comes from the
legacy player pose at the origin, the same for every step and pose.
**Workaround:** `JUDAS_TERRAIN_SCREENSHOT` (real loop, visible window, one capture after
600 fixed steps). I timed autopilot scenarios so the moment of interest lands at 10 s.

### E2. Harness cannot drive JudasJS input (tooling limitation)
`HOLD`/`TAP` set `Window` test-action state. Scripts read `InputSystem` instead
(`ScriptSystem.cpp:595-599`), which is never fed in test mode. Even `move_x/move_y`,
which `Window::InputAxis` bridges, read 0 from JS. There is also no directive for
named actions such as `jump`.
**Repro:** a script logs `input.axis('move_y')` in `fixedUpdate`. With a test script
`STEPS 240` / `HOLD W 0 240`, it logs `0` every step. (I checked this with this game's
logging variant: `python3 tools/build_course.py --test "" 30`, then run with `notes/runs/hw.txt`.)
The harness CSV also only reports the legacy player, which is all zeros for motor projects.
**Workaround:** a game-side autopilot that drives the same intent object, plus
`console.log` JSON telemetry.

### E3. CharacterMotor treats uphill motion along a slope as leaving the ground (engine semantics / limitation)
`CharacterMotor.cpp:59`: `departing = dot(velocity - supportVelocity, intentUp) > 1e-4`.
If a script sets the velocity along an inclined surface going uphill (the natural
thing to do on a ramp), that velocity has an upward component. The motor then counts
the step as departing and reports `supported=false` on every step. The character
"floats" up the ramp in air state, and grounded logic, jumping and coyote time all break.
**Repro:** put a motor on an 11° static box ramp and each fixed step set
`velocity = tangent(horizontalIntent, supportNormal)` rescaled to 7 m/s.
`state.supported` stays false while climbing. (This game before the fix:
`--test ramp` logged `"mode":"air","sup":false` with `vy` stuck at 0.79.)
**Workaround:** use horizontal intent uphill and let the motor's slide resolve the
slope; follow the plane only downhill. The docs mention that "explicit outward motion"
clears support-following. They don't warn that motion *along an uphill support*
counts as outward.

### E4. `supportNormal` at roof edges is the capsule-to-corner contact normal (documentation ambiguity)
When the capsule rolls over a box edge, `supportNormal` tilts (it's still within the
slope limit, so it still counts as support). The docs call support normals "actual
geometry normals", which suggests the face normal.
Projecting velocity onto it pulled the runner down at roof edges: vertical velocity
went from −0.6 to −3.2 to −7.9 m/s within about 0.2 s.
**Repro:** with the motor on a flat box, run off the edge while projecting velocity
onto `state.supportNormal`; vertical velocity spikes downward before support is lost.
**Workaround:** raycast straight down for the surface normal, and clamp the
vertical speed when walking off an edge. *(The projection itself was my bug; the
surprising normal is the engine side.)*

### E5. Motors produce no trigger/collision events (documented limitation)
Sensor bodies can't detect the player, so checkpoints, the finish and hints are
game-side box tests in `course.js`. This is documented; it's listed here because
any course/trigger-based game hits it.

### E6. No sky or background colour setting (missing capability)
The clear colour is hardcoded (`Renderer.cpp:627`, dark grey). The only way to get a sky
is an environment background. **Workaround:** the generator writes a procedural
Radiance `.hdr`, bakes it with the stock `judas_environment_bake`, and turns on
`environment-background` with linear rendering.

### E7. `.judasui` and scene files are positional and not documented for hand authoring (documentation gap)
The `.judasui` format is a space-separated positional record of about 50 fields. Its
only specification is `SerializeUIDocument` in `RuntimeUI.cpp:41`. `RUNTIME_UI.md`
covers editor authoring only. Writing a game code-first means reading engine source
to get field order and enum values (`UIKind`, `UIFlow`). Scene and input-map
serialization were also learned from existing projects and the parser source.
Nothing failed, but this was the biggest time cost after the controller itself.

### E8. Minor cosmetic issues
- The window title is always "Project Judas" (`Application.cpp:58`); the project
  name is never used, including in exported packages.
- Startup prints "none saved; F6 saves, F7 deletes" (`Application.cpp:126`) even
  when the project's input map doesn't bind `save_state` / `delete_state`.
- `JUDAS_TERRAIN_SCREENSHOT` only counts fixed steps, so it can never capture a
  modal menu (gameplay is paused). I used a desktop capture (spectacle) for the
  finish menu.

### Things that worked well
- `CharacterMotor.configure({halfHeight, offset})` can resize the capsule at runtime
  (used for slide/crouch). Writing `entity.transform` on a motor entity works as a clean teleport.
- `gravityScale:0` plus script-applied gravity gives full control over jump and
  wall-run arcs.
- `physics.capsuleCast/raycast` were enough for every probe: ledge, vault, wall, headroom.
- `presentationUpdate` + `presentedTransform` gave a smooth interpolated camera with
  no extra work. The stale-asset (duplicate id) error was clear and actionable.
- Export produced a working package on the first try.

## My own game-code bugs (found and fixed during development)

1. Velocity projected onto the tilted edge `supportNormal` → sudden plunge off roofs (see E4).
2. `jump()` reset the time-since-ground counter, so wall-climb never started.
3. Wall-run probe range and vertical-speed gate were too strict: the runner was
   falling too fast by the time it reached the wall (now 1.0 m and −6 m/s).
4. Uphill plane projection lost support on the ramp (see E3).
5. The roll window was too tight; holding slide at touchdown now also counts.
6. I left a stale `camera.js.judasmeta` behind, which caused a duplicate asset id.
   The engine reported it correctly.
7. The controls text was unreadable on white roofs; it now has a background panel.

## Known game limitations

- Wall-run uses rays from the capsule centre, so very short walls or walls with gaps
  can drop you early.
- Vaults and mantles are scripted moves (they ignore gravity while rising) with timeouts, not physical animation.
- There's no first-person body or hands (there are no animated assets).
- Best time lasts only for the session (`session`), not across launches. Save slots
  could store it but aren't used.
- Mouse sensitivity is a script property (`sensitivity`) with no in-game options menu.
