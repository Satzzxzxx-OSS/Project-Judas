# M64 baseline, before dispatch extension

HEAD/main/origin/main b5676438ed12d9cb3d05c634eaf5f578d7216ada; clean.
Supplied audit/journal frozen at M60 a9c6cd7. Original bundle untouched. Raw
skate reproductions/project not supplied in this checkout: ramp comparison will
be an independently reconstructed current project, not a claim of rerunning Claude's game.

| Shape/pair | Rigid contacts | Rays/casts | Motor | Consumers |
|---|---|---|---|---|
| Sphere/box | existing precise primitives/manifolds | existing M44 | existing capsule distance | nav/audio/M42/stream/save |
| Old box compound | child boxes, no rotation | child primitives | child boxes | liquid includes primitive children |
| Radial terrain | sphere/box sampled surface | ray/sphere/capsule approximate, box rejected | 9 core samples | normal terrain/nav |
| Cooked convex / triangle mesh | absent | absent | absent | absent |
| Oriented/sphere/hull compound children | absent | absent | absent | absent |
| Collider metadata / closest point | no public snapshot/query | absent | n/a | E-04 gap remains |
| M62 surface geometry | particle/patch against sphere/box/terrain seam | deformable-local picking | no motor soft geometry | inherited approximation |
| M63 rigid fragments | ordinary box proxies | ordinary M44 boxes | ordinary boxes | no arbitrary hull generation |

Ownership: PhysicsWorld.cpp -> Narrowphase/Contacts -> ContactSolver/ImpactSolver;
M44 PhysicsCastGeometry, CharacterMotor/StepClimb; cooked resources will enter the
existing ResourceManager/AssetDatabase, not a second physics/asset pipeline.
M54/M55 cavity/liquid authority is preserved. Hull/open-mesh fluid loading must be
rejected unless an explicit existing supported interaction proxy is authored.

New QuickHull cooker is pinned Public Domain upstream (not a dynamics runtime).
