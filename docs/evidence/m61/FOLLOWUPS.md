# M61 development records

Original logs under `development/` and the early `focused/` directory are retained.
They are development evidence, **not** the final acceptance set. The final review
results identify the later passing commands explicitly.

- Initial compilation failures include missing includes, the entity argument helper,
  archive error-string typing and a test attempting to inspect private motor state.
  Fixes retained the public motor boundary; private solver memory was not exposed.
- The destroyed-hierarchy checks exposed a real lifetime gap: ordinary static/bodyless
  entities needed runtime records, and hierarchy traversal had to use current runtime
  definitions rather than only the subset of managed entities. The corrected focused
  check also verifies old-world native handles cannot alias the restored world.
- A contact snapshot encountered already-retired/unowned pairs. Normal retired and
  explicitly ephemeral endpoints are excluded; required live endpoints still reject.
  Restored authoritative touching history produces Stay rather than replaying Enter.
- The first streamed fixture selected a nonexistent changed entity; the current fixture
  selects an actual authored prop. Root adoption was initially rejected by ownership
  validation, which now accepts the ordinary `root` owner as well as registered regions.
- Editing content between processes correctly rejected the earlier slot. A later test
  also mistook an existing file for a newly committed save when the project's overwrite
  confirmation had not yet accepted the operation. The fixture now sends the confirmation
  edge and requires the actual save request to report completed. Old apparent write
  passes and the subsequent failed read remain recorded.
- The first Save Lab write reported an accounting assertion failure. Final proof checks
  the live ledger, exact restored owner/cell/flow/parcel bytes before resume, and the
  established accounting tolerance on resumed steps. No liquid solver algorithm was
  changed for persistence; the early log is not overwritten or presented as a pass.
- A second streamed save raced the restored HUD's metadata refresh and correctly got
  busy. The fixture waits for that project operation to finish rather than bypassing
  serialization. The successful follow-up writes an independent second slot.
- Initial environment/tooling failures (missing virtual-display helper/Node command,
  declaration example checking) are preserved. Final checks use the bundled Node and
  the existing SDL offscreen backend; human visual/listening acceptance is not inferred.

Candidate validation and any final narrow corrections are recorded separately in
`RESULTS.md`. Protected historical reports are untouched.

## Single candidate gate and narrow final follow-up

The clean Release/production/async gate in `final/` is preserved exactly. Of 124
suites, four initially failed: three legacy camera/prefab checks affected by eager
bookkeeping, and the new save performance test. All 12 async cases / 246 checks
passed; source and protected-path stability checks passed. One new test compiler
indentation warning is recorded in the clean build log.

The performance failure exposed a legitimate boot-save case: before any fixed
step, lazy motors had no participant records, but private reconstruction allocated
them. Persistence preparation now initializes required bookkeeping/motors/agents
without simulation, preserving existing initialized state. Ordinary legacy camera
and prefab construction keeps its previous collection/presentation behavior.
Focused proof covers a save before the first fixed step and real translating/
rotating support on the first resumed steps.

Other narrow finishing corrections: new projects mint a durable namespace instead
of colliding on display name; save request tokens reject NaN/fractions; quiet private
audio teardown does not reset the live world's environment; Range copies round and
cooldown after fixed mutations and keeps its navigation timer in script state;
Save Lab no longer advertises an absent copied radial scene. Measurement warning
and screenshot test compilation errors were corrected with affected rebuilds only.
The broad gate was not repeated or rewritten to hide the original failures.

The final streamed proof reproduced a prefill race: another legitimate region
could publish a persistent voice after the fixture requested Save. Capture now
defers as a whole until ordinary persistent-voice prefill/seek is ready, with a
30-second preparation bound; failed sources still identify the entity/audio reason.
No voice is silently omitted. The original failed stream logs remain. Screenshot
inspection also exposed double offsets in a vertical Save Lab menu; current content
uses its existing layout flow, keeping all four slot operations inside the panel.

## Final application and control follow-up

- The measurement fixture omitted required Scene 3 fields and failed before startup.
  The runner now emits the ordinary format. The subsequent editor assertion read
  stdout only, although the successful Play/Stop identity report was on stderr.
  The successful original editor log remains; only the unrun streamed editor was
  run afterward. These were harness errors, not engine failures.
- A strengthened gameplay proof actually fires twice to defeat a navigator and uses
  the already configured N action to spawn another. It found that the streamed root
  never instantiated the copied `arena.js` spawn handler. Ordinary player JS now
  handles that authored action. The first defeat/load then exposed the old enemy
  visual cue scaling its motor root, contrary to M49's unit-scale contract. Current
  content uses a dark material override; motor/physics rules are unchanged. Original
  gameplay and exported failed-load logs remain. Corrected fresh-process proof
  preserves earned score 250, defeated state and spawned instance/count.
- `input-edge-failure.log` reproduces one held controller-confirm edge after Load:
  clearing the raw device map let its next poll create a new jump press. Runtime
  and editor activation now discard pending edges while retaining held baselines.
  Input 40 checks, fresh Lab load 20 checks and streamed load 19 checks pass; both
  final moved/read-only package proofs were refreshed. No broad suite was repeated.
- `resting-hinge.log` adds a representative first-three-step comparison for a resting
  contact and a spring hinge (position/angle difference below 0.01 m/rad), alongside
  the existing moving/rotating support comparison. All 27 world checks pass.
