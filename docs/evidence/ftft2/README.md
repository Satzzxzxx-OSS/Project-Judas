# FTFT2 — real asynchronous application loading

## Outcome and reproduced defect

Implemented for review; no commit, push or tag. Baseline: clean `main` at
`23f5447d37fb96ab4c42017d33b5de1fccae997b`. FTFT3 was not started.

`ResourceManager::ReleaseRef` cancelled Queued/Loading tasks but omitted
CpuReady. A final consumer could release already decoded data awaiting an
upload slot, then the next ordinary application frame would create an unused
GPU resource. The pre-fix real-GL witness failed **two assertions** and created
six GPU objects; with the fix it creates five and the entry stays Cancelled.
See `pre-fix/run.log`, the operation trace, result metadata, source hashes and
preserved source snapshots. `ReleaseRef` now uses the existing cancellation,
task identity and generation-discard mechanism for CpuReady too. Remaining
consumers still protect the task. There is no scheduler or streaming rewrite.

Two existing job-test defects were also repaired: a tautological submitted-job
comparison and cross-thread unlocking of a `std::mutex` (undefined behavior).
The latter now uses a condition variable and observes actual queue cancellation.

## Reproduce

From the repository root:

```sh
python3 scripts/ftft2_validation.py
```

Dependencies: C++17 compiler, CMake, SDL2/GLM development packages, Python 3,
and working OpenGL 3.3. This run used SDL `offscreen`, Mesa software rendering,
and seven hardware-derived workers. The late-old-completion-after-new-ready
case explicitly requires at least two workers; it fails clearly if unavailable.
The runner builds all production targets, executes the actual async application
suite, all 35 other production suites, the existing editor autotest, a labelled
blocking runtime reference and the M31 stress tool. Fixtures/build products go
under ignored `build/`; source, logs, traces and hashes stay in this evidence tree.
`final/results.json` records exact commands, environments, exits, timing, binary
hashes and source fingerprints before/after. Watchdogs detect hangs only; elapsed
time is not an acceptance threshold. No arbitrary sleeps exercise race windows.

## Actual production path and observation boundary

- Application parses the project/authored scene, initializes EngineHost (window,
  GL, Renderer/font, JobSystem, ResourceManager), scans project assets and builds
  RuntimeWorld. Authored resource demand uses the real ResourceManager.
- `JUDAS_TEST_SCRIPT` forces blocking mode and exits through RunTestHarness before
  the normal loop. That path remains unchanged and is labelled blocking evidence.
- The new executable compiles the same `Application.cpp` as `judas`, uses its
  normal `PollEvents -> PumpResources -> InteractivePlay::Frame -> SwapBuffers`
  loop, and asserts blocking mode is off with real workers and a real Renderer.
- Optional callbacks inject SDL events, observe frames before buffer swap and
  supply a deterministic frame clock. They do not replace simulation, rendering,
  pumping, IO, decoding or uploads. Real OBJ/PNG files are read and decoded.
- Read gates occur after actual fread chunks; decoder gates occur after the
  indivisible real decoder returns. CPU-ready windows use the existing Pump(0)
  upload budget. Setup barriers deliberately arrange states; the progress case
  holds real IO while the ordinary application loop remains free to run.
- Renderer readback inspects actual GL buffers/pixels, not CPU mirrors or counters.
  Actual decode/create/destroy boundaries record thread IDs and context status.
- Pending project replacement exercises EngineHost::OpenProjectAssets and real
  RuntimeWorld/session teardown/build inside Application's loop. ReleaseAll
  **synchronously** cancels, waits and discards before the database rescan.
  This is safe handoff evidence, not a claim of non-blocking project closure.
- Shutdown cancels resources, joins workers, destroys renderer resources, then
  destroys the context. The queued/read cancellation gate waits for both cancel
  operations so a newly freed worker cannot race into the queued request.

## Executed integrated cases

| Case | Result | Seconds | Observable acceptance |
|---|---|---:|---|
| shared-progress | PASS | 0.0966 | Two authored consumers coalesce; temporary third consumer release preserves load. SDL event, four real fixed steps/body displacement and rendered pixels while a file read is gated. |
| queued-cancel | PASS | 0.0682 | All worker slots occupied deterministically; last demand cancels queued load before IO. |
| read-cancel | PASS | 0.0697 | Cancel after one actual 1 MiB fread chunk; no next chunk or decoder entry. |
| cpu-ready-cancel | PASS | 0.0674 | Real decode, Pump(0), last demand release; later ordinary Pump cannot upload or resurrect. |
| stale-generation | PASS | 0.0665 | Hold old decode completion, invalidate/re-request changed bytes, verify newer GPU vertices, then release and reject old completion. |
| project-switch | PASS | 0.0653 | Old decode pending at EngineHost project replacement; synchronous drain; same AssetId in new project resolves to different GPU vertex contents. |
| project-close | PASS | 0.0653 | Old decode pending at project closure; discard survives subsequent ordinary Pump. |
| missing-recovery | PASS | 0.0676 | Actual missing-file error; unrelated load succeeds; repaired file invalidated/retried and read back from GPU. |
| corrupt-recovery | PASS | 0.0692 | Actual OBJ and PNG decoder failures; repair/invalidate/retry; unrelated GPU mesh remains correct. |
| residency | PASS | 0.0737 | LRU unused GPU deletion; active consumers retained at 216 bytes against zero budget; evicted asset loads again into new GPU buffers. |
| upload-budget | PASS | 0.0697 | Three real completed decodes; ordinary Pump creates two GPU resources, leaves one CpuReady, uploads it next frame. |
| shutdown-outstanding | PASS | 0.0661 | Actual application teardown with queued IO, read in progress and CpuReady data; both cancellations issued before releasing blocked worker; joins precede renderer/context destruction. |

