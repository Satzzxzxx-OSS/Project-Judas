# FTFT9 completion candidate — operator review

**Required approximate-mode behavioural and performance gates pass. No commit,
push or tag was made.** This does not validate exact CFD or integrate the protected
R1 prototypes. Six additional strict diagnostics still fail, as detailed below.

Base: `d2246cd19ad70065d121ffc5d4a4445ebca5d82b`. All implementation remains
uncommitted. [Machine-readable disposition](ACCEPTANCE.json) and
[source/change inventory](CHANGED_FILES.md) accompany the raw executions.

## Repairs and actual execution path

- Numerical particle radius is preferred clearance. Radius-zero physical solid
  geometry owns hard exclusion. Feasible legacy clearance is retained; a blocked
  correction is shortened against the solid union, and incompatible soft
  projections cannot undo each other. Skin correction is excluded from velocity
  reconstruction. No particle deletion, extra force, mass cutoff or floor rule.
- The ordinary rigid step executes once after gravity/external/hydrostatic kicks.
  Fluid collision then consumes copied **resolved** rigid motion segments over
  its cadence. Static geometry retains its original constant-pose arithmetic;
  resets/teleports are discontinuities. Cavity membership uses the same history.
- A wet occupancy envelope could lack spherical motion donors. Existing resolved
  interpolation is retained; only missing motion data extend actual admitted
  column velocities/accelerations. Replacing all weights with column weights
  worsened coarse immersion to 60.8618% and was immediately reverted. That failed
  source and run remain [preserved](../support-consistent-motion/README.md).
- The query-only player does not displace particles. It now samples its interior
  liquid instead of excluding columns as if it were a solid. The unchanged
  rotated free-fall endpoint error fell from 0.061959 m to 0.010067 m. Rigid
  displaced-solid reconstruction remains the default; no gravity substitution.
- The broad run exposed a safe-anchor coding error for a particle crossing a
  thin stationary wall at high speed: the far-side endpoint was exterior but
  its path was invalid. Adding the already-existing physical wall-crossing
  predicate to anchor validity restores the original feasible 0.125 m clearance.
  Physical fixtures/assertions and collision laws were unchanged.

The cumulative candidate also includes approximate exterior hydrostatics/linear
relative drag, authored `fluidCavities`, finite-mass contained-contact exchange,
custom-controller density/swimming, conservative pairwise velocity smoothing,
scene/editor metadata and default 30 Hz particle / 60 Hz rigid integration.
See [METHOD.md](../METHOD.md) for ownership and numerical limits.

## Executed checks

| Evidence | Result |
|---|---|
| [Initial requested gate](../boundary-repair/fourth-gate/results.json) | Closing gap, unchanged `half_025` 731 checks/480 steps, legacy coupling 28 assertions: PASS |
| [Final closing-gap witness](repaired-fluid/closing-gap.log) | One particle; position/velocity `(0,0,0)`; outside floor and body; finite state; PASS |
| [Boundary-contact regression](repaired-fluid/judas_fluid_boundary_contact_tests.log) | Original 8 cases/24 checks: PASS |
| [Sampler](../non-displacing-player/focused/sampler.log) | 93 checks: PASS |
| [Body integration](repaired-fluid/judas_production_fluid_tests.log) | 11 cases/8,021 checks/5,040 ordinary steps; zero mandatory failures, 2 optional failures |
| [Player integration](repaired-fluid/judas_player_fluid_integration_tests.log) | 12 cases/5,661 checks/5,040 ordinary steps; zero mandatory failures, 4 optional failures |
| [Containers](repaired-fluid/judas_fluid_container_integration_tests.log) | 8 cases/7,014 checks/3,124 ordinary steps: PASS |
| [Affected fluid rerun](repaired-fluid/results.json) | All 9 targets plus original closing-gap witness: PASS; fingerprints unchanged during execution |

The body/player/container totals include preparation and separately counted
optional diagnostics. They are not a claim that every diagnostic passed.

### Physical observations

| Requirement | Final observation |
|---|---|
| Half density, spacing .20/.25 m | Independent surface/shape immersion 57.4005% / 57.9036%; unchanged 40–60% gate passes |
| Neutral, spacing .20/.25 m | Four-second drift about -0.00601 / -0.00842 m; <=0.20 m gate passes |
| Twice density, both spacings | Sinks to actual floor; finite state/count retained |
| Rotated half density | 50.5990% independently observed immersion; PASS |
| 80 kg body | Finite; peak speed 2.85039 m/s; observed immersion 52.0723%; no mass-specific rule |
| Zero gravity | Body drift -0.000093 m; player drift 0.00639 m; no buoyant launch |
| Player common free fall | Independent endpoint errors 0.01081 / 0.01007 m, identity/rotated; unchanged <0.05 m gate passes |
| Player swimming, .20/.25 m | Actual input enters and leaves liquid; bounded jump propulsion; particles exactly match distant-player reference |
| Supported containers | All particles retained; settled load relative errors 0.000159–0.00601%, versus <=10% requirement |
| Released containers | Relative drift 0.09545 / 0.05448 m; ballistic errors 0.03148 / 0.0000614 m |

