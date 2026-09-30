# M34 executed results

Human listening was **accepted by the operator on 2026-09-30**. Automated tests cannot establish how this sounds.

- Starting HEAD: `bd22935fe4954c994178e5b854b87d42fcecfee2`; no commits made.
- One genuinely fresh Release configure/build: PASS, zero compiler warnings.
- One complete production run: 62/63 suites passed. Project tests exposed changed headless error wording and the shipped asset count (4 became 7). Raw failing output remains in `final/production/judas_project_tests.log`.
- Corrected only those failures and two backend wrapper issues: no-falloff spatial voices now retain panning; explicitly stopping a finished one-shot reports Stopped.
- Affected targets rebuilt; project suite PASS (0 failures), audio suite **59 checks / 0 failures**. No second full matrix. All 63 suite outcomes pass across that run and the affected reruns; this is not a claim of an unchanged-source full rerun.
- Actual async application: 12 cases, 246 assertions, PASS. Editor default-async Play/Stop/save/reopen PASS; standalone startup blocking reference PASS. No human visual or listening validation claimed.
- Protected FTFT evidence, M33 evidence and P1 prototypes unchanged. Physics/fluid sources and WorldState are unchanged; fingerprint schema deliberately advances to 4 for authored audio.

## Lightweight performance

| Actual voices | WorldAudio mean update (µs) | CPU to mix one second (ms) |
|---:|---:|---:|
| 0 | 0.006154 | 0.013188 |
| 4 | 0.647955 | 1.661482 |
| 32 | 3.308405 | 11.413147 |

1000 ordinary world-audio updates per case. Actual backend DSP advances 48000 frames in explicitly device-free mode; no recording/perceptual analysis. These are measurements, not device-latency or universal real-time guarantees.

## Reproduction and provenance

Focused commands are in README.md and RESULTS.json. The one-shot clean gate is `python3 scripts/m34_validation.py`; it preserves existing runs and refuses to overwrite `final/`. To repeat later, archive this run first. The original complete-run source hashes are in `final/production/results.json`; post-run source corrections are identified in RESULTS.json, and the final source is fingerprinted in FINAL_SOURCE_SHA256.json. Raw iteration failures remain preserved. The inherited runner's FTFT2 scope label refers to its original implementation; M34 reuses it without changing fixtures/assertions.

## Dependency and limitations

Pinned miniaudio 0.11.23 from official upstream, MIT-0 option; runtime requires no downloads. WAV/MP3/FLAC, whole mono/stereo clips, 48 kHz PCM, 512 MiB decoded limit. OGG/streaming, acoustics, HRTF, doppler and editor preview are not provided. The operator accepted listening validation on the visible default-device demo. Runtime-created entity definitions retain their existing limited component contract; transient sounds use AudioSystem directly. Strict schema-3 saves are rejected rather than silently migrated.
