# FTFT4 closure — practical impact timing

**CLOSED under the authorized game-engine approximation policy.** This does not certify a general simultaneous-impact material law. The robust FTFT4A primitive geometry, signed gaps, local anchors, persistent solver, FTFT1–3 evidence and fluid prototypes remain intact. No milestone tag is created.

## Executed correctness

- New actual-engine impact suite: **89 cases / 3,197 checks**, zero failures. Includes transformed isolated impacts, induced chains, mixed/simultaneous contacts, high-speed thin obstacles, cached support, out-and-back player queries, event cap and unaffected rotation.
- Original unchanged temporal oracle: **45/45**; extended ordinary-frame temporal cases: **240/240**. Inelastic positive-gap bodies reach contact and remain there; restitution 0.5 uses the physical impact time.
- Original actual-engine energy witness: **85.5625 → 84.5748460367322 J**, change **−0.9876539632678 J**; linear-momentum residual **2.98023223877e−7 kg·m/s**. All **36** creation-order/axis/boost variants meet unchanged energy/momentum bounds.
- Reusable impact helper: **46 cases / 267 checks**. This is supporting evidence; engine-path evidence is listed separately above.
- Independent geometry oracle: **823 cases / 887 primitive pairs / 1,245 contacts / 1,646 bounds**, zero unresolved predicates. Existing invalid-input, 21 player sweeps, rigid-contact, lifecycle, cache/storage, broadphase and terrain/project suites pass.
- Full regression: **43 production suites**, **12 real-GL async cases / 246 assertions**, editor Play/Stop, standalone blocking reference, FTFT1 valid/legacy/mismatch smoke checks, and FTFT3 **136 scenes / 272 constructions / 32,640 fixed steps / 12,095 checks** pass.
- Four **900-step** classic/terrain near/far harnesses pass; near/far physical payloads remain byte-identical. No human visual validation is claimed. Build logs contain no compiler warnings.

## Numerical policies and diagnostics

Isolated material closing impacts retain restitution and Coulomb friction. Connected/mixed/induced/persistent-support events are inelastic. Low-speed capture uses the existing 0.5 m/s threshold. Event energy is checked with spin and prescribed-boundary work; a failed tentative response is restored and retried without restitution or impact friction. More than 16 events captures connected contacts inelastically while consuming the remaining time.

The 89-case suite executed 113 impact events: **0 energy-safety fallbacks**, **1 deliberate event-cap fallback**, **1 rotating-graze sampling fallback**, **0 sampling caps**. Its largest sampling interval was **0.000731197 s**. These counts describe that suite, not uninstrumented historical fixtures.

Conservative advancement is bounded at 64 iterations. Rotating grazes and uncertified proximity advances use pair-local, feature/speed-dependent actual-contact samples and bisection. Very short contact intervals between samples may be missed; this finite-resolution CCD limitation is explicit. Terrain keeps its approximate geometry and strict boundary convention. A proximity result alone never permits an impulse. See [METHOD.md](METHOD.md).

## Seven-run whole-workload comparison

Baseline: committed `c43af4c`. Seven interleaved sequential baseline/current trials; identical Release flags and fixtures; all cache preparation inside `world.Step`. **420 crate steps + 6,720 mixed steps** executed. First step, construction, teardown and oracle are included in separate raw measurements.

| Metric (median across seven trials) | c43 baseline | Current |
|---|---:|---:|
| 1,500-crate whole step | 14.105757 ms | 14.265518 ms |
| Steady whole step | 14.088593 ms | 14.194573 ms |
| First step | 17.322158 ms | 19.253990 ms |
| All 30 steps, total | 440.310286 ms | 442.528571 ms |
| Construction | 2.870910 ms | 2.900664 ms |
| Teardown | 0.322539 ms | 0.436969 ms |
| Exhaustive oracle/instrumentation | 2859.458903 ms | 2958.844592 ms |
| Whole test fixture | 3308.552886 ms | 3413.096637 ms |

Crate gates pass: **14.265518 ms ≤15 ms**, ratio **1.011326 ≤1.10**. Settled crate steps allocate no heap storage after preparation. The old approximately 4 ms measurement used less-robust geometry; it is not the current baseline. The multi-second exhaustive-oracle cost is not production physics time.

| 32-body moving workload, mean step median | c43 baseline | Current |
|---|---:|---:|
| rotating_boxes | 0.293783 ms | 0.841129 ms |
| offset_compounds | 1.226847 ms | 6.279427 ms |
| near_parallel_and_exact_touch | 0.221571 ms | 0.223767 ms |
| moving_static_support | 0.532336 ms | 2.312100 ms |

**Moving CCD costs are material and are not a speedup.** Offset compounds cost about 5.1× the old endpoint/speculative path in this workload; moving supports about 4.3×. New event searches and changed collision trajectories add work. These measured costs remain a known performance limitation despite passing the stated resting-crate acceptance gates. No trajectory equivalence is claimed after the timing repair.

## Reproduction and preserved failures

- Passing records: `correctness7/`, `performance3/`, `regressions2/`; commands, raw exits, timing samples and source fingerprints are included.
- `stabilization-source.zip` contains the matching 238 source/test/script files and manifest. Binaries stay in ignored build output.
- Earlier failed candidate directories remain unchanged. `correctness4/` records grazing advancement exhaustion; `regressions/` records the terrain-equality integration abort; `performance/` failed its absolute 15 ms gate.
- `correctness5/` contains a new test-oracle error, explained with raw represented geometry in `development-persistent-oracle/`: after drift the spheres are genuinely separated by 2.98e−8 m, so their next floor impacts are isolated elastic impacts. The correction preserved all physical inputs and tolerances.
- Historical `docs/evidence/ftft4b/baseline/` physical failures and `implementation/` rejected PLUS/Poisson contradiction are preserved, including nonzero witness exits.

## Source scope

Production changes are limited to PhysicsWorld event orchestration/queries/diagnostics and the new ImpactSolver/RigidMotion helpers. ContactSolver, primitive geometry, force/quaternion integrator, persistence, resources and fluid research were not rewritten. CMake wires new diagnostics/tests; scripts and documentation record the executed evidence. ResetBody remains discontinuous; the motion ledger is transient and never persisted.
