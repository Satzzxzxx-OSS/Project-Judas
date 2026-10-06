# Rooftop Run

A small first-person parkour time trial in the style of Mirror's Edge, built on
Project Judas `e452751` with only public JudasJS APIs and game-owned content.

You run across six rooftops past five checkpoints to the green beacon. White roofs
and red "runner vision" objects show where you can vault, wall-run, climb and slide.

## Run

From this folder, using the engine clone next to it:

```bash
../Project-Judas/build/judas rooftop_run.judasproj
```

Or the exported standalone package:

```bash
../RooftopRun_Package/judas
```

You can watch the autopilot run the whole course (this is the test scenario):

```bash
../Project-Judas/build/judas autotest.judasproj
```

## Controls

| Action | Keyboard / mouse | Controller |
|---|---|---|
| Move (hold W to build speed up to 8.4 m/s) | WASD / arrows | Left stick |
| Look | Mouse | Right stick |
| Jump / vault / wall-run / wall-climb / wall-jump | Space | A / LB |
| Slide (while running) / roll (just before a hard landing) | Left Ctrl or C | B / RB |
| Respawn at checkpoint | R | Y |
| Restart run | Backspace | Back |
| Pause menu | Esc | Start |

## Moves

- **Vault**: run at a red obstacle up to about 1.4 m high and press Space.
- **Ledge grab**: in the air, hold forward into a ledge you can reach (up to
  2.25 m above your feet). You climb up automatically.
- **Wall-run**: jump toward a wall at an angle while holding W. Press Space during
  the wall-run to wall-jump away from the wall.
- **Wall-climb**: jump straight at a wall while holding Space + W to run up it.
  It chains into a ledge grab. Press Space during the climb to turn and kick off.
- **Slide**: press Ctrl/C while running to fit under 0.95 m gaps. You keep
  crouching until there is headroom.
- **Roll**: big drops (landing faster than about 9.6 m/s) stun you and kill your
  speed. Press or hold Ctrl/C as you land to roll and keep your momentum.
- Coyote time and a jump buffer make jumps at roof edges more forgiving.

## Layout

- `Assets/scripts/runner.js`: movement controller and first-person camera.
- `Assets/scripts/course.js`: timer, checkpoints, falls, finish, HUD and pause menu.
- `Assets/scripts/autopilot.js`: scripted test inputs.
- `tools/build_course.py`: generates the scene, HUD, project/input map,
  checkpoint data, audio clips and sky. Edit this, then run
  `python3 tools/build_course.py`.
  To build a test scenario, run `python3 tools/build_course.py --test <scenario> [logEvery]`. That
  writes `autotest.judasproj`. Bake the sky with `../Project-Judas/build/judas_environment_bake tools/sky.hdr Assets/environment/sky.judasenv 128 128`.

Headless test example (prints JSON state lines prefixed `JS:`):

```bash
printf 'STEPS 2400\nLOG_EVERY 6000\n' > /tmp/h.txt
python3 tools/build_course.py --test course 30
JUDAS_TEST_SCRIPT=/tmp/h.txt ../Project-Judas/build/judas autotest.judasproj | grep '^JS'
```
