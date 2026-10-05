# M60 candidate results

Starting/final HEAD: `277e67d2b0811644b2122bae27a4301d6dd3c96b`.
Uncommitted candidate. Human auditory/gameplay acceptance is **pending**.
No M61 work. No commit, push, tag or staged changes.

## Candidate gate and honest adjudication

One fresh Release configure/build completed with zero compiler warnings. One
production run covered 119 suites: 118 passed, with the technology-project asset
integrity assertion failing on the accepted icon PNG's missing metadata. The PNG
is unchanged; a normal stable texture sidecar and expected tenth-asset assertion
were added. Only the project test reran: PASS. Original failed gate/log preserved.

Actual async integration passed all **12 cases / 246 checks**, including editor
Play/Stop and a blocking reference run. The original gate also flags source edits
while the build/review completed (`tested_sources_unchanged_during_run=false`).
That flag has not been hidden. See [follow-ups](DEVELOPMENT_FOLLOWUPS.md) and
[thread audit](THREADING_REVIEW.md): relevant reviewed audio/test sources compiled
after their changes; final dependency builds and affected checks establish the
final narrow corrections. No second broad production/async gate was run.

## Focused final checks

| Check | Final result / evidence |
|---|---|
| Production WAV/MP3/FLAC PCM, seeks/EOF/loops/starvation/retirement, Doppler/filter/reverb/groups, serialization/prefab/fingerprint | **194/194** — `acoustics-review-measurements.log` |
| Real geometry PCM, angular/teleport motion, deterministic zones, muted demand, three real region trips/adoption/cancellation/cleanup | **65/65** — `world-review.log` |
| Existing buffered audio compatibility | **59/59** — `compatibility-review.log` |
| Real application, logical menu/control, reload/cleanup and ordinary export | **19/19** — `application-review.log` |
| Brief actual desktop-device operation (low authored levels, no OS volume change) | **17/17** — `device-final.log`; subsequent final packages also use the device |
| API/live inventory/TypeScript | **236 symbols, 24 exports, 155 native operations, 13 callbacks**, TypeScript 5.9.3; three negative drift controls pass — `api-final.log` |
| All 25 cookbook scripts | Original 209 checks had one streaming wall-time fixture failure; streaming-only **9/9** after adequate real job time. Final audio example **9/9**, including paused one-shot discard. Other examples passed in the original run. |
| Editor authoring/Play–Stop and lab UI export | Authored state **IDENTICAL** after Stop for lab and range; range additive preview/activation passes — `editor/` |
| Final moved/read-only exports, unrelated `/tmp` cwd | Both **exit 0**, no script/asset fault, own-window normal close — `package-lab-final/`, `package-range-final/` |

The delayed-decode fixture emits safe zero input, holds its cursor, recovers and
retires without waiting for the delayed decoder. Unknown-duration MP3 starts
without calling the backend's potential whole-file length scan, learns duration
at decoded EOF and supports worker seeks. Paused groups discard new independent
one-shots without voices or a resume burst. Numerical PCM proof is not listening.

## Scope / shared corrections

Audio, project/assets/resource lifetime, generic optional authoring, editor,
JudasJS, export, current lab/range content and focused tooling. Shared corrections:
64 KiB incremental composed-asset hashing with identical SHA-256 output/cancellation;
registration of the unchanged existing icon. Canonical schema 5 remains unchanged.

No physics/fluid/navigation/character implementation was redesigned. Protected
FTFT/P1 research/prototypes, historical evidence and accepted fluid projects remain
unchanged. Current `docs/M59_WORLD_STREAMING.md` only gains its later audio contract.

Bulky generated copies of older fixture projects/raw regression profiles remain
locally under `.cache/m60-gate-artifacts`, **not** as new source/assets. Original
paths and unchanged hashes are in `LOCAL_GATE_ARTIFACTS.json`. Existing protected
historical files were neither moved nor rewritten.

## Human door follow-up

The first human review found E could not move the physical door. Authored jamb/
floor clearance and a raised hall marker jammed the real motor. A content-only
correction now passes **15/15** actual-angle and obstruction-ray checks over two
open/close cycles; corrected lab export/moved startup passes. No engine/JS changes
or broad reruns. Original failure/isolation logs and previous final manifest retained
in [door follow-up](door-followup/RESULTS.md). Human re-test remains pending.

## Handoff

- [Current contracts, controls, limitations and ONE human checklist](../../M60_AUDIO.md)
- [Measurements](PERFORMANCE.md)
- `CHANGED_FILES.txt`: exact worktree change list.
- `FINAL_SOURCE_SHA256SUMS.txt`: final non-evidence source/assets/docs/tool fingerprints.
- `CANDIDATE_RESULTS.json`: machine-readable gate/follow-up state.
- Ordinary projects: `projects/audio_lab/audio_lab.judasproj`,
  `projects/streamed_range/streamed_range.judasproj`.
- Final exports: `.cache/m60-review/audio_lab`, `.cache/m60-review/streamed_range`;
  moved/read-only copies `/tmp/Judas_M60_audio_stream_final`,
  `/tmp/Judas_M60_range_stream_final`. The corrected lab copy is now
  `/tmp/Judas_M60_audio_door_corrected`; the earlier lab copy is historical.

Suggested commit **after** operator acceptance:
`Add M60 streaming audio and environmental acoustics`.