Exterior momentum conservation is **not claimed**. Contained-contact exchange and
independent inelastic witnesses close their declared budgets. Zero-gravity tank
momentum residuals are 5.17e-5 / 1.15e-5 kg·m/s; prescribed-impulse energy agrees
with its work oracle. Free-fall residuals are 0.06861 / 0.00646 kg·m/s. Supported
container logs explicitly include **unmeasured ordinary static support** and
are not closed whole-system momentum evidence. Gravity-driven kinetic energy
is distinguished from unexplained creation. No contact cap or accelerator
fallback occurs in the shipped performance runs.

## Broad regression and source scope

The **one** [broad run](final-validation/results.json) executed 61 production
suites: 60 passed, one boundary suite failed its two fast-wall position checks.
That raw overall FAIL remains unchanged. The corrected boundary suite and all
nine affected fluid targets subsequently passed. The only production/test/script
fingerprint change after the broad run is `src/FluidWorld.cpp`, confined to the
safe-anchor check and explanatory comment. All final-source affected tests and
performance runs retained identical before/after fingerprints.

The broad run also passed:

- Actual nonblocking real-GL async application: 12 cases / 246 assertions.
- Default-async editor Play/Stop and standalone blocking reference startup.
- Current canonical-schema FTFT1 valid/mismatched/legacy runtime/editor loads.
- FTFT3 scene-gravity tests and current rigid/contact/broadphase/lifecycle suites.
- Four 900-step classic/terrain near/far walks; each near/far physical payload
  is identical. These scripted walks remain labelled blocking references.
- Source invariance and protected FTFT1–3 evidence/prototype integrity.

Release build logs have zero compiler warnings. Offscreen EGL environment
warnings remain in raw application logs; no human visual validation is claimed.
M31 stress remains supporting diagnostics, not deterministic acceptance.
No second whole-engine matrix or clean-from-scratch final release gate was run.
Broad execution: 413.415 s; affected rebuild/witnesses/regressions: 277.115 s.

## Final-source shipped performance

[360 rigid ticks / 180 fluid executions per real authored scene](final-performance/run.log).
Measurements include coupling, field preparation and ordinary player sampling.
These are CPU simulation timings, **not renderer or whole-frame timings**.

| Metric | Classic, 125 particles | Terrain, 125 particles |
|---|---:|---:|
| Executed particle median | 7.085 ms | 11.832 ms |
| Executed particle maximum | 9.869 ms | 15.538 ms |
| Executed hybrid median | 7.834 ms | 11.909 ms |
| Hybrid amortized / 60 Hz rigid frame | 4.325 ms | 5.746 ms |
| Whole ordinary fixed-step median | 2.902 ms | 4.161 ms |
| Contact caps / normal accelerator attempts | 0 / 0 | 0 / 0 |

Both <=15 ms median / <=8 ms amortized gates pass. Larger reference pools are
costlier: roughly 15–38 ms particle medians in the body family, outside the
125-particle shipped timing scope. The legacy 200-particle terrain overflow
regression measured 15.814 ms mean/step. Configurable larger quantities have
no universal real-time guarantee; performance does not imply ocean-scale CFD.
The prior 1,500-crate result (14.266 ms, FTFT4 closure) was not rebenchmarked here.

## Remaining limitations and review actions

- Two body diagnostics fail: zero-gravity individual-particle micro-rest and
  sub-particle-sized-body settling. Four player micro-rest diagnostics fail.
  Original predicates/fixtures/tolerances remain present; `--strict-diagnostics`
  returns failure for them. Bulk stationarity and all brief-required physical
  checks remain mandatory. [Scope and before sources](../acceptance-scope/README.md).
- PBF local agitation, coarse surface/air-pocket reconstruction, sampling-order
  sensitivity and nonuniform motion continuity at missing-data seams remain.
- Hard endpoint exclusion is strict for represented geometry. Terrain path
  checks and rigid-to-liquid temporal sampling are finite approximations;
  unresolvable physical overlap fails explicitly rather than deleting liquid.
- Exterior coupling is approximate and nonconservative; contained exchange is
  a finite-iteration contact model, not a pressure-projection CFD solve.
- Positive physical narrowing gaps can still have incompatible preferred-skin
  velocity rows; no general solution of that extra diagnostic is claimed.
- Canonical fingerprint schema 2 includes new authored metadata; old-schema
  saves are rejected, not migrated. Save format/atomic preflight are preserved.

FTFT9 code work requested here is complete as a tested candidate. Operator review
and checkpointing remain. The final clean-build stabilization release gate and
final repository signoff remain separate; no new feature work was started.

## Reproduction

Run from the repository root with installed project dependencies (C++17, CMake,
SDL2/OpenGL/GLM and Python 3). Build uses ordinary Release settings, no fast-math.

```sh
python3 scripts/ftft9_validation.py --output docs/evidence/stabilization/ftft9/review-new --jobs 4
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_production_fluid_performance --output docs/evidence/stabilization/ftft9/review-perf-new
```

Use fresh output directories; preserve previous evidence. The local runners and
exact commands are also recorded alongside each run. Binaries remain in ignored
build output; source snapshots/fingerprints and raw failures persist here.

Proposed commit: `Close FTFT9 with stable approximate fluid body coupling`.
