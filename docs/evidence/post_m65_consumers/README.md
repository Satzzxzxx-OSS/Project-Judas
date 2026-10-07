# Post-M65 consumer review evidence

Accepted engine: `51c18c249432e09910dabc41e670bf960db8050c`.
Runtime SHA256: `5208198640f9573e48a065097f7cbef589f39175cdedc6d5652b82a0a713f3fd`.

[Current conclusions and original-finding table](../../POST_M65_CONSUMER_REVIEW.md).
The four copied original reports are immutable inputs to this review, not edited
historical evidence. `import-provenance.json` records original consumer assets.

- `individual`, `switch-desktop`: initial ordinary-loop real-input/coexistence checks.
- `rooftop-*`: actual course, wall-run, incline and checkpoint scenarios.
- `skate-apis-followup`: actual imported rider joints, IK, socket, runtime joint,
  physical-material identity. `skate-save`: normal save/load and Spanish locale.
- `skate-stream-revisit-followup`: normal spatial-interest unload/reconstruction;
  test-only explicit relocation at fixed steps 240/480 isolates revisiting.
- `skate-ragdolls*`: same actual imported 13-body rig, 20 instances and a separate
  test floor; 30 simulated seconds; no solver/fidelity adjustments.
- `void-*`: original mission scenario, public gravity, true motor ray misses,
  retained pilot damage approximation, actual physical fire input, Ruby scenario.
- `scaling`: unchanged original S1 generator and current measurements.
- `reference-vm-cost.log`: isolated M65 dependency-helper cost; no engine fix.
- `capsule-repro-final.log`: independent geometric expectations, observed defect.
- `harness-scene-boundary`: F2 is queued but built-in harness never applies the
  outer scene boundary. It is not a normal-application transition failure.
- `import*-diagnostic`: deliberately malformed imported modules; current stack
  identifies the import filename/line, contrary to the old S3 observation.
- `authoring.log`, `editor-*`: actual consumer multi-edit/copy/reference tests and
  actual editor Play/Stop round-trip. Widgets/controller feel still need a human.
- `package-final`: Release export, nine selected scenes, moved package's actual
  executable startup/pause checks and ordinary outer-loop repeated switching.
- `profile-summary.json`: bounded M56 summaries. Raw exports are gzip-compressed
  losslessly. Nested scopes overlap; do not sum inclusive scopes as total cost.
- `final-source-sha256.json`, `changed-files.txt`, `protection.json`: final handoff.

## Failures are preserved

`initial`: collection script called nonexistent `input.consume`; removed from
ordinary content. `skate-apis`: first fixture used the wrong world-joint anchor
space and serialized an undefined probe value; corrected fixture is separate.
`rooftop-checkpoint`: first test spawn was beyond the roof, not inside the sensor;
corrected on-roof test is separate. `skate-stream-revisit`: first probe used a
nonexistent transform method; corrected to the documented property setter.
`capsule-repro-fixture-failure.txt`: initial native fixture omitted PhysicsWorld
initialization; this was a review-tool error, not the capsule geometry defect.
Initial TypeScript coverage invocation used an unavailable module path;
`api-coverage-followup.log` records the successful installed-checker invocation.
`void-query`/`void-query-followup` preserve genuine engine misses and why removing
the original workaround was rejected. No failure was converted into a false pass.

No full production suite rerun: runtime implementation is unchanged. Focused
review tools are Release builds; existing M65 focused checks also passed.

`void-source-drift.diff` and its two ship snapshots record one external original
package file changing after inventory. The hash-verified frozen import is retained
in the port; the external current file was not modified/restored by this task.

Final evidence-checker fixture errors are retained as `review-verification-*-failure`:
it first selected the collection's empty state instead of the second script slot,
then expected uppercase PASS in a JSON coverage artifact and looked for PCM in
an older pre-mixer capture. Corrected checker reads the actual probe, JSON pass
field and final sequential PCM observations. No expected physics results changed.
