# JudasJS — current API reference (M55)

JUDAS PROVIDES ENGINE PRIMITIVES. JAVASCRIPT PROVIDES GAME BEHAVIOUR.

This reference describes the public virtual `judas` module through M55, based on
M54 checkpoint `d9ebc8c7a987047b1d4175ed5d5de7da8dea472d`. It is not an
eternal compatibility/semantic-version promise. Source authority is
`src/ScriptSystem.cpp` and the runtime systems it calls; demos do not define API.
M50 introduced the documentation/tooling. M51 adds generic mass/inertia snapshots
and pointer capture intent; see [engine/game boundary](M51_ENGINE_BOUNDARY.md).
M52 exposes the existing generic impulse-at-point body operation and render pose
through `Entity.presentedTransform` / `presentationUpdate(dt, alpha)`; the
[Spring Range project](M52_SHOOTER_GAME.md) implements all shooting/score rules in JS.

## Find an API

| Topic | Reference |
|---|---|
| Imports, properties, start/update/fixedUpdate/presentationUpdate/uiUpdate/destroy | [Lifecycle](judasjs/lifecycle.md) |
| Entity, transform, tags, spawnPrefab | [Entities/prefabs](judasjs/entities.md) |
| input, time, console | [Input/time](judasjs/input.md) |
| physics.raycast/sphereCast/capsuleCast/boxCast, Joint, contacts/triggers | [Physics](judasjs/physics.md) |
| Audio, particles, camera, world.fluidSample | [Effects/view](judasjs/effects-camera.md) |
| ui, UIDocument, UIElement, onUI | [Runtime UI](judasjs/ui.md) |
| scenes, session, script state, safe handles | [Lifetime/state](judasjs/scenes-state.md) |
| Animation.crossFade/layers, Ragdoll | [Animation/ragdolls](judasjs/animation-ragdolls.md) |
| entity.character / CharacterMotor | [Character](judasjs/character.md) |
| navigation / NavigationAgent | [Navigation](judasjs/navigation.md) |
| liquid / LiquidVolume / conserved reservoirs | [Liquid](judasjs/liquid.md) |
| Executed scripts | [Cookbook](judasjs/cookbook.md) |
| JSDoc, editor setup, practical conventions | [Practices](judasjs/practices.md) |

## Tooling and completeness

- [judas.d.ts](judas.d.ts): offline completion/type information for JS editors;
  no TypeScript runtime dependency.
- [API inventory](judasjs/API_INVENTORY.md) and [machine manifest](judasjs/api-inventory.json):
  actual bindings → declarations → reference, native bridge operations and explicit
  internal/dynamic verification exceptions.
- `scripts/check_judasjs_api.mjs`: source AST, declaration AST, inventory/reference
  coverage, standard TypeScript checking and actual-VM enumeration comparison.
- Existing milestone records remain architecture/history; start here for CURRENT
  JS usage. No raw C++-only system is implied to be scriptable.

## Important boundaries

Use fixedUpdate for authoritative movement/physics, frame updates for relative
mouse input, presentationUpdate for render-following poses/cameras, and uiUpdate
for menus while paused. Worlds have fixed local
float coordinates with a double absolute origin; no automatic live rebasing or
universal world up. Tags, collision layers and render layers are distinct.
Read returned snapshots; assign updates through explicit APIs. Safe wrappers do
not extend native lifetime and cannot cross scene replacement.

Scripts are trusted QuickJS code, not Node/browser/npm or a malicious-code sandbox.
No arbitrary async callbacks, debugger/hot reload, raw bones, ECS, runtime layer
registry editing or JS disk-save API. Current subsystem limits are documented
on their pages rather than silently upgraded. Human hardware/visual/audio review
remains authoritative.

## Human developer review

1. Confirm this landing page and subsystem navigation are obvious.
2. Search raycast, Joint, Animation, Ragdoll and CharacterMotor.
3. Compare signatures/results with familiar runtime behaviour.
4. Open judas.d.ts in a TS-aware JS editor and inspect completion.
5. Copy/attach/run one cookbook script with its listed setup.
6. Check lifecycle/fixedUpdate explanations are understandable.
7. Check safe handles and scene/session lifetime rules are clear.
8. Check tags, collision layers and render layers remain distinct.
9. Check arbitrary-gravity coordinates do not assume world-Y.
10. Confirm engine primitives and game behaviour remain separate.

- [Navigation: queries, agents, obstacles and explicit links](judasjs/navigation.md)
