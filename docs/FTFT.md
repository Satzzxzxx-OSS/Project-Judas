# FTFT remediation ledger

Only **FTFT1** is authorized in this change. Starting clean checkpoint:
`587759e93add89f32774cbc51fe47878fab127ec`. No fluid prototype changes.

| ID | Status | Evidence | Changes | Tests | Remaining issue | Fixing commit |
|---|---|---|---|---|---|---|
| FTFT1 | Implemented; awaiting operator review | [Pre-fix reproduction and final evidence](evidence/ftft1/README.md): changed baseline accepted; later invalid record partially mutated world | Save v2; canonical authored-scene SHA-256 schema 1; complete preflight shared with mutators; strict parsing; legacy/mismatch protection | Fingerprint, persistence, full lifecycle/scene/project suites; actual runtime/editor valid and rejected loads | Strict compatibility only; no migration, asset-content/build identity, or crash-safe file replacement | Not committed |
| FTFT2 | Queued; not authorized | Async application validation gap | — | Not run for this item | Real async application coverage | — |
| FTFT3 | Queued; not authorized | Gravity construction concern, not established defect | — | Not run for this item | Verify real scene construction | — |
| FTFT4 | Queued; not authorized | Speculative-contact guarantee and hover defect | — | Not run for this item | Reviewed numerical specification and independent checks | — |
| FTFT5 | Queued; not authorized | Fixed origin exists; live rebasing does not | — | Not run for this item | Verify actual guarantees; separately specify live rebasing | — |
| FTFT6 | Queued; not authorized | Independent editor/project/lifecycle integration gaps | — | Not run for this item | Complete that item’s behavioral coverage | — |
| FTFT7 | Queued; not authorized | Orbital/spacecraft/frame validation gaps | — | Not run for this item | Independent physical budgets | — |
| FTFT8 | Queued; not authorized | Combustion budget validation incomplete | — | Not run for this item | Finite-fuel/thermal accounting | — |
| FTFT9 | Queued; not authorized | Production fluid coupling limitation; prerequisites only | — | Protected P1-C/P1-C-M/P1-PF unchanged, not rerun | Separately reviewed moving-geometry specification | — |

FTFT1 does not change authored scene data or delta settling tolerances. The
near/far lifecycle regression still compares byte-identical physical payloads;
its compatibility records now correctly differ for different authored origins.
No milestone claim is upgraded by this persistence fix.
