# Physics queries, dynamic motion, joints and touch events

[Index](../JUDASJS.md) · [Entities](entities.md) · [Character](character.md) · [Ragdolls](animation-ragdolls.md)

## physics

`import {physics} from 'judas'`. Queries are explicit read-only snapshot questions,
not contact events. They use the existing AABB tree and authoritative geometry.
Directions normalize internally; maximum/distance are metres, fraction is relative
to maximum distance. No all-hits binding exists.

| Method | Signature / result |
|---|---|
| `raycast` | `(origin, direction, maximum, filter={}) → CastHit|null` |
| `sphereCast` | `(origin, radius, direction, maximum, filter={}) → CastHit|null` |
| `capsuleCast` | `({position,rotation?}, radius, halfHeight, direction, maximum, filter={}) → CastHit|null` |
| `boxCast` | `({position,rotation?}, halfExtents, direction, maximum, filter={}) → CastHit|null` |
| `joint` | `(ownerEntity) → Joint|null`; owner is the entity authoring the joint component, not automatically a connected body. |

Pose-local Y is the capsule core axis, not universal world up. Capsule full height
is `2*(halfHeight+radius)`. Boxes use positive half extents. Default cast rotation
is identity. Casts translate with fixed orientation, no rotational sweep or target
motion prediction. Invalid input throws TypeError; zero maximum can test initial
overlap. Hits choose nearest distance, then body/primitive identities for ties.

`CastHit`: `{entity,entityId,bodyId,point,normal,distance,fraction,primitiveIndex,
initialOverlap,shape}`. Target surface point and outward normal; deep initial
overlap has a deterministic feature normal, not unique physical penetration data.
`shape` is `sphere`, `box` or `terrain`; compounds return child primitive index.
`bodyId` is an opaque generation-bearing number, not a controllable JS Body.
`entity` can be null for unassociated geometry; otherwise it may later become
invalid. Retained hit data does not update with the world.

`QueryFilter`: `includeLayers`, `excludeLayers`, `requiredTags`, `excludedTags`
are project-name arrays; `ignored` is Entity[] (up to 256), `includeSensors` defaults
false. Includes default to all, excludes to none; all required tags must match,
any excluded tag rejects. Unknown names throw. Query masks are independent of
physical body collision masks. Ignored missing bodies are skipped safely.

`world.overlap(min,max,filter={})` returns entity wrappers from conservative
broadphase bounds in ascending body-slot order, NOT exact overlap penetration
tests or an entity-ID sort. `world.sweepCapsule(from,
displacement,rotation=identity,filter={})` retains the existing player-sized capsule
(radius 0.3, cylinder half-height 0.6); result always has `{hit,distance,normal,entityId}`,
including miss. It sweeps a displacement, not a unit direction/maximum pair. Use
`physics.capsuleCast` for explicit shape dimensions. Neither returns a Character.

Terrain: ray/sphere/capsule sample actual RadialTerrain, using approximate radial
surface distance/bracketing. Thin/grazing features can be missed. Box casts reject
permitted terrain candidates. Render-only meshes and query-only motors are not
rigid query targets. No concurrent worker-query API. [Geometry limits](../PHYSICS_QUERIES.md).

All vectors/poses use local float simulation coordinates around a fixed double
absolute origin. No JS absolute-coordinate/rebasing API is exposed. No universal
world-up: supply directions/orientations from local state, support and gravity.

## Joint

`Joint.id` is a plain opaque string; `valid` returns false after generation retirement.
Other stale joint calls throw ReferenceError. `state` is detached:
`{active,enabled,coordinate,motorImpulse,type}`. `type`: 0 fixed, 1 hinge,
2 ball/socket, 3 slider. Active is solver participation, distinct from enabled.

| Method | Behaviour |
|---|---|
| `setEnabled(enabled)` | Boolean authored-runtime enable. |
| `setLimits(lower,upper,limits=true)` | Hinge radians [-π,π], slider metres; ordered finite values. |
| `setMotor(speed,maxForce,motor=true)` | Hinge rad/s + N·m, slider m/s + N; finite bounded motor. |
| `setSpring(rest,stiffness,damping,spring=true)` | Free-coordinate equilibrium and implicit spring/damping: slider rest in m, stiffness N/m, damping N·s/m; hinge rest in rad, stiffness N·m/rad, damping N·m·s/rad. |

All setters return true or throw on invalid settings/stale handle. Controls do not
create joints, replace types or edit anchors through JS. Author frames/anchors in
editor/scene: body-local frame X is hinge/slider axis; a world-side anchor/frame is
transformed through the joint owner's authored pose. Ball/socket has no angular
cone limiter. Motors do not imply a gameplay state. Use fixedUpdate for control.
[Joint geometry/solver semantics](../JOINTS.md).

## Collision and trigger callbacks

`onCollisionEnter/Stay/Exit(event)` and `onTriggerEnter/Stay/Exit(event)` receive
`{other,point,normal,relativeVelocity,normalImpulse}` at the fixed boundary.
Normal points from other toward the recipient. Relative velocity is OTHER minus
SELF point velocity (including angular contribution), observed when contact was
recorded, not a fabricated post-solve measurement. `normalImpulse` is a reported
solved normal impulse where available, otherwise null (especially sensors/exits).
Exit retains historical contact observations, not a new geometric query.

One event per body pair/phase, deterministic body-handle ordering, then recipient
A/B and authored slots. Sensors share normal geometry/M39 physical filtering but
produce no response; overlaps are discrete endpoint observations, not continuous
trigger trajectories. Disabled/destroyed recipients are rechecked; destroyed `other`
may still be an Entity wrapper with `valid=false`. Check validity before use.
Mapped ragdoll bodies have per-body events; owner scripts do not receive automatic
aggregate articulation events. Motors have no M42 events. JS decides game meaning.
