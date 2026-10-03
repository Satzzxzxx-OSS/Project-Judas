# Spring Range — an ordinary JudasJS game

Shoot all 12 hinged plates at least once. Orange / cyan / pink rows score
100 / 150 / 200 points. A hit scores only while the plate is ready; the real
hinge spring must return it near rest before it scores again. Clearing every
plate completes the round; you can keep practising or restart. The crate reacts
to shots but never scores. Best score survives round reload for this session.

## Play

From the repository: `./build/judas projects/shooter_game/shooter_game.judasproj`.
The M52 clean Release executable is `.cache/m52-release/judas`.
Open the project in Judas editor for normal Play/Stop and Export Project.

| Control | Keyboard/mouse | Controller |
|---|---|---|
| Move | WASD | Left stick |
| Aim | Mouse | Right stick |
| Fire (one shot per press) | Left mouse | Right trigger |
| Launch | Space | South / A |
| Camera | V | North / Y |
| Restart | R | Back |
| Pause | Escape | Start |
| Menu | Mouse or arrows + Enter | D-pad + South |

## Source tour

- `Assets/scripts/player.js`: logical input, CharacterMotor intent, script launch.
- `camera.js`: first-person and collision-tested shoulder camera; same player.
- `weapon.js`: camera intent ray, player-origin obstruction ray, actual hit-point
  impulse, effects. No damage/weapon class exists in engine C++.
- `target.js`: reads hinge state; scores via `round.js` and rearms on physical return.
- `round.js`: scene-VM score and per-target game state.
- `hud.js`: authored UI text/menu policy, session best, reload/quit.
- `math.js`: small local project vector/quaternion helpers.
- `Scenes/range.judas`: motor, 12 plates, normal M45 hinges, crate, arena and emitters.
- `Assets/ui/range.judasui`: anchored HUD/reticle and pause buttons.

Change `speed`, `launchSpeed` or `shotImpulse` defaults in `player.js`, or a target's
`points` script property in the scene; reload. No C++ compilation is required.
The project schema uses the normal strict authored/script fingerprint rules.

## Boundaries and approximations

Flat uniform gravity is authored content; locomotion reads motor gravity/up and
support. First-person cosmetic avatar meshes move below the arena; they have no
bodies. In third person they follow the motor's render-interpolated pose in
`presentationUpdate`, together with the camera. Shooting/motion use authoritative
`transform`; visual follow uses `presentedTransform`. Camera, motor and physics
remain distinct. Two rays keep an offset camera from firing through player-side
cover; this is basic hitscan, not a weapon/ballistics framework. Camera toggling
can change the target beneath the reticle because of parallax. Camera boom is one
sphere cast, without smoothing or a full camera collision solver.

A cooling plate still receives impulses, but doesn't score until it settles.
There is no enemy AI, navigation, damage, inventory, projectile or save system.
Sounds reuse authored emitter voices (no polyphonic gun mixer). Audio, visual game
feel and real controller hardware still require human validation.

Exports use the normal Linux package pipeline and its system-library requirements.
Developer API reference: `docs/JUDASJS.md` and `docs/judas.d.ts` in the engine repo.
