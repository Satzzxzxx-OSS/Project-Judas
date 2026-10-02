# M38 candidate results

- Starting clean checkpoint: `d1136c4b1049e043b40abf24fdaeed722cfc99ed`.
- One genuinely clean Release build: PASS, **zero compiler warnings**.
- Existing production gate: **70 suites PASS**, 12 async application cases /
  **246 checks PASS**. Existing editor async Play/Stop, standalone blocking
  reference and supporting M31 diagnostic also passed. About 594 seconds total.
- Focused exporter: **18 checks PASS**. Covers package structure, startup/input,
  prefab→mesh→texture dependencies, excluded unregistered junk, stable relative
  metadata, matching effective authored fingerprint, stale replacement, failed
  export preserving the prior package, moved scene resolution and external saves.
- Packaged normal application: **13 checks PASS**. Copy/move to `/tmp`, launch
  from `/`, poison development engine-data override; ordinary async resource
  pumping decodes mesh/texture/audio, creates voices, renders particles and the
  secondary camera, spawns a prefab by ID, and quits through SDL_QUIT.
- Editor Export + existing Play/Stop: **PASS**, authored scene IDENTICAL.
- Actual exported Release `judas`, moved outside repository/build, no project
  argument, CWD `/`: **PASS**. Existing three-step/screenshot harness is a
  blocking startup reference, separately labelled from async application proof.
- Review package: `/home/conner/Documents/Judas_Export_Demo_M38`.
  **8 registered assets, 1 scene, 4,695,361 bytes (4.48 MiB), 0.015 s export**.
  System platform libraries are prerequisites, not counted/bundled in package.
- Protected FTFT/P1/M33–M37 evidence and fluid prototypes: **unchanged**.

## Narrow late correction

The initial editor export automation crashed after successfully creating its
package. Debugger traced `ImGui::BeginPopupModal` via `HandleRequests`: the new
hook incorrectly called that UI handler after `ImGui::Render`. It now submits
Export alongside normal requests while the frame is active, exactly where the
real button is handled. Export/resource algorithms were unchanged. Only the
editor target was rebuilt and the affected Export/Play/Stop and moved-runtime
checks repeated, all passing. The full production suite was not repeated.
Original failed output/exit -11 and the backtrace remain under final/development;
`SIGNOFF.json` records current acceptance, `RESULTS.json` retains attempt history.

`final/SOURCE_SHA256.json` fingerprints final reviewed implementation/assets/docs;
`final/CHANGED_FILES.txt` enumerates them. The original full-suite fingerprints
remain in `final/production/results.json`, distinct from the late automation fix.
No commit, push or tag. Human visual/listening validation remains PENDING.
