# M40 implementation evidence

Candidate based on `4697675c8878ddf101d7df6a3f451a5119a6f57b`; uncommitted for operator review.
Human gameplay/visual/listening validation is **PENDING**.

## Reproduce

From the repository root: `python3 scripts/m40_validation.py` performs one clean
Release build, the existing production gate, editor Play/Stop and standalone demo.
Evidence directories are protected against overwrite. Archive existing M40 output
and the ignored `.cache/m40-before-clean-release` rollback build before reproducing.
`python3 scripts/m40_validation.py --focused-followup` is the narrow follow-up used
for the late handle lookup correction; it does not rerun the full production suite.
Dependencies match the existing Linux build; QuickJS-NG is vendored, no Node needed.

## Executed results

- Clean Release build: PASS, 509.301 s, **zero compiler warnings**.
- Complete production run: **73 test executables**, 72 initially passed. Lifecycle
  reported one stale body-to-entity lookup regression (75/76 checks). The new static
  lookup fallback was returning an unloaded dynamic body's cached handle. The fix
  excludes dynamic records from that fallback, preserving the existing live-generation
  lookup. No solver, physical fixture or assertion changed.
- Follow-up: lifecycle **76/76**, scripts **34/34**, script application **19/19**,
  persistence **307/307**, prefabs **34/34**, classification **39/39**. All PASS.
- Actual application tests cover script input, audio/particle requests, prefab spawn,
  saved state restored before callbacks, and M38 export/move/launch from `/` with
  an unrelated development-root environment. They use ordinary Application::Run.
- Existing async application integration (all 12 cases), editor async Play/Stop,
  blocking runtime reference and supporting M31 stress diagnostic passed in the
  complete run. Their scope is preserved in its machine-readable result.
- M40 editor smoke: authored scene after Play/Stop **IDENTICAL**.
- M40 normal standalone demo smoke: PASS.
- Script frame + fixed pair mean: 0 scripts **0.011 us**; 10 **32.330 us**;
  100 **770.891 us** (200 pairs each, includes representative native transform reads).
  These are lightweight measurements, not realtime guarantees.

`final/` retains the original full run, including its lifecycle failure and the
source-during-run guard failure: only RuntimeWorld.cpp and the M40 runner changed
for the identified fix while the long suite completed. `followup/` contains the
recompiled affected tests and final candidate source hashes. No second full-suite
run was performed. `preflight_guard/` records the first guard-only rejection before
any build/test execution; the new runner explicitly reviews the authorized M40
WorldState extension. Historical validation scripts were unchanged.

## Source and scope

`followup/SOURCE_SHA256.json` fingerprints final candidate source/assets;
`CHANGED_FILES.txt` enumerates all changed/new files. Raw logs/results are retained,
including earlier focused checks. Protected FTFT/P1/M33–M39 evidence and prototypes
are unchanged. No commit, push or tag was made.

See `docs/SCRIPTING.md` for APIs, limits, compatibility and the operator checklist.
The demo is `projects/script_demo/script_demo.judasproj`.
