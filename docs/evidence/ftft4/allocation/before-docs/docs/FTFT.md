# FTFT remediation ledger

This change implements **FTFT4A and its FTFT4A-P performance pass only**,
starting from `3404388f1af48419505c802fc11dbf8a0b577f38` (accepted FTFT3).
FTFT4A-P preserves and optimizes the uncommitted geometry repair; it does
not reset that repair to HEAD.
Protected FTFT1–3 work and fluid prototypes remain unchanged. FTFT4B and
FTFT5–9 are not authorized by this geometry pass.

| ID | Status | Evidence | Changes | Tests | Remaining issue | Fixing commit |
|---|---|---|---|---|---|---|
| FTFT1 | Accepted and checkpointed | [Pre-fix reproduction and final evidence](evidence/ftft1/README.md): changed baseline accepted; later invalid record partially mutated world | Save v2; canonical authored-scene SHA-256 schema 1; complete preflight shared with mutators; strict parsing; legacy/mismatch protection | Fingerprint, persistence, full lifecycle/scene/project suites; actual runtime/editor valid and rejected loads | Strict compatibility only; no migration, asset-content/build identity, or crash-safe file replacement | `23f5447` |
| FTFT2 | Accepted and checkpointed | [Real-GL reproduction and validation](evidence/ftft2/README.md): final demand release at CpuReady still uploaded unused data | Cancel CpuReady through existing generation guard; actual operation tracing, application-loop automation, GPU readback; repair two weak job-test checks | 12 deterministic real-GL cases; 35 production suites; default-async editor and blocking runtime reference; M31 stress reused | Project closure still synchronously drains; no decoder interruption or human visual-validation claim | `0d38ddb` |
| FTFT3 | Accepted and checkpointed | [Pre-fix scene witness and final evidence](evidence/ftft3/README.md): IsWorldDown erased small authored transverse acceleration only at 9.81f | Select Faithful only for an exactly identical computed vector; no angular snapping | 136 scenes, 272 original/roundtrip constructions, 12,095 checks; 36 production suites; FTFT1/2 and classic/terrain near/far regressions pass | Existing float representation/integration error; fixed origin only; no new live-rebase or player-motion claim | `3404388` |
| FTFT4A / FTFT4A-P | Correctness checks pass; performance review pending | [Geometry mechanism](evidence/ftft4/GEOMETRY.md), [original evidence](evidence/ftft4/README.md), [matched performance and cache evidence](evidence/ftft4/performance/RESULTS.md) | Geometry repair retained; exact-key shape/orientation/bound caches, factored interval SAT, per-body solver phase reuse; unchanged exact fallback and timing law | 823 independent geometry fixtures; 13,327 cache checks; 400 tree queries; 39 production suites; FTFT1–3; seven interleaved three-version crate/mixed trials | Crate median 19.794→12.876 ms (35% reduction), still 3.23× historical 3.984 ms; full test/oracle wall time regresses 5.8%; explicit arithmetic/terrain and FTFT4B timing limits remain | Not committed |
| FTFT4B | OPEN; not authorized | Real engine witnesses retain zero-g pre-contact stopping, premature restitution drift and missed impact-chain timing | No event/timing mechanism changes in FTFT4A | Explicitly failing temporal checks retained | Separately reviewed event integration and remaining-time candidates | — |
| FTFT5 | Queued; not authorized | Fixed origin exists; live rebasing does not | — | Not run for this item | Verify actual guarantees; separately specify live rebasing | — |
| FTFT6 | Queued; not authorized | Independent editor/project/lifecycle integration gaps | — | Not run for this item | Complete that item’s behavioral coverage; 12-bit body-handle generation wraps, so indefinite stale-handle immunity is not established | — |
| FTFT7 | Queued; not authorized | Orbital/spacecraft/frame validation gaps | — | Not run for this item | Independent physical budgets | — |
| FTFT8 | Queued; not authorized | Combustion budget validation incomplete | — | Not run for this item | Finite-fuel/thermal accounting | — |
| FTFT9 | Queued; not authorized | Production fluid coupling limitation; prerequisites only | — | Protected P1-C/P1-C-M/P1-PF unchanged, not rerun | Separately reviewed moving-geometry specification | — |

FTFT1 does not change authored scene data or delta settling tolerances. The
near/far lifecycle regression still compares byte-identical physical payloads;
its compatibility records now correctly differ for different authored origins.
No milestone claim is upgraded by this persistence fix.
