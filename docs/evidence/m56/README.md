# M56 candidate — integrated performance profiling and diagnostics

Starting checkpoint: `7b1e5ea1eab46456bead72b0dcdc7b7fdc9b36fd` (accepted M55).
Candidate only: no commit, push or tag. M57 was not started. Human UI review is pending.

## Implementation / review scope

`src/PerformanceProfiler.*` owns bounded process-level diagnostics; native RAII,
parent-path hierarchy, counters, independent worker lanes, actual outer frames,
fixed-step identities, startup, waits, lifecycle markers and owned recent/spike
snapshots. Instrumentation uses actual runtime boundaries and cached subsystem
counts. It does not change liquid cadence, solver algorithms, physics, resource
scheduling, quality or game policy. Existing specialized timers/HUD remain.

Renderer owns asynchronous GL 3.3 timestamp pairs (128 pairs). Only older available
results are read; pending/unavailable data is explicit and source-frame associated.
The service/UI never makes a GPU result request block for profiling.

`profiler.scope(label, callback)` preserves exactly-once execution, return/exception
and interruption with capture on or off. `profiler.counter(label, value, mode)`
provides bounded annotations. Types/reference/inventory/live enumeration agree:
21 exports, 191 public symbols, 124 native operations, 13 callbacks, 22 unique
cookbook examples. No gameplay timings/readback API.

**Guide:** [PROFILER.md](../../PROFILER.md).
**Script reference:** [profiling.md](../../judasjs/profiling.md).
**Exact files:** [CHANGE_SCOPE.txt](CHANGE_SCOPE.txt).
**Final source hashes:** [FINAL_FINGERPRINTS.json](FINAL_FINGERPRINTS.json).
**Performance:** [PERFORMANCE.md](PERFORMANCE.md) / [machine-readable data](PERFORMANCE.json).

## Validation

- One fresh Release build, resumed in the same clean build directory after fixing
  the ordinary legacy direct-source executable link omission; zero warnings.
- All **107 production suites passed once**; actual async **12 cases / 246 checks**
  passed. Raw results are preserved in `final/production/results.json`.
- Broad runner correctly reported its source-stability guard false: two additional
  concurrent-counter tests and a native overhead probe were added to
  `tests/PerformanceProfilerTests.cpp` before that target compiled. No runtime
  source changed during the broad run. The raw false guard was NOT rewritten.
  `final/RESULTS.json` records the narrow stable-source follow-up separately.
- Core **26**, JS **14**, final application **103** checks pass. Warmed native
  scope allocation test records zero heap allocations; overflow is bounded/explicit.
- Cookbook **185 checks**, TypeScript 5.9.3, API negative-drift tests and actual
  runtime enumeration pass. TypeScript was temporary tooling, not a dependency.
- Actual Spring Range/M55 loops: interleaved off/hidden/visible, equal input and
  120 authoritative fixed steps per trial; physics, script and liquid state hashes
  identical across modes. Bucket manipulation exercises real opening exchange,
  carrying and pouring; no quantity writes. Final captures have zero event/GPU
  drops and zero truncations. Deliberate synthetic saturation/freeze tests report
  drops/incomplete data as expected instead of hiding them.
- Editor renders real profiler UI, Play/Stop preserves authored scene. Narrow
  final interaction smoke proves F8 cursor release, frozen capture with continuing
  unpaused simulation, and restored capture after closing the panel. Separate edit
  viewport CPU and completed editor-camera GPU intervals are verified. Frozen GPU
  arrivals are deliberately discarded and counted, not evidence of simulation loss.
- Public scene reload reconstructs runtime state and marks the capture boundary.
  Export moved to `/tmp/Judas_M56_Spring_Range`; launched from unrelated `/tmp`.
  Ordinary startup/reload/quit succeeds both with profiling off and opt-in on.
  The exported fixture deliberately auto-reloads/quits and is a smoke package,
  not the operator's interactive demo.
- Late diagnostic-only fixes received affected builds/checks: live editor cursor
  routing, per-submission counts (standalone engine totals accumulate), selected
  snapshot retention, and final profiler window placement/column readability.
  Broad production/async suites were not repeated.

## Genuine development failures and follow-ups

Preserved under `development/` and the raw final records:

1. First clean build: legacy tests directly compiling instrumented physics/renderer
   sources omitted the collector implementation. Shared `judas_profiler` archive
   now links all local executables; resumed the same clean build.
2. Reused JS fixture directory made the second metadata fixture invalid; fixture
   recreation corrected. The initial crash/diagnostic log is preserved.
3. Executed profiling example initially called a nonexistent public `saveState()`;
   uses the real `.state` surface. Temporary TypeScript path and conservative
   catch typing were corrected; original logs retained.
4. Broad source guard failure above; stable core follow-up preserves both facts.
5. Reload smoke generator selected a helper module instead of the attached script;
   observed `Player.start: TypeError: not a function`. Corrected only ignored
   fixture generation, then reload/export/moved-startup passed.
6. Review found accumulated standalone counters were mislabeled per-frame; delta
   observation corrected and 103 application checks passed. Renderer totals and
   rendering behaviour were preserved.

## Limits

Bounded recent capture, not a full trace/heap profiler. Audio backend callbacks
are uninstrumented; only safe main-thread audio work is timed. Workers publish
complete intervals (long queue waits can span many frames), not partial jobs.
GPU intervals can overlap and include scheduling; do not add them to CPU/wait
numbers. Long/exhausted labels, full lanes, old GPU results and cross-capture
scopes are diagnosed. Intern identities remain bounded for process lifetime.
Profiler memory estimate excludes consumer copies/allocator/GL driver allocations;
resource bytes are estimates, not measured VRAM. Data consumption/export is main
thread only. First-use/registration/history warmup may allocate. No zero-overhead
or desktop FPS claim; these automated timing runs use software GL under live
operator workload. Human responsiveness remains operator authority.

## HUMAN CHECKLIST

Projects (content unchanged):

- `projects/shooter_game/shooter_game.judasproj` — Spring Range.
- `projects/liquid_surface_demo/liquid_surface_demo.judasproj` — accepted M55 demo.

1. Open Spring Range with `build/judas_editor`, View → Profiler, **Collect**, Play.
   Inspect scripts/navigation/motor/physics scopes and counters.
2. Press **F8** for a free cursor while Play continues. Freeze capture, select a
   history frame/spike, inspect it, resume. F8 again restores game capture.
3. Inspect GPU source-frame/pending data and counts; toggle Collect/window visibility
   and check responsiveness. Closing the window restores normal cursor policy.
4. Open M55, move a body/use the bucket. Distinguish **Dynamic surface solve**,
   **Solid excluded storage rebuild**, **Liquid body loading**, **Water optical
   paths** and upload/submission; no simulation quality changes were made.
5. Export JSON to the editable path (default `/tmp/judas-profile.json`), read it,
   Stop/reload and confirm the retained lifecycle recordings remain understandable.
6. Export the ordinary project and launch standalone. Default collection is off;
   `JUDAS_PROFILE=1 JUDAS_PROFILE_OUTPUT=/tmp/profile.json` opts into a report on
   normal exit. Confirm ordinary game behaviour and acceptable responsiveness.

Protected FTFT/P1/history and current project assets are unchanged. The pre-existing
operator `docs/ROADMAP.md` edit is preserved byte-for-byte and excluded from M56.
