# Entities, transforms, tags and prefabs

[Index](../JUDASJS.md) · [Physics](physics.md) · [Safe lifetime](scenes-state.md)

## Entity

`import {entity, world, Entity} from 'judas'`. `entity(id)` and `world.entity(id)`
construct wrappers, **not verified lookups**. Falsy IDs and string `"0"` give null;
any other supplied ID produces a wrapper. Prefer decimal strings: runtime IDs
can exceed safe JS integers. `new Entity(id)` also exists but does not test validity.
`id` is a plain writable JS field; treat it as opaque/immutable by convention.

| Member | Arguments / result / behaviour |
|---|---|
| `id`, `valid` | String identity; validity resolves the current world's definition and returns false after destruction. |
| `transform` | Read detached `{position,rotation,scale}`; assign a partial object to update selected fields. World transforms in fixed-origin local simulation coordinates. |
| `parent`, `children` | Safe wrapper/null; immediate authored/runtime children, ascending entity ID. |
| `destroy()` | Destroy hierarchy, true or throws on failure. Stale owner throws, not an idempotent no-op. |
| `setColliderEnabled(bool)` | Enable/disable existing body; boolean success, false when absent. |
| `hasTag(name)`, `addTag(name)`, `removeTag(name)` | Registered project tag; boolean result, unknown name throws. |
| `classification` | Detached `{renderLayer,collisionLayer,collisionMask}`. Layer IDs are numeric, 64-bit mask is a decimal string. No JS classification setters exist. |
| `scriptState(slot)` | Detached bounded JSON copy of another slot's state; null when absent/faulted. Invalid slot/state throws. |
| `velocity`, `angularVelocity` | Read/write dynamic-body world-space m/s and rad/s; missing/static body throws. Character velocity is instead on `.character`. |
| `applyForce(v)`, `applyImpulse(v)`, `applyTorque(v)` | N, N·s, N·m; world-space, existing dynamic body required. Return undefined. No application-point JS overload. |
| `audio`, `animation`, `ragdoll`, `character`, `camera` | Component facades/snapshots or null when component absent; see subsystem pages. Stale entity usually throws; `.character` specifically returns null. |
| `playAudio/stopAudio/pauseAudio/resumeAudio`, `setAudioEnabled`, `burst`, `setParticles`, `setCameraEnabled` | See [effects/cameras](effects-camera.md). |

Returned transforms/vectors are snapshots. `e.transform.position.x += 1` only
edits the temporary object. Assign it back, e.g. `const t=e.transform;
t.position.x+=1; e.transform=t`. Quaternion components are `{x,y,z,w}`; finite
nonzero rotations normalize on write. Supplied position/scale must have all xyz
components; scale is positive. Pose writes teleport, not collision-aware movement.
Visual scale does not resize authored collider dimensions. Parent-local authoring
is resolved before these world-space reads; scripts do not set a parent through this API.
Disabling a collider does not invalidate its entity/body identity: these dynamic-body
operations remain callable while disabled, although physics participation stops.

## world

| Member | Arguments / result |
|---|---|
| `entity(id)` | Alias above. |
| `queryTags(required=[], excluded=[])` | All required, no excluded project tags; sorted live Entity wrappers. Unknown tags throw. |
| `spawnPrefab(assetID, transform)` | Partial placement defaults to identity; normal hierarchy/components, independent runtime IDs. Returns root Entity or throws; no half-valid success. |
| `overlap(min,max,filter={})`, `sweepCapsule(...)` | Read-only existing queries; [physics reference](physics.md). |
| `viewRay`, `setView`, `clearView`, `fluidSample` | [Camera/liquid reference](effects-camera.md). |

Use stable asset IDs, not filenames, for prefab/resource APIs. Runtime prefab
support is the existing ordinary component subset, not every imaginable engine
component. Nested prefab variants are not introduced here. Source data is not
modified by runtime edits. See [prefab architecture](../M36.md).

Tags are semantic labels (many per entity); collision layers/masks are physical
policy; render layers/camera masks are independent presentation policy. Registries
and default relations are project authoring, not JS-created runtime namespaces.
JS currently reads classification and changes tags; it cannot rename registries,
set render masks or replace collider layers/masks on ordinary bodies. Character
query configuration is an explicitly exposed exception, not a global setter API.
An entity without a rigid body reports collision layer 0/mask "0"; this does not
create a collider or describe its CharacterMotor query settings.
