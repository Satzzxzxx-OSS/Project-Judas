# Development failures and corrected follow-ups

These records are development evidence, not accepted final results.

- `mechanics-first.log`: physical mass/momentum checks passed; the replacement fixture then attempted to create another entity using existing ID 400. Set its authored ID to 0 for normal runtime allocation. `focused/mechanics.log` passes, including safe body generation reuse.
- The first cold-read fixtures attempted whole-world capture before restored script initialization, assumed an incorrect body count after restoring the active annex, and attempted a further cut after every bond had already failed. Capture correctly defers at the lifecycle boundary. The final fixture compares exact deformable participant bytes, checks a removed-part tombstone, and applies a new **physical** load to surviving connected material. The impact exhibit's authored strength was tuned to retain a useful connected remainder. `focused/write.log` and `focused/read.log` pass in distinct processes.
- `game-audio.log`: the extended game fixture launched the project startup lab instead of the registered game scene, then dereferenced a game-only agent. Corrected the argument to the ordinary `Scenes/game.judas` startup path. `game-corrected.log` then exposed missing audio-listener authoring: spatial voices intentionally require a listener. Added an ordinary active-view listener in the new game scene. `focused-followup/game.log` passes both actual audio obstruction states plus ray and motor/navigation passage.
- `focused-followup/cookbook.log`: the new cookbook accessed rigid descendants in the short interval after resource readiness and before physical publication. The API correctly returned null part entities and rejected loading. The example now waits for the ordinary part entity before submitting a load. No runtime convenience or implicit publication was added.
- `listener-authoring.log`: wrong navigation baker invocation rejected with usage; corrected two-argument project/relative-scene invocation is in `listener-bake-corrected.log`.

Earlier small-fixture, malformed content and TypeScript follow-ups remain in their original development logs. No historical evidence was edited.

- `yield-first.log` exposed an actual lab-authoring weakness: 220 Pa beam strength allowed gravity alone to break the specimen before the intended low/high load demonstration. The unchanged solved-demand path measured ~805 Pa initial traction. The final cook uses 1500 Pa: low loading yields an intact plastic bend; high loading reaches ~1627 Pa and separates it. Uniform and radial application checks pass with that cook.
- `candidate-focused/read.log` correctly restored the exact saved state, but its continuation assertion searched an impact panel whose remaining bonds had already physically failed during subsequent settling. The final cold-save fixture also opens one interface in the supported frame, retaining an alternate connected path, saves both that partial structure and the physically broken/plastically deformed specimens, then physically breaks the restored surviving structure. This is explicit tool cause followed by real solved physical failure, not a fabricated surviving bond or healing.
- `cpu-performance-first.log` used an excessive 10000 kg·m/s impulse and correctly stopped on the inherited M62 element-inversion diagnostic. The bounded measurement uses 30 kg·m/s and completes at both 30 and 90 cells. It does not weaken inversion protection.
- Early performance captures retained only the profiler's last 120 frames; the intact/failing windows were absent. Their zero-sample summaries are preserved and are not measurements. The corrected fixture ends intact/baseline at 120 frames, failing at 160, and settled at 360. Only those performance cases are rerun, after the broad build finishes. Startup is reported from the profiler's separate startup record.
- An early moved-game assertion demanded a permanently clear ray at frame 160, although ordinary moving debris could still cross the opening. The final proof records that early observation and requires real ray clearance, normal audio clearance and motor/navigation passage by the bounded frame-300 deadline. No debris is deleted or collision-disabled to pass it.

Final ownership review added narrow guards before the clean build compiled engine code:
required transient rigid material now rejects M61 capture rather than writing an
unrestorable save; removed soft surfaces reject fabricated current-revision loads
and are excluded from cross-family edge contacts; disabled soft families defer
pending topology like disabled rigid families. The required-state application check
passes, and the removed-material assertions are included in the final core target.
These guards change only the new fracture paths; M62 non-fractured material is unchanged.

### Final content preflight

The final annex walkway hand-authored record omitted required render fields; `review-focused/mechanics.log` preserves the parser failure. Replaced it with the same complete ordinary box record used by the generator and deck. No runtime change or assertion weakening. Final follow-up reruns current scene startup, cold saves, editor and export.

The copied H binding was still named `elastic_mode` while the new lab action expected `bend`. Renamed the ordinary project action consistently to `adopt` in project, JS and content generator. The corrected follow-up proves H-driven adoption and refreshes the final export; no engine behaviour changes.

`review-final/stream.log` then exposed a stale API spelling in the new lab script (`scenes.regionEntity`). The actual documented/runtime API is `scenes.resolveRegionEntity`; corrected both uses without adding a binding. The input-driven follow-up detects this path, unlike the earlier host-only adoption check.

Final package/source verification initially assumed sidecars were byte-identical. Normal M38 export writes an asset-relative `source` field instead of the authoring sidecar’s empty field. The corrected check verifies byte-identical cooked/content payloads plus identical asset ID/type and the expected package-relative metadata source, without modifying exporter or assets. See PACKAGE_CONTENT_VERIFICATION.json.

Final evidence audit removed the application fixture’s `.015` yield override and changed its low load from 10 N to the authored demo’s 5 N. The existing `.06` authored yield threshold remains unchanged. The beam still yields intact and breaks at 120 N (uniform peak 1582.73 Pa, radial 1602.09 Pa); uniform/radial, cold write/read and native moved-package probe all pass. No engine or project change was needed. See authored-beam-final/.
