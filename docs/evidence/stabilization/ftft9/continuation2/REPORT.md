# FTFT9 bounded continuation and handoff

**FTFT9 remains INCOMPLETE. No passing checkpoint, push or final readiness claim.**
The operator's two-attempt limit was reached for settling diagnosis and geometric
escape. Both outcomes were reported during work. No third fluid algorithm or
reconstruction-tuning experiment was started.

## What was completed

- Retained the exterior-column-consistent bulk velocity/acceleration sampler and
  the auxiliary contact accelerator's finite/cone/residual safeguards. The
  authorized cached-support reuse remains restricted to the main solver's
  already-cached contacts. Main ContactSolver/impact policy is unchanged.
- Corrected a reference-pool initial-state defect: all body density/spacing cases
  now share a deeper pool, and independent raw-particle surface measurements must
  establish fully submerged neutral release. The existing physical tolerances
  and strict rest prerequisite are unchanged. See [the targeted witness](../body-preparation/attempt1/README.md).
- Preserved and reverted the exact Poly6-gradient candidate after it made the
  ordinary player pool materially worse. The previous Spiky surrogate remains.
- Added passive diagnostics testing pressure-velocity cancellation. Maximum
  measured error was 1.67638e-6 m/s; it is too small to explain the observed
  material agitation. No corrective damping was added.
- Preserved and removed both rejected endpoint-escape candidates after raw
  integrated particle snapshots exposed reference-pool leakage. The previous
  sequential solid projection and original boundary assertions are restored.
- Fixed an ordinary project-test oracle error: compare actual disk bytes before
  and after Play, rather than comparing old authored text with newly expanded
  canonical defaults. The separate canonical authored-memory invariant remains.

## Single combined validation: rejected geometry candidate

Command executed once:

```sh
flock build/ftft9-build.lock python3 scripts/ftft9_validation.py \
  --output docs/evidence/stabilization/ftft9/continuation2/validation --jobs 4
```

The runner took 366.993 seconds and exited **1**. Its source remained unchanged
during execution. [Machine-readable inventory](SUMMARY.json),
[raw runner record](validation/results.json), and all raw nonzero outputs remain.

| Coverage | Executed result |
|---|---|
| Production suites | 61; 58 pass, 3 fail: project disk oracle, body fluid, player fluid |
| Body integration | 11 cases / 8,021 checks / 5,040 fixed steps; 9 failed assertions |
| Player integration | 12 cases / 5,661 checks / 5,040 fixed steps; 5 failed assertions |
| Dynamic cavities/containers | 8 cases / 7,014 checks; pass |
| Actual asynchronous application | 12 cases / 246 assertions; pass |
| Editor Play/Stop and standalone reference | Pass; standalone reference is explicitly blocking |
| FTFT1 current-schema application smoke | Six runtime/editor invocations; pass |
| Classic/terrain near/far | Four 900-step runs; pass, byte-identical physical near/far payloads |

**This run used the rejected geometry candidate. It is not validation of the
restored final fluid code.** Passing nonfluid checks, container fixtures and
isolated boundary checks do not erase its failed fluid acceptance.

### Exact remaining failures

Body: five strict preparation failures (.25 half/neutral/dense, zero gravity and
the small-body case), plus four physical assertions. Coarse half-density observed
immersion was 0.617688 outside [0.40,0.60]. The small-body sampled/observed immersion
was 0.380042/0.398312, and final vertical velocity was -1.26326 m/s outside the
settling limit. Neutral .20/.25 displacement passed (-0.006799/-0.009809 m).

Player: four microrest failures (all three .20 density cases and rotated .25
half-density), plus rotated common-free-fall independent endpoint error
0.0619589 m versus the unchanged 0.05 m limit. Neutral .25 displacement
**passed at -0.00683844 m**. The -0.643903 m value belongs to the dense sinking
case, not neutral. Both production-spacing swim-through fixtures passed and
their reference particle fields remained identical.

Dense .20 raw snapshots contained two particles at y=-71.3106/-69.5166 m with
vertical velocities -37.3302/-36.8334 m/s. It preserved particle identity/count,
yet lost spatial containment. The existing suite lacked that spatial assertion;
the CSV audit detected the failure. Three saved snapshots cannot locate the first
crossing. This is reference-pool leakage, not a failed dynamic-cup result.

