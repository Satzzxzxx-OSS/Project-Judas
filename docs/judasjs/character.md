# CharacterMotor via entity.character

[Index](../JUDASJS.md) · [Input](input.md) · [Physics](physics.md) · [Animation](animation-ragdolls.md)

Judas owns collision-aware motion. JavaScript owns character behaviour. No C++
walk/run/jump/swim/camera/animation state belongs to this component. Author a
Character motor and Scripts through the editor; normal prefab overrides/spawning work.

## Character

`entity.character` returns a Character facade or null. `id` is plain opaque owner.
`state` returns detached `{velocity,actualDisplacement,supportNormal,supportVelocity,
gravity,up,supported,collided,supportEntityId,supportEntity}`. Convenient direct
getters: `velocity`, `supported`, `supportNormal`, `supportVelocity`,
`actualDisplacement`, `gravity`, `up`. Support entity may be null/stale.

- `velocity = {x,y,z}`: world-space script velocity, m/s, including previous support
  carry. Enforced fixed phase. The motor clips inward blocked components without
  erasing tangential/outward motion.
- `accelerate(vector)`: extra m/s² sum for the next motor step, then clears;
  enforced fixed phase; returns true. GravityField sampling remains authoritative.
- `enabled = bool`: setter only (read undefined), clears support when disabled.
- `configure(settings)`: validated partial patch, returns true or throws; DOES NOT
  accept enabled (use setter). No authored game-speed fields.
- `ignore(entities)`: replace ignored generation-safe body list, up to64; bodies
  absent from valid entities are skipped. Returns true.

Configuration: `radius`, `halfHeight` (capsule cylinder half-height), `offset`
(model-local metres), `stepHeight`, `supportDistance`, `skin`, `maxSlopeDegrees`,
`gravityScale`, `reorientationDegreesPerSecond`, `interactionMass`, `maxPushImpulse`,
`collisionLayer` (registered name), `collisionMask` (names), `requiredTags`,
`excludedTags`. Defaults/ranges are in [motor architecture](../M49_CHARACTER_MOTOR.md).
Unknown object properties are ignored; typings help detect mistakes.

Gravity is current acceleration sampled at the motor, NOT a support normal.
Up comes from local/gravity orientation; negligible gravity retains orientation,
never assumes global Y. GravityScale changes motor acceleration response, not the
GravityField. Support uses actual resolved body transforms and v+ω×offset; not
transform parenting/attachment. Supported motion replaces old support carry,
departure retains world momentum. Displacement includes steps/recovery/carry;
it isn't necessarily `velocity*dt`. Force APIs on Entity are for rigid bodies;
Character velocity/extra acceleration are its separate intent path.

Current limits: query-only motors don't block each other or generate M42 events;
unit entity scale, no simultaneous root rigid body/ragdoll, bounded recovery/slide,
short lip probe can add up to0.08m reported travel, approximate capped dynamic push,
teleported supports aren't continuous motion. Skeleton/animation is independent.
Disabling/removing scripts does not inherently disable their motor; last velocity
and gravity remain until explicitly changed/disabled. No fluid solver is owned.

Ordinary reload/Stop reconstructs authored motor state. Explicit [M61 slots](saves.md)
preserve velocity/orientation, support state and its supporting identity, then
reconstruct generation-safe handles. Do not retain a support wrapper across Load.

Use [conserved `liquid.sample`](liquid.md) for M54/M55 water, or
[`world.fluidSample`](effects-camera.md#worldfluidsample) for the separate legacy
PBF field, to implement buoyancy/drag/propulsion in project JS. Existing
coarse production liquid limitations still apply; do not turn the motor into a
hardcoded swim mode. Copyable [motor example](examples/character.js), current full
project [character demo](../../projects/character_demo/character_demo.judasproj).
