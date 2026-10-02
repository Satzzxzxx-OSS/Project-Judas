# M44 candidate evidence

Starting HEAD: `6258e7a00a8783b912ee98fa706eabb7c729fe4a`.

- `focused/geometry.log`: 26 geometry/filter/state/tree checks; 1,024-body query timing.
- `focused/application.log`: corrected real GL/Application + JS API checks.
- `focused/initial-build.log`: original test wiring error (`GetBodyTransform` vs `GetTransform`).
- `focused/initial-application.log`: original test-only relative import lacked asset metadata;
  follow-up registers it through normal asset identity. Neither is a physical failure.
- `final/`: clean Release production/async, focused tests, editor Play/Stop,
  standalone and moved export results. `RESULTS.json` records the actual gate;
  `SOURCE_SHA256.json` fingerprints tested changed engine/test/script/demo files.

Reproduce with `python3 scripts/m44_validation.py` in a clean evidence checkout.
The runner refuses to overwrite recorded results. GPU smoke uses a real GL
context with SDL offscreen / Mesa software, not human visual validation.
No protected historical evidence is rewritten; only fixture-output destinations
are adapted by the existing regression infrastructure.

The imported demonstration font keeps its original LICENSE.txt. No new library
or license dependency is introduced. Full API/limits: `docs/PHYSICS_QUERIES.md`.
