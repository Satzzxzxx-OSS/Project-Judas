# M34 focused audio evidence

No perceptual audio validation is asserted. The explicit device-free seam runs
miniaudio's normal voices, spatializer and DSP with deterministic sample advancement.
The connected test also uses EngineHost and a normal InteractivePlay frame with real
GL, then Stop/project closure/shutdown. No alternate simulation or audio mixer exists.

Focused command:

```
cmake --build build --target judas_audio_tests -j4
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_audio_tests
```

Whole gate: `python3 scripts/m34_validation.py`. It creates a genuinely fresh Release
build and reuses the existing production/editor/runtime/async gate once. Historical
FTFT fixture outputs are redirected here; their assertions are unchanged. The script
refuses to overwrite an existing final run. Protected evidence is never regenerated.

Compiler/test iteration logs are retained. The early rename failure came from a test
passing an unjoined asset directory into AssetDatabase::Scan, not a production asset
move defect; corrected to the API's actual absolute-directory contract.

Original demo audio is under `assets/audio/` with stable metadata and CC0 provenance.
Offline tone creation/conversion is asset authoring, not an engine synthesis feature.
Build products and generated asset-test project data remain ignored build/cache data.
