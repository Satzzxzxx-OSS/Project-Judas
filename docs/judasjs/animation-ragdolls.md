# Animation, pose layers and articulated ragdolls

[Index](../JUDASJS.md) · [Physics/joints](physics.md) · [Character](character.md)

Judas owns skeleton/pose infrastructure. Clips, physics and future producers
contribute to a resolved pose; no producer owns bones permanently. JS controls
WHY playback/physical transitions occur; it cannot access raw pose arrays or GL.

## Animation

`entity.animation` returns a facade/null. Its `entityId` is a plain opaque field.
`info` returns `{ready,playing,loop,speed,time,clip,clips,transitioning,
transitionFraction,error,joints,layers}`. `clips` are `{name,duration}` (seconds);
`joints` are stable hierarchy keys. `layers` contain only
`{id,clip,weight,enabled,additive}`, not all settings. Empty clips/error while async
asset loads: check ready. A facade's existence doesn't imply successful mesh import.

| Member | Behaviour |
|---|---|
| `clips`, `playing`, `time`, `layers` | Convenience detached info reads. |
| `speed`, `loop` | Read/write finite speed and boolean; speed may be negative (existing playback supports reverse). |
| `play(clip='')` | Named clip resets time/mixer; empty string resumes current playback. Unknown ready clip throws. |
| `pause()` | Pauses playback/mixer, true even if asset not ready. |
| `resume()` | Alias for `play()` with empty clip. |
| `stop()` | Reset through existing playback stop/rest pose; clears mixer. |
| `seek(time)` | Finite nonnegative seconds; sampling handles clip wrap/clamp. |
| `crossFade(clip,seconds=.3)` | Existing weighted shortest-path quaternion mix; interrupted contributors remain deterministic, zero duration immediate. Duration 0..3600. |
| `layer(id,settings)` | Add/update an ordered contribution; patch existing settings. |
| `removeLayer(id)` | Remove if present; true even absent when asset ready. |

Except pause/settings, commands return false while asset unavailable (not a queued
command). Validity checks still apply. Unknown clips/masks/settings throw once
ready. Up to 16 authored/runtime layers and 16 interrupted fade contributors.

`AnimationLayerPatch`: `clip`, `referenceClip`, `weight` [0,1], `speed`, `time`,
`referenceTime`, `enabled`, `additive`, `mask` (up to128 joint keys). Missing
fields retain existing settings, or defaults for a new layer. New layers need a
valid clip; empty mask affects all joints. Masks affect local joints, not automatic
subtree expansion; parents/children compose normally. Updating the same clip
preserves its playback clock (including when supplying time); do not promise a
separate layer seek API. Enabled layers advance with the mixer; base loop/seek
controls do not automatically control each layer's clock.

Ordinary layers blend absolute sampled poses in insertion order. Additive layers
use explicit reference clip/time or rest pose, translation/rotation/scale delta;
not an arbitrary absolute clip treated as an offset. Stable joint keys aren't
humanoid assumptions. C++ exposes validated external sources used by ragdolls;
**no arbitrary external pose source/raw bone setter exists in JS**.
[Pose implementation/authoring](../ANIMATION.md).

## Ragdoll

`entity.ragdoll` returns facade/null for authored mapping. `id` is its opaque owner.
`active` boolean reports articulation. `enter()` activates from CURRENT resolved
pose, recent observed motion where available, returns true or throws. `leave(seconds=.4)`
captures physics pose, retires bodies/constraints, and visually fades back to
animation; duration 0..3600, zero immediate. `enabled` is setter-only (read undefined).
`body(jointKey)` returns mapped ordinary Entity or null; check validity before
force/impulse/query use. No built-in death/recovery/balance or pose-driving motors.

Mappings and constraints are authored via ordinary scene/editor/prefabs; JS does
not construct mappings. M45 joints/normal PhysicsWorld own articulation, M47
resolves its physics contribution into the same skeleton. Owner translation
follows physical root; reference orientation/scale remain fixed. Mapped body
collisions are per-body, not owner-aggregate. Asset replacement/destruction/disable
cleans articulation; mapped transient entities are not independently saved.
Active ragdoll and mixer state are not persisted. Uniform positive mapped scale
only, no owner rigid collider; partial active physical control is not provided.
See [mapping/lifecycle limits](../RAGDOLLS.md).
