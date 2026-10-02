# Project Judas — Future Capability Roadmap

This document tracks **future engine capabilities**, not completed milestone history.

Current completed engine milestone: **M44 — Judas learns what you're pointing at**.

For current architecture and implemented capability, see `docs/ARCHITECTURE.md`.
For historical continuity and reasoning, see the local Project Persistent Memory.

Milestone numbers below are reserved only where explicitly stated.
 if you are an agent do not read past the current working milestone in this file.
---

## Near-term roadmap

### M45 — Judas learns how things are connected

General rigid-body joints and constraints.

Planned capability:

- fixed constraints;
- hinge joints;
- ball/socket joints;
- slider/prismatic joints;
- limits;
- springs where appropriate;
- motors where appropriate;
- editor/scene/prefab authoring;
- runtime and JavaScript control.

These are physics primitives, not doors, vehicles, machines or gameplay systems.

---

### M46–M49 — Unassigned

Deliberately open.

Do not invent milestone assignments merely to fill the numbers.

Choose capabilities according to what Judas actually needs after M44/M45.

---

### M50 — JudasJS learns that it needs proper documentation

Reserved documentation/tooling milestone for the public Judas JavaScript API.

Planned work includes:

- coherent JudasJS API reference;
- lifecycle documentation;
- modules and namespaces;
- safe handles;
- physics/query APIs;
- input;
- audio;
- particles;
- prefabs;
- classification;
- UI;
- collision/trigger events;
- scene/session APIs;
- examples and expected usage;
- maintained TypeScript declarations where useful.

M44–M49 should document new bindings sufficiently to use them, but should not turn into giant documentation projects. M50 is the deliberate consolidation milestone.

---

## Larger unassigned engine capabilities

| Capability | Current boundary | Future work |
|---|---|---|
| Live origin rebasing | M23/FTFT5 provide a fixed double absolute origin with precise local float simulation. | Continuous travel through a moving/rebasing local region covering physics, broadphase, contacts, attachments, fluids, rendering, interpolation and persistence. |
| Large-world streaming | M43 replaces/reloads registered scenes synchronously. | Sector/world streaming, background loading, lifecycle policy and potentially additive scene/world composition. |
| Skeletal animation | No general skeletal-animation system is currently claimed. | Skeletons, skinning, animation clips, playback and later blending/IK/physical animation as separately scoped work. |
| AI/navigation | Game behaviour can be written in JavaScript, but no general navigation system is claimed. | Navigation/pathfinding and reusable spatial/steering primitives where engine support is justified. |
| Networking | No general networking capability is currently claimed. | Transport, sessions, replication, authority and later prediction/reconciliation as separately scoped systems. |
| Deformable structures | Production collision bodies remain rigid. | Deformation/material models, structural failure and appropriate simulation budgets. |
| High-fidelity fluid/solid coupling | Production water is the accepted approximate system. P1-C/P1-C-M/P1-PF remain protected research. | Moving geometry and a complete validated high-fidelity liquid/solid integration if future requirements justify it. |
| Portals | M33 already provides authored secondary cameras and render-to-texture. | Portal-specific rendering/traversal, recursion policy and associated spatial semantics. |
| Unicode/text shaping | M41 runtime UI currently uses basic ASCII text rendering. | UTF-8/Unicode glyph handling, shaping, fallback fonts and localization primitives when required. |
| Profiling/diagnostics | Judas has subsystem measurements and debug tooling but no claim of a complete integrated profiler. | Unified CPU/GPU/subsystem profiling and resource diagnostics. |
| Rendering expansion | Current renderer provides the demonstrated OpenGL rendering, lighting, shadows, cameras and particles. | PBR/material expansion, LOD, batching/instancing, occlusion, decals, reflections and post-processing as independently justified features. |
| Audio expansion | M34 provides whole-clip WAV/MP3/FLAC playback and spatial audio. | Streaming/music, Doppler, occlusion, reverb and richer acoustic modelling if required. |
| Input expansion | M35 provides logical configurable keyboard/mouse and one active gamepad. | Multiple-controller assignment, local multiplayer and haptics when needed. |

---

## Standing roadmap rules

Judas is a **game engine, not a game**.

Demos prove engine capability; they do not define engine architecture.

**Judas provides engine primitives. JavaScript provides game behaviour.**

Do not create engine milestones for game-specific systems that existing primitives and JavaScript can already express.

Small conventional games must remain able to use Judas without paying for planetary, large-world or high-fidelity simulation features they do not need.

Approximation is acceptable when it is mechanistically credible, demonstrated sufficient and honestly bounded.

Future research-heavy systems should be undertaken because a real requirement demands them, not merely because a more complicated solution exists.
