# Ordinary application development follow-ups

The first application parse failure (missing required normal scene setting) and
second (wrong component air-velocity property key) remain in their original logs.
The first material integration parse failed because the generic serialized slot
payload was not enclosed in the scene format's quoted string; the generator now
uses the normal encoding. No scene parser was weakened.

The first actual M61 cold save failed with `saved clip unavailable`: the content
selected `Sway`, but the included original model exposes `Wave` and `Stretch`.
The lab now selects `Wave`. The save contract correctly rejected unavailable
animation data; no persistence or animation validation was bypassed.

Early input injection used physical edge state before the application began its
frame. Tests now push normal SDL events, so the application itself samples the
logical actions. Prefab outcome now requires a real runtime-owned deformable,
not an authored entity count. The project uses the documented root Entity return
from `world.spawnPrefab`, not an imagined array return.


The first region revisit failed `stale entity/no deformable`: normal public entity
lookup intentionally hides privately staged region owners. Deformable restoration
now uses an internal staged-owner path before publication; ordinary motor/script
state restoration remains after publication as before. Failed preflight hides and
tears down the region rather than advertising partial success. The follow-up
proves exact node restoration with a fresh identity, and live external pins.

A public-handle test initially forgot to register its imported relative module.
The ordinary module loader correctly rejected it. The fixture now tracks both
files through AssetDatabase; no module-loading rule changed. It proves disabled
force rejection, fixed-phase enforcement, body-to-world binding replacement,
reset, prefab ownership, and stale ReferenceError/valid=false after destruction.

Inspection found the editor replacement helper would keep old bindings whose
groups did not exist in imported topology.
The generic helper now removes those bindings and reports that authoring change.
The real editor import/Play-Stop check passes. Invalid imported source bakes still
require rebaking; the source parser/persistence checks were not weakened.
