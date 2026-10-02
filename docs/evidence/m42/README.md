# M42 collision and sensor event candidate

Starting HEAD: `01d4a920fff3c407cbda702d8da2a62972dd224f`.
No commit/push/tag. Human validation is PENDING.

## Executed results

- One genuinely clean Release build: PASS, zero compiler warnings.
- One complete production run: 77 suites; 76 passed initially. The existing lifecycle performance assertion failed because the new script bridge scanned/copied entity definitions in worlds without scripts. Original nonzero result retained in `final/production/results.json` and its raw lifecycle log.
- Narrow repair: one early return in `RuntimeWorld::DispatchPhysicsEvents` when there is no script consumer. No physics law, geometry, tolerance or assertion changed.
- Unchanged lifecycle test rerun: PASS. All-full 1500-entity workload 14.095 ms/step; reduced workload 0.947 ms/step (original candidate: 18.691 and 5.054 ms/step). These are the existing test's averaged step timings, not seven-run median benchmark claims.
- Focused current-source event tests: 26 checks, zero failures.
- Normal Application demo tests: 7 checks, zero failures over the normal fixed-step/resource/render path. Scene bodies, filtered blue patrol, JS HUD and a runtime prefab participant exercised.
- Existing async integration: 246 checks, zero failures, in complete run.
- Demo editor Play/Stop: PASS, authored scene after Stop IDENTICAL.
- Demo standalone startup: PASS; Application test additionally supplies actual event/UI execution evidence.
- All required suites/checks pass with the affected-path rerun. The full suite was not redundantly rerun after the one-line no-script guard.
- Final source fingerprints: 33 entries, all matched; `followup/SOURCE_SHA256.json` is the final candidate manifest. Only `src/WorldScripts.cpp` differs from the original complete production run's source manifest.
- Protected FTFT/P1/M33–M41 evidence/prototypes and historical failures remain unchanged.

## Reproduction / artifacts

Clean gate on the final candidate: `python3 scripts/m42_validation.py` (requires fresh evidence destination; refuses overwriting old evidence). The narrow follow-up commands/source are recorded in `followup/run.py` and `followup/RESULTS.json`.

`focused/` retains earlier demo failures: event callbacks arrived, but a patrol script accidentally reset its per-entity event counter each step. The script reset was removed; `focused/application-final.log` and current-source follow-up show the correction. The unrelated complete-run lifecycle timing failure is also preserved, not relabelled a pass.

Architecture/API/scope: `docs/COLLISION_EVENTS.md`. Demo: `projects/touch_demo/touch_demo.judasproj`. Exact changed content/source list: `CHANGED_FILES.txt`.

## Accepted candidate scope / human review

Real rigid-body contact/TOI observations, discrete sensor endpoint overlaps, existing M39 bilateral filtering, deterministic per-pair fixed-step transitions and safe M40 entity wrappers. Sensor colliders never solve physical response. Custom sweep-only player locomotion, fluid particles and continuous trigger crossing are not event sources. Normal impulse is supplied only where existing support solve data is available; otherwise null. Exit carries last contact data. Collider enable toggles and pair history are runtime state, not automatically persisted game state.

Operator checklist: red/grey collision reaction; green sensor without physical blocking; exit when patrol leaves; excluded blue stays silent; P-spawned prefab reacts independently. No human visual or input acceptance is claimed.
