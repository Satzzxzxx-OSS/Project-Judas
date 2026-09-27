# FTFT1 evidence

Baseline: clean `main` at `587759e93add89f32774cbc51fe47878fab127ec`.
The protected fluid prototype tree is unchanged. No commit, tag or push is made.

## Reproduce

From the repository root (C++17 compiler, CMake >=3.20, SDL2 and GLM development
packages, Python 3; the application smoke also needs SDL offscreen/Mesa OpenGL):

```sh
python3 docs/evidence/ftft1/run_regressions.py
python3 docs/evidence/ftft1/runtime_smoke/run.py
```

The first command builds the actual shared engine, runtime, editor and five test
executables, then runs the complete relevant suites. The second launches the real
runtime/editor on matching, mismatched and legacy saves. Inspect its new editor
screenshots as described in `runtime_smoke/README.md`; exit zero alone is not a
Play success check. Blocking resources are explicitly selected for these
persistence checks. No FTFT2 async claim is made.

The old defect can independently be reproduced without resetting this checkout:

```sh
python3 docs/evidence/ftft1/reproduce_baseline.py --jobs 4
```

That script archives and fingerprints the exact Git baseline into ignored build
output, builds its engine, and runs the preserved minimal witness. The witness
**must fail two rejection assertions**. It accepted a changed same-name baseline
and replaced x=99 with x=44; another load returned false only after destroying an
earlier entity. See `pre_fix_verified.log` and its metadata (fresh-source build,
232 source hashes). `pre_fix.txt`/`pre_fix_metadata.json` preserve the earlier
existing-library reproduction; the verified reproduction removes that dependency.

## Final results

| Suite | Executed checks | Result | Wall time (s) |
|---|---:|---|---:|
| Canonical authored fingerprint | 132 | PASS | 0.0261 |
| World-state compatibility/atomicity | 302 | PASS | 0.0103 |
| Full lifecycle suite | 78 | PASS | 0.1728 |
| Full scene suite | 76 | PASS | 0.0236 |
| Full project suite | 107 | PASS | 0.1518 |

Total: **695 checks, zero failures**. Final incremental build: 11.02 seconds.
Exact commands, return codes, timings and executable hashes are in
`regressions.json`; each executable has a matching `.log`. `focused_tests.txt`
records the earlier 294-check pass before adding the identity-exhaustion case;
it is not the final count. Current application smoke results and timings live
in `runtime_smoke/results.json`, with screenshots and `visual_review.json`.

## What rejection evidence checks

Tests compare exact observable live-world snapshots before/after failures:
physical states, handles/generations, entity definitions/IDs, lifecycle/fidelity,
reconstruction bookkeeping, counters/version, body counts, and doors/switches.
Malformed file fixtures additionally compare the source bytes unchanged. Failed
parsing leaves its output object unchanged. Asset-backed preflight verifies no
resource demand was initiated. Valid created/destroyed/moved/door/switch and
unloaded-entity deltas still reload; created-then-destroyed identities stay
consumed, including the last allocatable ID boundary.

## Compatibility policy and limits

Save format 2 records canonical fingerprint schema 1 and SHA-256. Identity covers
authored scene data, including IDs/NextId, names, object order, transforms,
components, stable asset references and active settings. Scene comments,
whitespace, numeric spelling and file locations are irrelevant; signed zero is
canonicalized. Runtime mutations do not affect the cached baseline identity.
Different names/order/allocator state are deliberately incompatible too.

Legacy v1 cannot prove compatibility and is rejected, preserved and not
implicitly overwritten. There is no migration. The fingerprint does not cover
external asset bytes, engine revisions or project settings outside the Scene.
It is not authentication. Validation is completed before application on the
main thread; this is not rollback against allocation/process failure or a
crash-safe disk-save transaction. Existing delta settling tolerances are unchanged.

FTFT2–FTFT9 were not executed. Protected P1-C/P1-C-M/P1-PF suites were not rerun
because their sources were not touched. No moving-geometry/fluid work occurred.
