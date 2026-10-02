# M38 candidate evidence

Focused checks: `cmake --build build --target judas judas_export_tests judas_package_application_tests judas_editor -j 4`;
`./build/judas_export_tests`; `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_package_application_tests --output build/m38-application`.

Candidate gate: `python3 scripts/m38_validation.py` (refuses to overwrite evidence).
It performs one clean Release build, existing production/async/editor/runtime
validation, editor Export request + Play/Stop, and a moved actual Release runtime
startup from `/`. The application probe runs the normal async Application loop
with real decoders/GL/audio voices; the separate copied-runtime screenshot uses
the existing blocking script harness and is labelled as such. No perceptual audio
or human visual acceptance is claimed. Binary packages/builds remain ignored;
logs, source fingerprints and results are persistent here. Initial ordinary test
build errors are retained under development, separate from passing evidence.

Representative project: `projects/export_demo/export_demo.judasproj`, composed
from existing M33/M34/M36/M37 authored content, with normal IDs/components. No
special gameplay path. It includes the shipped input map and all its registered
runtime resources. Human launch checklist and platform requirements: docs/M38.md.

Completed results: RESULTS.md and final/SIGNOFF.json. A narrowly corrected editor
Export automation hook was verified with `python3 scripts/m38_validation.py
--export-only` after rebuilding only `judas_editor`. Original failed logs remain;
this option requires the already-passing production result and does not repeat
full validation. Final source fingerprints include that correction.
