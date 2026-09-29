# FTFT6 — editor/project/lifecycle integration

**CLOSED.** Current engine paths pass; asset identity defects were reproduced
and repaired. No lifecycle, persistence or simulation mechanism was rewritten.

## Reproduced defect and repair

The actual `AssetDatabase` accepted sibling-prefix destinations such as
`Assets_backup`, overwrote missing/corrupt destination metadata on import/move,
and allowed Track to replace an existing identity. A legitimate filename
beginning with `..` was incorrectly refused. Dangling metadata links were also
vulnerable to overwrite. The extended independent fixture ran against the
original implementation: **17 cases, 67 checks, 54 failures, exit 1**.
Raw failures and the original implementation are preserved in
`development-assets/`; they are not acceptance evidence.

The fix uses normalized path **components** for lexical containment and checks
metadata occupancy, including dangling links, before file mutation. Refusals
leave file bytes, link targets and database records unchanged. Existing
metadata must be resolved explicitly, never replaced by a new random identity.
This does not claim filesystem sandboxing through ancestor symlinks, concurrent
filesystem transactions or crash-safe two-file replacement.

## Executed final evidence

`validation/results.json`: **PASS, 88.151 seconds**, matching before/after
source fingerprints; protected historical evidence and prototypes unchanged.

| Path | Executed result |
|---|---|
| Real editor tiny-project workflow | 49 checks; 1,905 ordinary frames; 36 fixed steps; PASS |
| Same project through normal standalone Application | 19 checks; actual async resources and GPU mesh readback; PASS |
| Shipped tiny-game standalone | 18 checks; PASS |
| Asset integrity | 17 cases / 67 checks; zero failures |
| Mixed lifecycle/persistence | 76 checks / 121 fixed steps; zero failures |
| Current production suites | 46 suites; all exit 0 |
| FTFT2 actual async application | 12 deterministic cases / 246 assertions; zero failures |
| FTFT1 runtime/editor persistence smoke | Valid load, legacy and mismatch refusal; PASS |
| Existing editor Play/Stop + blocking runtime reference | PASS; blocking run is not async evidence |
| Build | Release; no warning lines in build/validation logs |

The editor creates a conventional project, authors bodies and a real OBJ asset,
renames/moves it without changing identity, observes missing-asset failure,
reports duplicate metadata, repairs/rescans, edits/undoes/redoes/deletes,
saves/reopens a changed startup scene, and runs the ordinary Play loop.
Full/Coarse/Dormant, Active/Unloaded/Destroyed, moving-state reconstruction,
runtime creation/destruction and saved replay are checked through real methods.
Stop restores exact authored data; incompatible same-name Play is refused
without changing authored data or existing save bytes.

The standalone observer proves **matching authored startup and the shared
application/frame path**, not identical editor/standalone trajectories. It
explicitly disables persisted deltas for that comparison; replay is tested by
the editor restart, lifecycle fixture and FTFT1 shared loader. The mixed
lifecycle fixture verifies actual physics slot reuse, stale-handle rejection,
monotonic IDs, door/switch deltas and bit-identical failed-load snapshots.
Fidelity is reselected at reconstruction rather than persisted as scene data.

## Measured cost and limits

The generated tiny project's standalone run took **0.115 s**, including startup
and shutdown. Its 31 measured application CPU frame submissions (before buffer
swap) have median **0.274 ms**, range **0.226–2.592 ms** under offscreen software
GL. This is not a GPU/display latency or broad engine performance claim.
Editor workflow wall time was **1.367 s**, including authoring and real IO.
Detailed timings, all commands, binary hashes and raw output are retained.

Automation invokes ordinary authoring requests programmatically; no human
mouse/visual acceptance is claimed. Existing finite handle-generation limits,
coarse-simulation approximations and synchronous project draining remain.
No gravity/contact/fluid algorithm changed. Research prototypes are untouched.
The earlier editor development crash was a new automation callback placed after
ImGui frame completion; it was moved before `ImGui::Render`. Its log/backtrace
and diagnosis are retained separately and are not a production defect claim.

## Changed implementation and validation

- `src/AssetDatabase.cpp`: containment and identity-preserving refusal.
- `src/editor/EditorApplication.cpp/.h`: opt-in observer/driver in existing loop.
- `tests/AssetIntegrityTests.cpp`, `LifecycleIntegrationTests.cpp`,
  `ProjectApplicationTests.cpp`: actual implementation integration checks.
- `CMakeLists.txt`, `scripts/ftft6_validation.py`: normal build targets/runner.
- `docs/FTFT.md`, `docs/ARCHITECTURE.md`, this evidence directory: scope/results.

Reproduction command and scope are in [README.md](README.md). Final acceptance
is `validation/`; development records remain explicitly historical.
