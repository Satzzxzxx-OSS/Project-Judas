# M60 genuine development failures and follow-ups

The original logs under `development/` are retained. These are ordinary focused
development corrections, not extra certification gates.

- Initial integration builds found a miniaudio frame-count type mismatch, a missing
  project audio settings include and a nonexistent world streaming accessor.
  Corrected through the actual existing APIs. Early misleading-indentation warnings
  and an accidental editor helper brace deletion were repaired; unrelated editor
  helper lines restored byte-for-byte to HEAD.
- Existing 59-check audio compatibility caught custom-source looping and the
  attenuation-none diagnostic regression. Both corrected; final compatibility is
  59/59, preserving independent voices, old loops/attenuation and normal Stop.
- First real PCM reverb fixture placed its impulse at sample zero, inside the
  backend's sound-start gain ramp. The fixture now puts the impulse at frame 1024;
  a measurable decaying tail is asserted. No fake reverb or runtime repair.
- First world fixture failed because authored group strings contained newlines,
  incompatible with the project's line tokenizer. Audio group serialization is
  invariant one-line data, and a full Project round-trip check was added.
- Region checks initially used nonexistent state `inactive` instead of M59's
  `unloaded`, and inspected an adopted buffered voice before asynchronous readiness.
  Corrected fixture waits for the actual voice and asserts original handle survives.
  Repeated trips then clear every local stream/retirement with root cursor advancing.
- Cookbook's first independent shot ran in its fixed-step-only fixture without
  starting world audio (historically play-request bookkeeping had not required it).
  The fixture now begins normal world audio/resource readiness before script calls.
  The new example also exercises the public master group and diagnostics.
- Type checking caught a missing `group` field in the cookbook state initializer.
  Added the declared boolean; subsequent nullable service return is handled explicitly.
- Final pre-gate source review included independent one-shot obstruction demand and
  correct reuse of sampled source velocity. This is audio control, not gameplay.

All corrected tests use the production graph/geometry/lifetime paths. Original
failures remain visible alongside corrected logs. Human auditory acceptance is
not claimed by these results.

## Candidate gate and narrow final follow-ups

- The one clean Release/async/production gate ran 119 suites. 118 passed; the
  technology-project asset integrity assertion failed because the already accepted
  icon checkpoint placed a PNG under `assets/branding` without asset metadata.
  The PNG is unchanged. A normal stable texture sidecar and the corresponding
  expected asset count/identity check correct that shared registration hygiene.
  Only `judas_project_tests` was rebuilt/rerun: PASS. The original failed gate
  report/log remains unchanged; it is not relabelled an all-green first run.
- The original full cookbook run had 209 checks with one streaming outcome failure.
  Its artificial 100-frame fast loop did not allow enough real job time to hash/load
  the expanded project. Streaming alone now allows 500 frames with 1 ms yield;
  its follow-up is 9/9. No runtime scheduling limits were weakened.
- Final review tightened audible demand for obstruction: zero-gain/muted/paused
  routing and zero-volume emitters do not spend geometry queries. The 65-check
  audio-world follow-up includes actual muted-master query rejection. Acoustics
  185/185, application 19/19 and actual-device 17/17 pass afterward. Seek/detach
  timing output was added to the existing focused fixture, not to the callback.
- The original gate recorded source-review edits while the clean build was in
  progress. WorldAudio compiled after its reviewed one-shot fix; focused test
  sources compiled after their additions. Its `tested_sources_unchanged_during_run`
  flag is honestly false. A final incremental dependency build verified every
  target up-to-date, and affected final audio checks/cookbook follow-ups were run.
  No second full production or async gate was run.
- Editor, final lab/device/reload and both moved read-only package startups passed.
  The original gate's copied input projects and bulky unrelated raw regression
  profiles remain locally under `.cache/m60-gate-artifacts`; relocation paths and
  unchanged hashes are in `LOCAL_GATE_ARTIFACTS.json`. Logs/results and M60 proof
  remain in the review tree. Nothing under protected historical evidence moved.

- Final reproducibility review added `CreateAudioFixtures.py` and candidate preflight
  generation so a fresh clone need not rely on an existing ignored cache. The
  fresh generated fixtures passed the acoustics check, including explicit corrupt
  input/retirement proof. No demonstration recording or runtime code changed.

- A final pause-policy review corrected newly requested independent one-shots in a
  paused group/master: they return false/discard without allocating a deferred
  voice, rather than bursting on resume. Existing held voices still resume from
  their cursor. Real PCM silence and the public cookbook prove the behaviour.
- Pinned decoder review exposed potentially full-file MP3 length queries during
  prefill. Streaming now forces the supported filename codec, never queries MP3
  length on startup, and publishes unknown duration until natural decoded EOF.
  A no-Xing MP3 fixture starts/readies and learns its true EOF length. WAV/FLAC
  still obtain header lengths. This is a boundedness correction, no backend
  upgrade or custom codec parser. Final acoustics are 194/194, world 65/65,
  compatibility 59/59, audio cookbook 9/9 and application/export 19/19.
