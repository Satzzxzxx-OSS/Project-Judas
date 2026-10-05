# M59 human backtracking follow-up

## Preserved reproduction

The operator left the demo running at the failed return journey. Its captured window shows gallery-0/4/5 stuck unloading, no pending preparation, 593 KiB retained records and the project travel gate. The process log reports stale target joint handles. The screenshot/log remain in this directory.

A focused actual Application/JS/CharacterMotor test holds W to the end, S back to bootstrap, then repeats. It uses normal spatial-interest policy, fixed movement, real asynchronous reads and normal physics; it never teleports the character. It releases initial manual demand through F10. Original run reached one round trip, then stuck on the next trip at z=-59.9638: 6 checks / 3 failures. CSV/log preserved.

## Concrete causes and generic corrections

1. Streaming recount() repeatedly called ObjectProperties() on every retained definition to estimate bytes, often several times per frame. Retained navigation/component records grew to about 500 KiB; accounting alone exhausted the 2 ms dispatch budget, starving installation/teardown indefinitely. Cache the exact same estimate when a snapshot/adoption record changes. Budget thresholds, limits and state retention are unchanged.
2. RemoveRegionObject called PhysicsWorld::DestroyBody with an invalid handle for body-less components. That sentinel also matches the empty world endpoint of world-anchored joints, deleting unrelated native hinges. Teardown now calls DestroyBody only for a valid body. PhysicsWorld, collision/constraint solver and safe-handle semantics are unchanged.
3. Keeping hinges alive exposed incorrectly double-transformed world endpoints: region placement changed both joint owner transform and its local endpoint, then normal joint registration transformed again. Preserve joint-local authored anchors/frames, apply ordinary owner transform once, and inverse-transform native world endpoint settings into local authored space during snapshot capture. Oblique/translated and suspended/reloaded endpoint checks added.

## Final proof

- 71 streaming unit checks, zero failures: including real world-anchored hinge survival after body-less component teardown and oblique endpoint placement/revisit.
- 48 actual application checks, zero failures, including Q/F7/F9/F10, score, liquid pinning, navigation seam, cancellation and whole-world reload.
- Two complete forward/backward traversals: 1862 frames, 7 checks, zero failures, no player/target script faults, peak retained estimate 508037 bytes. Logical movement through all regions; no teleport substitute.
- Current moved read-only export: 48 application checks, zero failures from /tmp, source/package baseline identical. Package /tmp/m59-backtrack-game-moved: 52 assets / 9 scenes / 53789510 bytes; export 0.177 seconds.
- Narrow Release builds only; no clean build or production-suite repetition. Runtime API unchanged. Temporary joint trace instrumentation removed; WorldScripts.cpp matches pre-follow-up fingerprint.

See TIMING.json for retained-heavy frames (>=450000 retained bytes): original integration median 5.46875 ms, p95 7.1574 ms; corrected median/p95 measured in final-travel. Captures are focused probes, not isolated graphics benchmarks; some builds/tests overlapped. A snapshot/native unit remains indivisible, and the final run's largest whole integration frame was 11.535451 ms. The 2 ms target remains a dispatch budget, not a hard real-time cap.

## Intermediate evidence

Cached accounting removed the traversal deadlock but exposed remaining joint failures. Diagnostic observations/trace and original logs preserved. One diagnostic build had a local auto-declaration compile error and its command continued with the previous binary; cached.log is explicitly NOT evidence for the correction. cached-corrected contained a temporary RuntimeJoint observer that could synchronize registrations, so final validation removed it. A later application assertion missed a plate because the surviving hinge exposed double-placement of the anchor; the correction fixes the real placement, not shot/game semantics. Final runs above use no temporary joint observer or trace.

## Scope / handoff

Changed implementation: src/WorldStreaming.cpp, src/WorldRegions.cpp. Tests: tests/WorldStreamingTests.cpp, new tests/StreamingRevisitTests.cpp and its CMake target. Current M59 documentation/evidence/fingerprints refreshed. No project/game policy or physics solver changes in this follow-up. Previous broad/performance and genuine failure records remain historical; this current focused follow-up supersedes traversal/hinge behaviour claims.

Corrected desktop reopened from clean state for human re-test. Nothing committed/pushed/tagged. Starting HEAD, operator roadmap bytes/staging, protected FTFT/P1 and historical evidence, accepted fluid and all existing tracked projects remain unchanged. No post-M59 work.
