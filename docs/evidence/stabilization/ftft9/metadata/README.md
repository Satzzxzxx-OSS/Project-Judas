# FTFT9 authored fluid metadata

These focused checks validate authored data, not hydrostatic or swimming behavior.
No protected historical evidence was rewritten.

- `run-0.log`: current production scene parser/writer/equality/fingerprint code,
  97 metadata checks, zero failures.
- `run-1.log`: expanded existing canonical-field suite, 140 checks, zero failures.
- `run-2.log`: independent Python `struct`/SHA-256 default-scene golden values.
- `run_metadata.json`: exact commands, elapsed times, HEAD and source hashes.
- `build-initial-api-error.log`: ordinary new-test API spelling error (`FindObject`
  instead of `Find`), corrected before execution; no physical failure inference.

Reproduce from the repository root (C++17 compiler, GLM headers, Python 3):

```sh
mkdir -p build/ftft9-metadata
c++ -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Isrc tests/FluidMetadataTests.cpp src/Scene.cpp src/SceneSerialization.cpp src/SceneFingerprint.cpp -o build/ftft9-metadata/fluid_metadata
c++ -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Isrc tests/SceneFingerprintTests.cpp src/Scene.cpp src/SceneSerialization.cpp src/SceneFingerprint.cpp -o build/ftft9-metadata/scene_fingerprint
build/ftft9-metadata/fluid_metadata
build/ftft9-metadata/scene_fingerprint
python3 docs/evidence/stabilization/ftft9/metadata/default_fingerprint.py
```

Scene grammar remains version **3**. Missing new fields read as their declared
values: fluid updates 30 Hz, hydrostatic drag 2/s, player density 950 kg/m³,
player fluid drag 2/s, swim acceleration 4 m/s², and no cavities. Writers always
emit the new fields, including defaults. A cavity is a body-local box with finite
center and strictly positive finite half extents; it adds metadata, not collision
surfaces. Classic scene cups declare their actual resolved interior bounds.

Canonical authored fingerprint schema is now **2**, adding all of these fields.
The independent golden script first reproduces the historical schema-1 default
hash, then constructs schema 2 with the added settings. The save format remains
2; existing production compatibility validation rejects earlier fingerprint
versions before mutation. This is deliberately strict compatibility, not a save
migration. `WorldStateTests` uses the current schema for malformed records and
adds an explicit prior-schema rejection; its whole-engine execution is recorded
by the encompassing FTFT9/final validation, not these direct source runs.

Editor controls expose cavity add/remove and bounds, player density/drag/swim,
and scene fluid update/drag settings with ordinary authored undo tracking.
Their compilation/integration belongs to the encompassing engine/editor build;
the direct commands above do not compile editor or dynamics code.