## Two-attempt disposition

1. **Settling:** the exact-gradient attempt passed its independent derivative
   check but worsened the pool; it was reverted. The second, passive arithmetic
   diagnosis disproved cancellation as the material cause. No further model
   change was justified by these observations.
2. **Geometry:** the first endpoint exclusion passed 12 cases / 60 checks; the
   stationary-wall crossing revision passed 14 / 78. Integrated leakage rejected
   the candidate. Its source and unaccepted fixtures are preserved in
   [rejected-integrated](../particle-batch/geometric-escape/rejected-integrated/README.md).
   The original eight-case/24-check boundary suite is intact. No accepted
   assertion or fixture was removed/weakened.

The retained earlier projection still has known coarse opposing-wall/cap
limitations. Removing the worse experiment does not solve that issue.

## Limited post-rollback validation and timing

[Reproducible commands, logs and matching source fingerprints](final-targeted/results.json)
cover one Release build of the affected targets, one project test, one restored
boundary suite and one shipped fluid workload. Build had no warnings; project
passed with zero failures; boundary passed **8 cases / 24 checks**. Source hashes
were unchanged through this run. No full repeated body/player/regression campaign
was performed after the rollback.

Reproduce these limited checks with:

```sh
flock build/ftft9-build.lock python3 \
  docs/evidence/stabilization/ftft9/continuation2/final-targeted/run.py
```

The command writes its targeted logs/results at that path; preserve an existing
record before intentionally rerunning. It does not establish FTFT9 acceptance.

| Current restored workload | Classic | Terrain |
|---|---:|---:|
| Particles | 125 | 125 |
| Ordinary rigid / executed fluid steps | 360 / 180 | 360 / 180 |
| Executed particle-step median | 3.176545 ms | 3.642064 ms |
| Executed particle-step maximum | 4.796004 ms | 5.145594 ms |
| Executed hybrid median | 3.859505 ms | 3.702098 ms |
| Hybrid amortized per 60 Hz frame | 2.314343 ms | 1.917584 ms |
| Whole fixed-step median | 2.427797 ms | 2.768194 ms |
| Auxiliary contact caps | 0 | 0 |

These are physics/coupling costs, not rendered editor frame timings. Both shipped
performance gates pass. Larger deep acceptance pools are substantially more
expensive; their timings are not the shipped 125-particle benchmark. No fresh
crate or editor-frame performance campaign was run.

## Exact continuation state

- Main and its local origin/main tracking ref remain
  `d2246cd19ad70065d121ffc5d4a4445ebca5d82b` (FTFT8 closure). The remote was not
  refetched or mutated in this continuation. FTFT4–8 closure hashes are listed in
  [STABILIZATION_STATUS.md](../../../../STABILIZATION_STATUS.md).
- Nothing is staged, committed, pushed or tagged. FTFT9 production/test/docs work
  remains intentionally uncommitted. [Git status](GIT_STATUS.txt) and
  [full tracked working diff](WORKING_DIFF.patch) preserve that state.
- [STATE.json](STATE.json) and [SOURCE_SHA256.json](SOURCE_SHA256.json) record the
  restored source; its 267-file [source archive](restored-source.zip) includes new
  untracked code, no binaries or research prototypes. Its hashes match the
  post-rollback targeted run. Documentation changed subsequently; source did not.
- Protected prototypes and historical FTFT1–3 evidence have no tracked or
  untracked changes. Main ContactSolver, WorldState, ResourceManager/JobSystem
  and Faithful/RadicalGravity files are unchanged. Canonical schema 2 extensions
  for authored fluid metadata are pending; FTFT1 atomic preflight remains intact
  and passed the combined current-schema smoke.

## What remains before a checkpoint

Resolve the strict rest/small-body settling failures, coarse wall feasibility and
rotated player free-fall discrepancy without changing the physical oracles. The
two bounded investigations did not supply a successful replacement policy.
Any continued repair must begin from the restored source and preserved failures,
not from the rejected candidate or a supposed green FTFT9 checkpoint.

Then run the affected acceptance fixtures and the final clean Release/full engine
gate once against the final source. Only a passing FTFT9/final gate authorizes the
closure commit/push and readiness statement. **Number 5's documentation/handoff
is prepared; its passing checkpoint is blocked.**
