# M39 evidence

Reproduce with `python3 scripts/m39_validation.py` from the repository root
in a fresh checkout/worktree (the runner preserves existing output/build evidence).
Focused checks alone: `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy ./build/judas_classification_tests`.

Automated candidate PASS: clean Release build, zero warnings, 71 production
suites, 12 actual async application cases / 246 assertions, 39 M39 assertions,
normal editor Play/Stop and standalone M39 demo startup. Human validation is
pending. Headless GL execution does not establish human visual acceptance.

`SUMMARY.json` is the compact result. `final/RESULTS.json` and
`final/production/results.json` retain commands, timings and suite exits.
`final/production/judas_classification_tests.log` contains actual physics/GL
observations. `final/SOURCE_SHA256.json` fingerprints the candidate source/demo.
Historical evidence and protected fluid prototypes are unchanged.

A filtered body pair yielded one rejected pair, zero candidate/contact rows.
1000 ordinary Renderer draw requests yielded 1000 versus zero submissions:
5.993823 ms versus 0.007368 ms CPU dispatch in this run. This is a lightweight
submission sanity measurement, not a GPU or whole-scene frame guarantee.

Limits: 64 lifetime IDs per registry, deleted IDs retired; no collision matrix;
runtime tag edits transient; AABB queries conservative; existing capsule sweep
only (no new raycast); shared shadow maps include all layers; aggregate fluid and
engine-owned player/torch visuals Default; fluid particles are not layer bodies.
See `docs/M39.md` for APIs/authoring and fingerprint compatibility policy.
