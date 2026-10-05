# Streamed Spring Range — M59 current project

Open `streamed_range.judasproj` in Judas editor, or run:

```sh
./build/judas projects/streamed_range/streamed_range.judasproj
```

This variant preserves one player, camera, localized HUD and round while six
registered galleries load and release in one runtime world. The accepted original
`projects/shooter_game` remains a separate unchanged regression reference.

## Controls

WASD / left stick moves; mouse / right stick looks; left click / right trigger
fires; Space launches; V changes camera; Escape pauses; R reloads; L changes locale.
F9 requests the next gallery, F10 releases manual requests. Spatial interests or
safety pins may still retain it. F7 requests the small conserved liquid exhibit.
Q picks/releases a nearby non-target physical prop using finite forces, adopting
its hierarchy into root ownership so it survives gallery suspension.

Walk south through successive galleries, shoot the hinged targets, and watch the
navigator cross the normal navigation link using its CharacterMotor. Revisit a
changed prop/target: physical state, JSON script state and destruction tombstones
survive automatic suspension. Deliberate R reload resets the round and regions.

The localized panel reports states, pins, estimated memory and integration work.
A project-scripted travel gate stops intent toward unavailable collision geometry;
no hidden engine collider or teleport is involved. Editor World regions shows
requests/pins, source edit targets and read-only additive preview. Request the
`oblique` fixture there to inspect its rotated/radial field; it is not a
planet-streaming or rebasing system.

## Source tour

- `Scenes/bootstrap.judas`: root/player/view/HUD, floor, fallback gravity/navigation.
- `Scenes/gallery-*.judas`: separately authored targets/props/navigation.
- `Scenes/liquid.judas`: 1000 L conserved, pinned exhibit.
- `Scenes/oblique.judas`: arbitrary placement/gravity fixture.
- `Assets/world/range.judasworld`: stable regions, placement and byte/work budgets.
- `Assets/scripts/residency.js`: interest, manual demand, carry policy, readiness gate.
- Other scripts retain ordinary JS movement/cameras/shooting/target/nav behaviour.

Export Project includes all registered runtime content. Move the resulting package
and launch its game from any working directory. Documentation, limitations,
component matrix and evidence: [M59](../../docs/M59_WORLD_STREAMING.md).

Human visual/travel/carry acceptance is pending. A liquid or pose/articulation group
is explicitly pinned because M59 does not invent lossless native-state suspension.

## M60 audio integration

Persistent-root 180-second streamed music continues across gallery travel. Gallery-local streamed ambience and alternating reverb settings use normal components. Q can adopt/carry the softly sounding physical prop out of its birthplace. Gallery removal retires streams asynchronously without pinning the region. Pause holds the effects group while music and UI remain independent. Shots request bounded independent buffered voices. The current [listening lab](../audio_lab/README.md) isolates audio comparisons.
