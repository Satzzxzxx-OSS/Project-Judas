# Post-M65 corrective-pass evidence

Starting checkpoint: `26e6f089fde0f39d68659882f6204da3a453473e`.
All records here are new outputs. [Original accepted review](../post_m65_consumers/README.md) and FTFT/P1/M33–M65 evidence are untouched.

[Current closure report](../../POST_M65_CONSUMER_REPAIRS.md).

## Evidence map

- `baseline/`: exact accepted query/reference helper observations. The old capsule repro deliberately returns success when observing a defect; that exit code is not a passing geometry regression.
- `query-current.log`: independent capsule regression assertions; `scenarios/void-query-*`: actual pilot capsule and real suit damage, including removal of the copied game's approximation.
- `metadata-adoption-followup.log`: final focused metadata/reference lifecycle assertions, including live two-region adoption/replacement/destruction and zero repeated VM construction.
- `reference-after-first.log`: supporting intermediate JSON-only helper timing. Final rebuilt helper timing is recorded separately.
- `scaling/baseline-font-root` and `scaling/after-current`: unchanged original inert-scenery reproduction at 0/400/800/1600 boxes, startup separately recorded, no-script and one-script cases. Earlier missing engine-root font failure is preserved, not accepted as baseline performance.
- `resident-comparison.json` and `application/*-pinned`: matched ordinary hardware-rendered resident workloads. Exact region IDs are explicitly demanded in a temporary measurement fixture. Earlier unconstrained `*-comparable` records drifted to different four-region identities and are excluded from matched speedup claims.
- `scenarios/skate-rig-*-diagnostic*`, `scenarios/*bone*`, `ragdoll-resistance*`: original sustained physical head/foot motion and diagnostic follow-ups. The original undamped fixture is not claimed to sleep. Only the copied current rig receives the explicitly operator-authorized passive resistance.
- `ragdoll-preflight-followup.log`: internal 90-check rig candidate. Final clean-gate rig output additionally measures sleep latency/quiet cost and sustained torque; use that final result when available.
- `streaming-units-final.json`, `scenarios/skate-revisit-units-final`: named unit timing with the entire registration/remap work measured, unchanged 2 ms target, actual out/back activation and retained-state restoration. `scenario-profile-summary.json` distinguishes CPU work from scripted-clock FPS.
- `application/harness-switch-final`: original harness service failure is repaired through the common shipping boundary; later rebuilt-runtime verification is recorded separately. Paused captures and scene assertions are explicit.
- `application/harness-skate-save-final`: genuine save prefill failure when no-draw frames skipped ordinary audio publication. `application/harness-skate-save-audio-followup`: corrected save/load follow-up, not a waived failure.
- `scenarios/rooftop-course-final` and `scenarios/void-input-final`: ordinary consumer course/checkpoints and boarding/fire.
- `candidate-freeze.json`: exact implementation/tooling/consumer bytes before the single clean Release gate.
- `final/`: clean Release, actual complete production runner, async integration, and the three new conventional focused targets. Existing contracts are reused with outputs redirected here and reviewed shared-source exceptions, not rewritten historical assertions.
- `api-final-types-source.log`: TypeScript 5.9.3, registered-source/type/reference drift and negative controls. Final live VM enumeration/cookbook output is recorded separately.
- `package/`: final editor Play/Stop, real-time games, ordinary reload/paused switches, Release export and physically moved package (after the gate completes).

## Failures and interpretation

Original failures and subsequent follow-ups remain. Early compile/fixture-driver errors (target names, resource readiness after world demand, cold participant preflight, an adopted entity correctly retaining its tombstone, missing benchmark engine root) are recorded separately from engine defects. They are not represented as passing final results. A stale binary accidentally invoked after one failed metadata compile is not final proof; the rebuilt 32-check adoption follow-up is.

The intentionally malformed imported-module diagnostic in the package driver is a negative diagnostic observation, not an acceptance failure or a runtime feature change.

Raw profiler history is bounded to the most recent 120 frames plus startup/spikes. Open scopes in a capture taken during the frame are retained honestly; dropped/nested/incomplete diagnostics are not reset. Scripted captures include readback/writing work in frame intervals, so their high maximum intervals are not labelled ordinary simulation stalls. Normal real-time measurements are reported separately.

No human visual acceptance is claimed by these automated captures. Candidate remains uncommitted for review.

## Final gate and narrow follow-ups

The single broad gate preserved its original failure: 136/139 production targets passed. Final complete passing coverage is in `final-coverage.json`: consumer authoring (missing arguments, 35 checks), save storage (required safe directory prefix, 28 checks), and isolated audio-world lifecycle (script metadata was never initialized, rebuilt 65 checks) each received only its affected follow-up. The frozen runtime bytes are unchanged. Exact changes to the test and validation adapter are recorded alongside old/new hashes; the initial freeze is not rewritten.

Final capsule 40 / metadata 32 / real-rig 93 checks pass. Cookbook 244 checks and actual live VM enumeration pass (289 symbols / 27 exports). `consumer-cold-driver.py` reproduces two separate processes with the same temporary copied project/save namespace; `consumer-cold-save` proves score 314 restored. Rebuilt harness diagnostics have zero dropped/nested/incomplete/open scopes. Package checks, real-time game profiles and final Skate API / Void Ruby observations are complete.

## Archive and generated-package hygiene

`profile-archives.json` maps every losslessly compressed M56 JSON to its original path, byte count and SHA-256. Standard gzip decompression restores the exact recorded JSON; original gate result/checksum records are retained. No profiling samples were discarded to reduce storage.

The developer-authoring test generated an export inside its output directory. That temporary package is retained at `.cache/post-m65-repairs-authoring-export`, outside source/evidence staging. `generated-package-relocation.json` records every original path, retained path and original hash. The accepted playable export and moved package are also outside the tracked source tree. There are no ELF executables in this evidence directory.
