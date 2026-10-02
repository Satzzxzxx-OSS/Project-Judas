# M46 candidate evidence

Starting baseline is the accepted and pushed M45 checkpoint:
`9254d2e16154e268b0018f156b7c2ed035acaf43`.
M45 was explicitly staged with all 38 recorded source hashes matching its
accepted capture follow-up; main/origin/main were aligned and clean before M46.

Candidate capture command: `python3 scripts/m46_validation.py` from the repository root.
The capture runner deliberately refuses to overwrite existing evidence/archive
folders; repeat captures require fresh output/archive paths. Individual focused
checks can be rerun with their executables and a fresh `--output` directory.
This performs one genuinely clean Release build, the existing production-suite
runner (including focused M46 executables), real async integration and editor
smoke, then the animation demo editor/standalone and moved M38 package smoke.
Existing output adapters relocate generated fixtures to M46 evidence so old
reports remain unchanged. The already-reviewed shared-source exceptions in
historical FTFT protection do not weaken its behavioural assertions.

- `final/RESULTS.json`: actual run results and commands.
- `final/SOURCE_SHA256.json`: exact candidate source/demo/dependency fingerprints.
- `final/fixture-details/judas_skeletal_animation_tests`: CPU analytic checks,
  actual GL rest/deformation images, normal async worker/context ownership,
  runtime/prefab lifetime and small performance measurement.
- `final/fixture-details/judas_animation_application_tests`: copied demo source
  with assertions executed through normal Application and actual JavaScript;
  playback, stale handles, prefab spawn, reload and authored pause UI.
- `initial_clean_build_failure/`: the first clean-build attempt found a missing
  pose-matrix link dependency in three legacy Renderer test targets. Their source
  lists were corrected and the same clean build completed before the one
  production-suite run.
- `focused/`: development checks and preserved demo-format failures. These
  ordinary authoring/test-wiring failures were corrected, not claimed passes.
  The initial probe dereferenced a missing demo entity after reporting invalid
  scene data; its scene-load check now exits cleanly. No production physics
  contradiction or algorithm substitution was involved.

Pixel/state assertions and an inspected screenshot are automated evidence.
Human visual acceptance is **pending**, with the checklist in `docs/ANIMATION.md`.
Whole-frame GL timings here use a hidden actual context/software rendering;
they are a sanity check, not a hardware/universal performance guarantee.
No fluid prototypes or historical evidence are modified or integrated.
