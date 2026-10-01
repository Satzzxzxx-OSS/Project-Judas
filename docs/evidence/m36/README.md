# M36 reproducible checks

From repository root:

- Focused: `cmake --build build --target judas_prefab_tests judas_prefab_application_tests -j 4`
- `./build/judas_prefab_tests`
- `SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_prefab_application_tests`
- Full clean Release + unchanged production/editor/runtime gate: `python3 scripts/m36_validation.py` (refuses to overwrite its output).
- Visible standalone demo: `./build/judas assets/scenes/prefab_demo.judas`
- Authoring: open that scene in `./build/judas_editor`.

`demo_writer.cpp` reproduces the small authored demo through engine APIs, using the fixed demo AssetId. It is not an alternate simulation implementation. Binaries live only in ignored build/cache output.

`focused.log` and `application.log` are development focused results. The final production directory contains the exact clean-build candidate's runs, including both M36 test targets. Protected historical evidence/prototypes are never overwritten. Human visual/prefab acceptance remains pending.

Read `RESULTS.md` / `RESULTS.json` for the aggregate result and the distinction between the initial full run and repaired focused reruns. `SOURCE_SHA256.json` fingerprints the final candidate.
