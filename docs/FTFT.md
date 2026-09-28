# FTFT remediation ledger

This is a **partial FTFT4A geometry candidate checkpoint**, including the
completed bounded cache allocation/lifetime pass. It is not full FTFT4 or
production-performance acceptance. Starting baseline: `3404388` (FTFT3).
Protected FTFT1–3 work and fluid prototypes are unchanged. FTFT4B-1 now
records authorized current-engine defect measurements only; timing repairs
require separate authorization. No further optimization is included.

| ID | Status | Evidence | Changes | Tests | Remaining issue | Fixing commit |
|---|---|---|---|---|---|---|
| FTFT1 | Accepted and checkpointed | [Pre-fix reproduction and final evidence](evidence/ftft1/README.md): changed baseline accepted; later invalid record partially mutated world | Save v2; canonical authored-scene SHA-256 schema 1; complete preflight shared with mutators; strict parsing; legacy/mismatch protection | Fingerprint, persistence, full lifecycle/scene/project suites; actual runtime/editor valid and rejected loads | Strict compatibility only; no migration, asset-content/build identity, or crash-safe file replacement | `23f5447` |
| FTFT2 | Accepted and checkpointed | [Real-GL reproduction and validation](evidence/ftft2/README.md): final demand release at CpuReady still uploaded unused data | Cancel CpuReady through existing generation guard; actual operation tracing, application-loop automation, GPU readback; repair two weak job-test checks | 12 deterministic real-GL cases; 35 production suites; default-async editor and blocking runtime reference; M31 stress reused | Project closure still synchronously drains; no decoder interruption or human visual-validation claim | `0d38ddb` |
| FTFT3 | Accepted and checkpointed | [Pre-fix scene witness and final evidence](evidence/ftft3/README.md): IsWorldDown erased small authored transverse acceleration only at 9.81f | Select Faithful only for an exactly identical computed vector; no angular snapping | 136 scenes, 272 original/roundtrip constructions, 12,095 checks; 36 production suites; FTFT1/2 and classic/terrain near/far regressions pass | Existing float representation/integration error; fixed origin only; no new live-rebase or player-motion claim | `3404388` |
| FTFT4A / FTFT4A-P | Geometry correctness: tested candidate checkpoint; bounded allocation/lifetime pass: complete; PARTIAL acceptance only | [Geometry](evidence/ftft4/GEOMETRY.md), [performance evidence](evidence/ftft4/performance/RESULTS.md), [storage evidence](evidence/ftft4/allocation/RESULTS.md), [checkpoint scope](evidence/ftft4/CHECKPOINT.md) | Pair-local geometry, preserved signed gaps/local anchors, conservative bounds, keyed caches, reused storage | 823 oracle cases; 13,327 cache + 921 storage checks; sanitizers; 40 production suites; FTFT1–3; seven interleaved crate/mixed comparisons | Production performance regression: OPEN. Allocation-pass gains are small/mixed, not a broad speedup. Exhaustive-oracle wall time is separate from production physics cost. FTFT4B timing: OPEN. Documented unsupported/extreme cases unchanged | This partial checkpoint |
| FTFT4B / FTFT4B-1 | Verification complete; defects reproduced; repair OPEN | [Current-engine budgets, traces and source map](evidence/ftft4b/baseline/RESULTS.md): +2.82038 J original energy witness; stopping short, premature rebound and missed chain impact | Diagnostic target/runner only; production unchanged | 36/36 energy failures, momentum budgets close; 13/45 original temporal checks fail; 244 actual angular partition cases; eight existing suites and geometry oracle pass | Coupled-impact energy and timing repair require separate specification/review; angular subdivision changes endpoints; no general rotational TOI evidence | — |
| FTFT5 | Queued; not authorized | Fixed origin exists; live rebasing does not | — | Not run for this item | Verify actual guarantees; separately specify live rebasing | — |
| FTFT6 | Queued; not authorized | Independent editor/project/lifecycle integration gaps | — | Not run for this item | Complete that item’s behavioral coverage; 12-bit body-handle generation wraps, so indefinite stale-handle immunity is not established | — |
| FTFT7 | Queued; not authorized | Orbital/spacecraft/frame validation gaps | — | Not run for this item | Independent physical budgets | — |
| FTFT8 | Queued; not authorized | Combustion budget validation incomplete | — | Not run for this item | Finite-fuel/thermal accounting | — |
| FTFT9 | Queued; not authorized | Production fluid coupling limitation; prerequisites only | — | Protected P1-C/P1-C-M/P1-PF unchanged, not rerun | Separately reviewed moving-geometry specification | — |

FTFT1 does not change authored scene data or delta settling tolerances. The
near/far lifecycle regression still compares byte-identical physical payloads;
its compatibility records now correctly differ for different authored origins.
No milestone claim is upgraded by this persistence fix.
