# Lifecycle, modules and script properties

[Index](../JUDASJS.md) · [Lifetime/state](scenes-state.md) · [Practices](practices.md)

## Module model

Registered `.js` assets are ES modules executed by QuickJS-NG v0.17.0. Import
`'judas'` for engine primitives. Relative `./` and `../` imports must resolve to
registered script assets. Other bare imports, absolute paths, Node and browser
modules are unavailable. Source modules are limited to 1 MiB. Modules are cached
once per world's VM; module globals are shared between instances in that world.
A new scene/reload/Play creates a fresh VM, not a retained module heap.

Default-export a constructible class. Each enabled authored slot creates its own
instance with `{entity, properties}`. Slot IDs are stable identities, distinct
from order. The constructor runs before restored `state` is installed. Do not
make constructor side effects depend on saved state.

```js
export const properties = {
  rate: {type: 'number', default: 2},
  active: {type: 'boolean', default: true},
  label: {type: 'string', default: 'Counter'}
};
export default class {
  constructor({entity, properties}) {
    this.entity = entity;
    this.props = properties;
    this.state = {count: 0};
  }
  start(dt) {} // restored state is already available here
  fixedUpdate(dt) { if (this.props.active) this.state.count += this.props.rate * dt; }
}
```

`properties` is an exported plain schema; only `number`, `boolean`, `string` are
supported. Supply matching defaults: a default is required whenever the authored
values omit that field. Authored values may override declared fields;
unknown fields/type mismatches fault the slot. The inspector edits those values,
not JS source. Asset references may be string properties. There is no exported
`Behaviour`, decorators, inspector entity-reference type or component scripting DSL.

Inspector metadata evaluation runs top-level module code in a no-world VM. Keep
top-level code declarative; importing `judas` is fine, calling its world API there
fails (logging is allowed). Restart Play after code edits; no hot reload.

## Callbacks and order

All callbacks are optional and synchronous. Return normally; do not return a
Promise. No async callback/top-level-await scheduler exists.

| Callback | Actual boundary and argument |
|---|---|
| `start(dt)` | Once before the first UI/frame/fixed callback that reaches this slot; not guaranteed to begin in fixed mode. |
| `uiUpdate(dt)` | Each interactive outer frame, including while a modal UI pauses gameplay. |
| `onUI(event)` | UI input events, before gameplay input; broadcast to live scripts. |
| `update(dt)` | Active gameplay frame, before that frame's fixed-step catch-up. |
| `fixedUpdate(dt)` | Before ordinary fixed-step force/physics advance; zero or several calls per rendered frame. |
| `onCollisionEnter/Stay/Exit(event)` | Fixed-step authoritative contact delivery after physics/motor/pose publication. |
| `onTriggerEnter/Stay/Exit(event)` | Same event boundary; sensors generate no physical response. |
| `destroy(dt)` | Slot removal/disable or ending the VM; last callback delta is passed, not a teardown timestep. Faulted instances skip this callback. |

Order within normal callback phases is ascending entity ID, then authored slot
vector order. `start` precedes the first phase reaching an instance, not a global
start-all-before-any-update barrier. Callback lists are snapshots: destroying an
entity invalidates its wrappers immediately; synchronization later retires its
slot and calls `destroy` if not faulted. Spawned script instances enter at the next
synchronization boundary, which can be another fixed step in the same outer frame.
Do not promise one-render-frame delayed spawning.

UI runs before gameplay, consumes logical actions sharing physical bindings, and
visible enabled modal documents pause gameplay/fixed accumulation. The frame
closing a modal is also withheld from gameplay. Physics steps call `fixedUpdate`,
advance the normal world once, then publish motors/poses and dispatch contacts.
Transitions are requested during callbacks and applied only after the outer
application frame returns. No world teardown occurs inside `scenes.load()`.

`time.fixed` is true in fixed callbacks and contact delivery; false in frame/UI
callbacks. Character velocity/acceleration setters enforce fixed mode (contact
callbacks technically qualify too). Prefer submitting intent in `fixedUpdate`;
contact intent applies after the current motor step. Forces are not phase-guarded,
but use fixed callbacks to avoid frame-rate-dependent repeated force accumulation.
Read relative mouse delta in frame updates, not once per catch-up step.

## Failures and budget

First module/constructor/callback failure records entity/slot/asset/phase/stack
and faults that instance until its VM is restarted. Other slots continue. A
faulted slot is omitted from state capture and does not receive `destroy`.
Module globals are still shared; error isolation is not independent VMs per slot.

The VM has 64 MiB managed memory, 512 KiB stack and a default 10,000 interrupt-poll
budget per invocation. This bounds runaway JS, not wall time or indivisible native
work. Standard synchronous language facilities exist, but no Node filesystem,
DOM, timers, networking, npm, promise pumping or JS worker API is supplied.
Scripts are trusted project code, not a security sandbox. `__judas` is an internal
native dispatch function; `globalThis.console` aliases the public logging object.
