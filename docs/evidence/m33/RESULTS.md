# M33 executed results

Candidate implementation; operator visual acceptance is pending. No commit/push/tag.
Baseline: `f1fc10d2d3b8bc55062abac62604033aecc3a81a`.

## Verification

- One genuinely clean Release configure/build: PASS, zero compiler warnings.
- One complete existing production gate: 61/62 suites initially passed. The only failure was the old literal fingerprint-schema-2 expectation in FluidMetadataTests. New authored camera coverage deliberately versions the schema to 3.
- Corrected that exact version assertion/labels, without changing fixtures/tolerances: 97/97 metadata checks pass. The independent schema-3 golden calculation and scene fingerprint suite pass (140 checks).
- Final focused actual-GL camera suite: **51/51** checks pass; same EngineHost, Renderer, RuntimeWorld and InteractivePlay paths. Includes resource deletion, actual pixels, camera/body motion, two views, ordinary material sampling, self-feedback, framebuffer/viewport restoration, cadence, undo and Play/Stop.
- Final lifecycle-reference correction: a consumer may retain a stable reference to a destroyed authored camera, resolving white fallback. Mixed destroy/create delta passes. Missing definitions still reject. Affected persistence suite **307/307** passes.
- Actual async application: 12 cases, 246 assertions, zero failures. Existing editor Play/Stop and standalone startup pass. Standalone/scripted reference is labelled blocking, not async evidence.
- Classic/terrain near/far: four complete 900-step walks; equivalent near/far physical payloads match.
- All other existing production suites passed during the full gate. No second full matrix was run for the disclosed affected-only corrections.

Raw full-run exit 1 is preserved in `final/`; it is not rewritten as a historical pass. `ACCEPTANCE.json` gives exact source changes after that run and corrected-check provenance. Only RuntimeWorld camera-reference validation and the two affected test files changed afterward. Focused binaries were rebuilt for those changes.

## Performance evidence

40 frames per configuration, same small scene. Clean-gate headless software GL, synchronized CPU/render wall time including first allocation (not GPU timestamp measurements):

| Active secondary cadence | Median ms | Mean ms | First frame ms | Draw calls / 40 frames | Target passes |
|---|---:|---:|---:|---:|---:|
| Disabled | 0.439074 | 0.496523 | 0.522205 | 240 | 0 |
| Every frame | 0.756969 | 0.844341 | 0.792876 | 360 | 40 |
| Every 2 frames | 0.678571 | 0.633340 | 0.777428 | 300 | 20 |
| Every 4 frames | 0.443578 | 0.566021 | 0.834479 | 270 | 10 |

Baseline submits 6 draws/frame, each secondary update adds 3. Cadence reduces mean cost and submissions; median alone is misleading when fewer than half of frames update. These are modest-scene observations, not universal performance guarantees. Final lifecycle run also records timings in its raw log.

## Scope and limitations

Targets allocate lazily in presentation; disabled cameras perform no passes. Renderer owns/deletes framebuffer, colour and depth. Worlds own camera runtime records and release targets before renderer/context teardown. Same world is rendered without extra simulation ticks. Camera GPU images/counters are not persisted.

Flat authored pass order, last completed cross-camera images, white self-feedback fallback; no recursion/dependency scheduler/portals. Shadows share main-view coverage. Normal lit materials tint the target (not an emissive display model). Consumers are box/sphere/imported mesh, not compound/terrain. Edit-mode preview is fallback; runtime/Play renders live targets. Resolution is 1–4096 and GL device limits also apply. Failed creation is reported; no silent asset substitution. Old schema-2 world saves are intentionally rejected by strict FTFT1 policy; authored format-3 scenes remain readable.

## Evidence and reproduction

- `scripts/m33_validation.py`: one-shot clean full gate, reusing existing scripts/assertions, with Release/output adapters only. Its existing-output guard prevents accidental reruns over evidence.
- Focused: `cmake --build build --target judas_render_camera_tests -j4`, then `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_render_camera_tests --output <fresh-output-directory>`.
- `final/`, `metadata-correction/`, `lifecycle-correction/`: raw executed commands/results.
- `source/` and `SHA256.json`: final source snapshots/fingerprints.
- Initial focused compile/GL-loader wiring errors remain in `focused/`; corrected by using normal EngineHost startup, not a alternate renderer.
- `CHANGED_FILES.md`: exact file inventory.

Protected FTFT1–9 evidence and P1-C/P1-C-M/P1-PF prototypes are unchanged. No physics/fluid/simulation mechanisms changed.

## Operator visual checks

Open `assets/scenes/camera_surfaces.judas` in editor and Play, or launch standalone. Confirm the red subject moves in the display, main camera stays independent, changing the camera pose changes the image, cadence/disable behaves visibly, and Stop restores authoring. Confirm the safe white feedback patch when a camera sees its own display. Automated pixels/screenshots are not human visual acceptance.

## Operator demo revision after initial report

The initial presentation was visually rejected. `demo-revision/` preserves the
revised authored scene and actual interactive screenshot. This exposed an existing
box UV pattern mismatch hidden by white textures: screen triangles tore the image.
Renderer now maps UVs consistently from each face's local vertex positions. The
rebuilt actual-GL suite passes **52/52**, adding a 16-sample quadrant image oracle.
No additional full suite was run for this localized fix. The earlier full-gate
results remain historical evidence rather than being rewritten.

Latest source overlay fingerprints/snapshots are in
`demo-revision/SOURCE_SHA256.json` and `demo-revision/source/`. Human visual
acceptance is still pending. No commits, pushes or tags.
