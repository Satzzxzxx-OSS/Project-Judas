# M46 candidate — executed results

M45 checkpoint: `9254d2e16154e268b0018f156b7c2ed035acaf43`, pushed to main;
HEAD/main/origin/main matched with a clean tree before M46 began.
M46 remains uncommitted. Human visual acceptance is pending.

| Check | Result |
|---|---|
| Clean Release build | PASS; zero compiler warnings |
| Existing production suites | 85/85 PASS |
| Real async application integration | 12/12 cases; 246 checks, 0 failures |
| M46 import/pose/actual GL/resource/lifecycle | 34 checks, 0 failures |
| M46 actual Application/JS/reload/pause | 4 checks, 0 failures |
| Existing editor Play/Stop | PASS |
| Animation demo editor Play/Stop | PASS; authored scene identical |
| Animation demo standalone | PASS |
| Moved M38 package, unrelated working directory | PASS; two clips, seek/pause and prefab verified by actual JS |
| Source unchanged during final production run | PASS |
| Final source/demo/dependency fingerprints | 49 files match |
| Protected FTFT/P1/M33–M45 evidence, prototypes, ROADMAP | unchanged |

One new clean build directory was used. Its first attempt found missing
`SkeletalAnimation.cpp` linkage in three legacy test targets that compile
Renderer directly. The source-list correction continued that same clean build;
production tests ran **once**, after successful compilation. The failed build
logs/results remain in `initial_clean_build_failure/`. Early demo-format and
test-wiring failures are preserved in `focused/` and described in README.

## Lightweight performance

The original demo has 3 skin joints / 4 total nodes, 144 vertices, two clips.
- Clip sample + hierarchy/palette resolution: **0.388018 microseconds** per evaluation (10,000 evaluations).
- Eight shared-asset skinned instances, 320×240 actual software-GL context:
  **0.396687 ms** mean frame throughput over 20 frames, including end GPU finish.
- Export: 0.018 s; package 7,220,039 bytes.

These are small-fixture sanity measurements, not universal rendering guarantees.
The final continuation/gate took 568.6 s, including remaining clean
build, production suites and smoke. The initial failed build has its own timings.

## Accepted candidate scope / limits

GPU linear-blend skinning, one clip producer per instance, STEP/LINEAR/CUBICSPLINE,
independent poses, arbitrary authored orientation, ordinary scene/prefab/editor/
async-resource/export integration. No collision/physics bone mapping. Import is
self-contained GLB or embedded-buffer glTF, one skinned mesh/skin, four influences,
48 skin joints / 128 nodes; no external buffers, PBR material import, morphs,
compression, blending, IK, root-motion gameplay or ragdolls.

Human checklist and controls: `docs/ANIMATION.md`.
Exact commands/raw results: `final/RESULTS.json`, `final/production/results.json`.
Source fingerprints: `final/SOURCE_SHA256.json`.