**12/12 cases, 246 assertions, zero failures.** Full async executable time:
0.8658 seconds.
Individual frame counts vary with worker scheduling; acceptance depends on state
barriers and invariants. All 74 traced mesh/texture creations have exactly
one matching destruction on the main/context-owning thread, before renderer
shutdown ends and before context destruction. Worker decode IDs differ from that
thread; no decoder event follows worker join. Raw per-operation evidence is in
`final/integration/*.events.tsv`; the machine-readable coverage is `coverage.json`.

At the held-read progress boundary: four frames, four fixed steps, dynamic-body
x moved from -2 to -1.98333311, 24 draw calls / 288 triangles, spatially differing
framebuffer pixels, input changed gameplay view, and the asset remained Loading.
No human visual-validation claim is made.

## Reused production/editor evidence

- **35/35 production suites passed**, including all existing physics/fluid
  regressions (without modifying those systems). Test log formats differ;
  `passing_lines` in JSON counts matching output lines, not every internal assert.
- FTFT1 remains intact: fingerprint **132**, world-state **302**, lifecycle **78**,
  scene **76**, project **107** checks pass (**695** total). The protected source
  and FTFT1 evidence paths compare unchanged.
- Job suite: **74** checks, including five additional supporting CpuReady tests.
  Its headless residency seam remains supporting state-machine evidence only.
- Existing editor autotest ran default async: seven workers, one completed asset
  job, one actual mesh upload, real Play world, undo and Play/Stop leave authored
  scene IDENTICAL; saved scene matches source bytes. It is machine-checked,
  not human visual review. The displayed frame-140 fixed-step count was zero
  (per-frame counter); ongoing simulation proof comes from the held-read case.
- Existing runtime script: three real fixed steps, explicitly **blocking**.
- M31 stress: 40 assets, real GL; async load 0.511085 s,
  blocking load 1.279410 s. Its opportunistic race timing and
  unconditional final success return are not deterministic acceptance evidence.
  Its measured async maximum frame was 51.1281 ms (four frames above 33 ms);
  no hard realtime responsiveness guarantee is claimed.

Final incremental build: 5.575 s;
whole validation command: 36.623 s. No build warnings were reported.
`integration-first.log` and root-level event traces preserve the earlier
246-check development pass; `final/` is the authoritative tested source run.

## Limits / checks not run

No manual editor project-picker interaction, human visual review, hardware-GPU
matrix, or sanitizer/race-detector run. Default-async editor startup/Play/Stop is
covered separately from the deterministic pending-load EngineHost handoff.
Cancellation inside an indivisible decoder is neither attempted nor claimed.
Fonts/terrain construction, project scanning and authored-scene parsing keep
their existing synchronous startup contracts. Uploads remain main-thread work;
the two-upload budget limits count, not milliseconds. Observers are optional,
nonthrowing/thread-safe diagnostics and must not reenter manager operations.

Protected fluid prototypes were not modified or run. Their Git tree remains
`b71542792b83ac3badf8a449bf4d6e7ea04f217a`; this work makes no new fluid claim.

## Exact changed source/documentation files

- `CMakeLists.txt`
- `README.md`
- `docs/ARCHITECTURE.md`
- `docs/FTFT.md`
- `src/Application.cpp`
- `src/Application.h`
- `src/AsyncFile.cpp`
- `src/AsyncFile.h`
- `src/EngineHost.cpp`
- `src/EngineHost.h`
- `src/Renderer.cpp`
- `src/Renderer.h`
- `src/ResourceManager.cpp`
- `src/ResourceManager.h`
- `tests/JobTests.cpp`
- `src/ResourceTrace.h`
- `tests/AsyncApplicationTests.cpp`
- `scripts/ftft2_validation.py`

New evidence is confined to `docs/evidence/ftft2/` (including this report, source
fingerprints, pre-fix witness, logs, traces and editor screenshots). No binaries,
cache or machine-specific build output are included in the review files.

Proposed commit: `Fix FTFT2 CPU-ready cancellation and verify async application loading`.
The changes remain unstaged and uncommitted for operator review.
