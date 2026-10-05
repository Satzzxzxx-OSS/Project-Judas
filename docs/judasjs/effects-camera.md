# Audio, particles, camera and liquid field

[Index](../JUDASJS.md) · [Entities](entities.md) · [Character](character.md)

## Audio

Author AudioEmitter/Listener components and registered WAV/MP3/FLAC assets.
[Audio playback and acoustics](audio.md) is the current reference for emitter
controls, bounded streams, motion, obstruction, reverb, sound groups and lifetime.

## Particles

`burst(count)` queues an authored visual emitter burst; boolean success, false
when no emitter. Count is converted to uint32 and must be <=65536; use a nonnegative
integer. Capacity/lifetime remain authored. `setParticles({enabled?,rate?})`
changes runtime enable/emission rate only; boolean success, false without emitter,
invalid rate/settings throw. No JS readback, collision, arbitrary particle-position
editing or fluid-particle API exists. Pose follows entity transforms. Offscreen
emitters continue ageing; rendering uses normal per-camera culling.

## Cameras

`camera` on Entity returns authored render-target camera `{enabled,width,height}`
or null. `setCameraEnabled(bool)` returns success. No JS target resize/cadence,
render-layer-mask setter or generated texture resource handle is exposed. Author
those settings through normal M33/M39 scene/editor data.

`world.setView(pose,fov=70)` publishes a separate runtime main view; partial transform
omissions default to identity (not the previous view). FOV is degrees, strictly
between 1 and 179. Rotation normalizes; invalid pose/FOV throws. Scale is not view
projection. `clearView()` restores the compatibility main-view path. Both return
true. This view is world-owned and clears on reset/destruction. It does not move a
motor or create another simulation. Scripts choose camera behaviour separately.
For a moving-body/motor camera, publish in `presentationUpdate` from
`entity.presentedTransform`, keeping position/orientation on the renderer's
interpolated timeline. Read look input in `update` and preserve authoritative
`transform` for physics queries. See [lifecycle](lifecycle.md).

`world.viewRay` is `{origin,direction}` from the latest view passed to ScriptSystem,
or null before any. It can lag rendered/input state; it is not a recomputed ray
from the current assignment. Prefer explicitly supplied geometry for authoritative
queries. Main view feeds normal presentation/audio; secondary cameras stay separate.

## world.fluidSample

`fluidSample(point,up,halfHeight,radius,tangent)` returns detached
`{immersion,density,velocity,acceleration}` from the existing production particle
field. Point/geometry in metres, density kg/m³, velocity m/s, acceleration m/s².
Up normalizes; tangent must not be parallel to up. Nonnegative half-height and
positive radius required. Dry worlds return zero/dry data. This is approximate
field sampling, not direct water ownership, exact pressure or a new fluid solver.
Use sampled liquid acceleration with Judas gravity for project-specific swimming
behaviour; the motor provides geometric motion, not a built-in swim mode.
Existing 20 Hz demo/coarse-fluid limitations remain [documented](../FLUID_DEMO.md).
