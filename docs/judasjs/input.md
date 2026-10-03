# Input, time and logging

[Index](../JUDASJS.md) · [Lifecycle](lifecycle.md) · [Character](character.md)

## input

`import {input} from 'judas'`. Names refer to the current project's M35 map.
`held(name)`, `pressed(name)`, `released(name)` return booleans; `axis(name)`
returns a number. Unknown names/no attached input return false/zero. There are no
raw key, mouse, controller, binding-edit or consumption APIs exposed to JS.
Project/editor/C++ own binding authoring; scripts consume the logical surface.

Frame action edges live for one logical frame. Fixed callbacks use a latched
snapshot: a short press across zero-step frames survives to the next fixed step,
then does not repeat in later catch-up steps. Read an edge in one behavioural
phase; reading it in update AND fixedUpdate can intentionally observe it twice.
The engine does not consume it just because one script read it.

Axes retain source semantics: keyboard/stick axes generally normalize/clamp to
[-1,1], sticks use configured deadzones; mouse/wheel axes are relative frame
deltas. Fixed callbacks read the current axis snapshot, not redistributed mouse
motion. Apply mouse look once in `update`/`uiUpdate`, using stick rate × dt where
appropriate. UI can consume a logical action and all actions/axes sharing its
physical bindings; consumed reads are false/zero for that frame's fixed steps.
One active logical gamepad is supported; hardware feel remains operator-tested.

## time

`time.elapsed`: authoritative simulation seconds (fixed clock, pauses with world).
`time.delta`: current callback's delta, seconds. `time.fixed`: current phase flag.
Neither is wall clock or an interpolation timestamp. Teardown retains the last
callback timing; do not integrate motion in `destroy`. Frame catch-up is bounded;
this is not a guaranteed replay/network determinism protocol.

## console

`console.log(...values)` converts each with JS String, joins with spaces and
writes `JS: ...` to application stdout. It returns undefined. Objects generally
print `[object Object]`; use `JSON.stringify` for structured output. No warn/error,
log levels, file sink or developer debugger API. The virtual module also sets
`globalThis.console` to this object.

## input.pointerCapture (M51)

Boolean get/set request for relative pointer capture in this runtime world.
Modal UI temporarily releases actual capture; the getter reports script intent,
not device state. The request resets at world destruction/authored reset.
New projects start uncaptured until a script requests capture. Historical projects
with `legacy-gameplay "true"` retain their compatibility capture policy.
