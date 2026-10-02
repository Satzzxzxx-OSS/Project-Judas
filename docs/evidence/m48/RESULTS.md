# M47 + M48 candidate results

Starting HEAD/main/origin/main remain `4c2ee4631ec36f29160c5c5a9a6b5c6c24039956`.
Nothing committed, pushed or tagged. Human review remains pending.

## M47 boundary

Recorded internally before M48: 16 CPU checks + 6 real Application/JS checks.
See `../m47/internal/` and its source fingerprints for that distinct scope.
Current M47 tests still pass after M48 integration. Composition supports weighted
clips, deterministic interrupted crossfades, ordered masks, explicit-reference
additive TRS and copied/validated external sources. Source and final poses remain
separate. The final eight-instance/32-node/three-contribution composition cost is
25.384937 microseconds per batch (1000 batches); no GPU time is included.

## M48 boundary

Final focused checks: 36 core checks + 4 real Application/JS checks, all pass.
Actual StepPlayedWorld drives gravity, contacts, M45 hinges and pose transfer.
Mapped centre readback residual: 2.56369276e-7 m. Maximum observed hinge angle:
1.20015764 rad against authored +/-1.2 limits (finite-step test allowance .05 rad).
Core test verifies rotated-gravity equivalence, current-pose activation, queries,
impulses, generation safety, body destruction, prefab spawning, return blending,
reset, asset replacement and reconstruction. See raw focused logs.

Complete fixed-step averages (120 steps, three bodies per articulation):
- one articulation: 0.051669 ms;
- five articulations: 0.092123 ms.
These include ordinary simulation/pose work, not just the final physics solver.
They are small-fixture CPU sanity measurements, not universal guarantees.

## Combined gate and narrow late corrections

ONE fresh Release build: zero compiler warnings. 89 production suites passed.
Actual async integration: 12 cases / 246 checks, zero failures. Normal editor
Play/Stop, scene replacement/reload, standalone and moved-package export passed.
Initial gate took 680.129 s. Package: 7,419,103 bytes.

Final review reproduced two cleanup defects in focused/lifecycle-before-fix.log:
a return contribution survived authored reset; previous/recent motion observations
survived mesh asset replacement. Reset now discards the per-instance mixer/source
state; asset replacement clears motion observations. No physics expressions or
solver changes were made in these follow-ups.

Affected checks only were rerun: M46 34+4, M47 16+6, M48 36+4, all pass. The updated
runtime/editor/exporter were relinked and editor Play/Stop + standalone + moved
package repeated narrowly (4.252 s). The 89-suite/async gate was NOT repeated.
`lifecycle-followup/SOURCE_SHA256.json` matches the final engine/test/script/demo
sources, including the late cleanup fixes. `final/` preserves the earlier gate.
The missing judas_runtime target command error is preserved in the focused build
log; Judas's actual targets judas/editor/export were then built successfully.

Original M47 scaled-expectation and M48 demo-string failures remain preserved.
All old FTFT/P1/M33-M46 evidence and research trees are unchanged. No M49 work.

## Limits / human review

See docs/POSE_COMPOSITION.md and docs/RAGDOLLS.md: named/path joint identity,
bounded layers/contributors/mappings, positive uniform rigid mapping scale,
box/sphere bodies, existing passive M45 constraint scope, transient articulation
state, visual return rather than physical recovery, no motors/IK/state machines.
No automated visual/physical acceptance is claimed. Demo:
projects/ragdoll_demo/ragdoll_demo.judasproj; current moved package:
/tmp/Judas_M48_Followup_Package.
