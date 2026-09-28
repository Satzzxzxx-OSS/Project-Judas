# FTFT4A executed results

**Disposition:** geometry and declared regression checks pass; operator review pending. The approximately fivefold crate cost increase is a material performance regression, not performance acceptance. Full FTFT4 remains OPEN.

## Executed checks

| Check | Result |
|---|---|
| Complete production suites | 38/38 PASS (baseline 36/36) |
| FTFT2 actual nonblocking application | 12 cases / 246 checks PASS; worker decode and main-thread GPU ownership checked |
| Editor Play/Stop and standalone | Real async editor and separately labelled blocking reference PASS |
| FTFT1 persistence runtime/editor smoke | Six processes PASS; strict compatible/changed/legacy behavior retained |
| FTFT3 authored gravity | 136 scenes / 272 constructions / 32,640 steps / 12,095 checks PASS |
| Independent contact geometry | 823 fixtures / 887 primitive pairs / 1,245 contacts PASS |
| Exact conservative shape bounds | 1,646 checks PASS |
| Raw actual AABB tree replay | 400/400 query expectations PASS before and after |
| Independent player sweep geometry | 21/21 PASS; existing 300 exhaustive comparisons also PASS |
| First-step lifecycle/persistence | 30 fixtures / 90 ordinary fixed steps PASS |
| Sphere convenience contact frames | Six contacts PASS; lifecycle suite total 1,050 checks |
| Primitive oracle robust predicates | 16,354 predicates / 64 exact fallbacks / 0 unresolved |
| Original rigid suite with diagnostic wrapper | 2,230,499 predicates / 2,082 fallbacks / 0 unresolved, all original assertions PASS |
| Adjacent-binary64 arithmetic | 2,000,026 equivalence checks PASS |
| Classic/terrain near/far walks | Four 900-step runs PASS; each near/far pair byte-identical |
| FTFT4B temporal witnesses | FAIL as expected; explicitly OPEN, not FTFT4A passes |
| Invalid/range diagnostics | Six invalid quaternions explicit; FLT_MAX range case unresolved, FLT_MIN resolved |
| Protected FTFT1–3 / fluid checkpoints | Source/evidence fingerprints unchanged |

Final aggregate run: **326.106 seconds**, including rebuilding all affected targets and seven complete broadphase suite repetitions. Independent Python+engine geometry validation: 3.355 seconds. Initial baseline aggregate: 63.522 seconds. These totals have different rebuild work and are not performance comparisons.

## Same-machine performance

Same Release build configuration and unchanged `TestScaleStatistics`: 1,500 crates plus one floor, 30 fixed steps per run, final-step timer, seven runs in each phase. No timing assertion was widened. Each final run reports 1,500 candidate/colliding pairs, 6,000 points, tree height 11, 45,000 predicates, zero exact fallbacks and zero unresolved cases.

| Timer | Baseline median [range], ms | Final median [range], ms |
|---|---:|---:|
| Whole step | 4.196 [4.029–4.745] | 20.979 [20.295–27.958] |
| Broadphase + proxy refresh | 0.392 [0.379–0.506] | 2.877 [2.596–4.960] |
| Narrowphase + contact assembly | 1.321 [1.174–1.625] | 12.226 [11.895–15.987] |
| Solver | 2.398 [2.239–2.554] | 3.172 [2.841–4.685] |

Median whole-step ratio: **5.000×**. Timer categories omit the initial force/proxy-reach portion from the component sum. The total is authoritative.

Instrumented profiling confirms interval arithmetic dominates: multiplication approximately 26.5%, addition 11.3%, bound construction 9.4%, SAT 5.8% of sampled CPU time. Instrumentation and symbol folding limit precise attribution; these percentages are diagnostic, not timings or fallback counts. A semantics-equivalent outward rounding optimization and cached unchanged anchor rotations reduced the initial 28.460 ms observation, but did not remove the regression. No sleeping, additional damping, thresholds or test fast path was introduced.

## Trace comparisons

Neither scene was silently rebaselined. `final/results.json` records every changed numeric column. Classic player and near/far equality remain unchanged; dynamic contact trajectories differ. Terrain player maximum sampled position-vector difference is about 0.040142 m, and some rigid objects differ by about 0.09124 m. Source-linked interpretation and the limits of causal attribution are retained in [trajectory/README.md](trajectory/README.md). Rebuilt baseline/current engine diagnostics reproduce the classic cube changes: duplicate edge-axis preference formerly generated one corner point for a face impact; the repair generates four face points. Separated sphere/box midpoint anchors also change friction lever arms. Terrain propagation is observed, but every later centimetre is not independently attributed. Existing rigid momentum, restitution, friction, stack, energy, terrain and fluid regression assertions all remain passing. This does not assert identical long-horizon trajectories or exact terrain contact geometry.

## Changed files

- `CMakeLists.txt`
- `docs/ARCHITECTURE.md`
- `docs/FTFT.md`
- `src/ContactSolver.cpp`
- `src/ContactSolver.h`
- `src/Contacts.cpp`
- `src/Contacts.h`
- `src/Narrowphase.cpp`
- `src/Narrowphase.h`
- `src/PhysicsWorld.cpp`
- `src/PhysicsWorld.h`
- `tests/BroadphaseTests.cpp`
- `src/ContactGeometryInternal.h`
- `tests/ContactGeometryProbe.cpp`
- `tests/ContactLifecycleTests.cpp`
- `scripts/ftft4_validation.py`
- `docs/evidence/ftft4/`: mechanism, original supplied reference/adapters, independent oracle/fixtures, source snapshots, executed logs, JSON results and fingerprints. No compiled binaries or caches in this evidence tree.

## Limits and next action

No human visual validation; protected fluid prototypes not rebuilt or rerun. Terrain sampling, sampled player sweeps, float input/pose representation and raw non-unit rigid inertia retain their stated limits. The finite FLT_MAX quaternion witness remains a reported arithmetic-range failure. No universal floating-point guarantee follows from this finite test set.

Impact timing, zero-gravity pre-contact stopping, premature bounce and impact-chain candidate regeneration remain OPEN FTFT4B. No impact target/step-order repair is included. No FTFT5 or fluid implementation is authorized.

Proposed commit message: `Fix FTFT4A pair-local contact geometry and preserve signed separation`.

No commit, push or tag was made. HEAD remains `3404388f1af48419505c802fc11dbf8a0b577f38`. The implementation and evidence are intentionally uncommitted for operator review. Final status and checked source fingerprints are in `final-audit.json`.
