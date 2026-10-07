# Judas | Three Real Games

An ordinary consumer project on accepted Judas M65
`51c18c249432e09910dabc41e670bf960db8050c`. No custom engine mode or
private native dispatch. The three original games retain their game rules.

## Launch and switch

From the Judas checkout:

```sh
./build/judas projects/post_m65_consumers/post_m65_consumers.judasproj
./build/judas_editor projects/post_m65_consumers/post_m65_consumers.judasproj
```

Export with the editor's Export Project command, or:

```sh
./build/judas_export projects/post_m65_consumers/post_m65_consumers.judasproj /tmp/JudasThreeGames ./build/judas
/tmp/JudasThreeGames/judas
```

The reviewed export is `.cache/post-m65-package/JudasThreeGames`; the copy
outside the repository is `/tmp/JudasThreeGames-post-M65`. Launch `judas` inside
either directory, from any working directory. Linux requirements are recorded
in its `RUNTIME_REQUIREMENTS.txt`.

| Everywhere, including pause menus | Action |
|---|---|
| F1 | Skate |
| F2 | Rooftop Run |
| F3 | Void Courier |
| F4 | Collection menu |

Menu buttons and normal keyboard/controller focus also select a game.
Switching reconstructs the selected game's world; it does not preserve an
unfinished run. Each game's original restart and pause controls still work.
Skate's explicit save slot survives switching; session best/locale values last
for this application session. Do not load a Skate save while another experience
is active: there is one project-wide save service, with ordinary content checks.

## Controls

**Skate:** W push, S brake, A/D steer; Space hold/release ollie; J flip (+direction),
K grab/manual, L grind, Q/E spin, F + direction invert, Shift boost, X bail,
C camera distance, mouse look, T checkpoint, R restart, Escape pause.
Controller bindings from the original project are retained.
Pause menu: save/load quick run, English/Spanish, resume/restart/quit.

**Rooftop Run:** WASD move, mouse look, Space jump/vault/climb/wall-run,
Ctrl/C slide/roll, R checkpoint respawn, Backspace restart, Escape pause.
Build speed before attempting the wall-run. Course hints and checkpoint banners
remain the original game's rules.

**Void Courier:** WASD move/thrust, mouse look, Space jump/ascend,
Ctrl/C descend, Shift run/boost, F board/exit when nearby/landed,
left mouse fire, Q/E roll in flight, V chase/cockpit view, X flight assist,
Backspace restart, Escape pause. Goal: board, clear wave, retrieve Ruby's core,
clear ambush, return home. Original raider scripts are retained.

## Structure and ports

- `Scenes/launcher.judas` and `Assets/collection/`: small JS/UI collection menu.
- `Scenes/skate/`, `Assets/skate/`: latest supplied Skate source, including its
  M61 saves and M64 smooth collision cooks. Lab scenes remain available to
  developers but are excluded from the exported scene set.
- `Scenes/rooftop/`, `Assets/rooftop/`: supplied Rooftop package; real M42 motor
  sensors replace checkpoint AABB polling; uphill movement now projects along
  either incline direction. Original wall-run and other mechanics remain JS.
- `Scenes/void/`, `Assets/void/`: supplied Void package; ship/raider scripts sample
  `physics.gravity`. The original pilot-hit approximation is retained because
  the current cast path does not correctly intersect target capsules.
- Each game has `scripts/collection_input.js`: a thin public logical-input
  facade. Names are project-namespaced, bindings unchanged; UI navigation shared.
- Asset IDs are deterministically namespaced; public scene paths and the Skate
  manifest are remapped. Original workspaces and executables are not modified.
- Skate presentation reads resolved toe joints rather than guessing lift from
  clip names. Authored deck/rider references use M65 entity properties.

The full original-finding comparison, remaining defects, measurements and
human checklist are in [the review](../../docs/POST_M65_CONSUMER_REVIEW.md).
[Import provenance](../../docs/evidence/post_m65_consumers/import-provenance.json)
records source paths/hashes and asset-ID mapping. Font licences are retained
under each game's `fonts/LICENSE.txt`; the original games' content is copied,
not claimed as new art. Engine dependency notices ship through the exporter.

## Known review limitations

This is a playable integration review, not a certification that M65 solved all
reported problems. Skate's streaming pin checks currently impose substantial
resident-frame overhead. Void's inert-entity scaling and camera/UI limits
remain. Real imported humanoid ragdolls do not consistently settle into sleep.
See the report before increasing entity, active-region or ragdoll populations.
Human control feel, controller hardware and listening acceptance remain pending.
