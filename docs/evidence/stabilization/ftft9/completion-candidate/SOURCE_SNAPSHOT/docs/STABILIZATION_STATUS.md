# Judas stabilization status

**FTFT9 TESTED CANDIDATE — operator review/checkpoint and final release gate pending.**

FTFT4–8 are committed closures. The approximate production-fluid implementation
is uncommitted work with passing required behavioural/performance checks.
No milestone tag or FTFT9 checkpoint has been created. See
[the current measured candidate](evidence/stabilization/ftft9/completion-candidate/RESULTS.md)
and [the concise ledger](FTFT.md).

## Verified scope and closure commits

| Item | Guarantee within its documented scope | Closure commit |
|---|---|---|
| FTFT1 | Versioned authored-baseline identity; incompatible saves rejected before world mutation | `23f5447d37fb96ab4c42017d33b5de1fccae997b` |
| FTFT2 | Actual asynchronous IO/decode/upload coverage; CPU-ready cancellation and generation safety | `0d38ddb2babbb8bc51d9500f3fd7721f7f8a1b84` |
| FTFT3 | Scene-authored uniform acceleration preserved without angular snapping | `3404388f1af48419505c802fc11dbf8a0b577f38` |
| FTFT4 | Robust contact geometry; actual-time isolated impacts; stable inelastic coupled-event approximation | `0ebf1d4a7e93b912c8dc59cc663ee729259358a0` |
| FTFT5 | Compact local simulations at large absolute fixed origins; no live rebasing claim | `52911068afcca393e8b0836dbfbb08673a9f1091` |
| FTFT6 | Tested ordinary nonplanetary editor/project/asset/lifecycle paths | `a0ba6d1dbaf60954a08b165e09e60741469ac0f3` |
| FTFT7 | Tested orbital, spacecraft and reference-frame force/motion paths with finite-step error budgets | `6145762bbc295b586846c5dcc29aea49915f5e34` |
| FTFT8 | Finite-fuel combustion and thermal accounting with an explicit atmospheric reservoir | `d2246cd19ad70065d121ffc5d4a4445ebca5d82b` |
| FTFT9 | Tested approximate hydrostatics/drag, declared containers and custom-controller swimming; review/checkpoint pending | — |

FTFT1's strict compatibility policy has no general save migration. The pending
fluid metadata uses canonical fingerprint schema 2 and rejects old-schema saves;
the persistence format and atomic validation mechanism remain unchanged.

## Explicit approximations

- Coupled/ambiguous impacts are inelastic; isolated impacts use authored
  restitution. Low-speed capture, event caps and finite-resolution grazing CCD
  are documented real-time policies. Active compounds/supports can be costly.
- Coordinates use a fixed double absolute origin and local float simulation.
  Travelling indefinitely through an automatically rebasing region is unsupported.
- Orbital/rigid integration has finite timestep and representation error; Coarse
  celestial simulation is contact-free. No exact energy conservation is claimed.
- The atmosphere is a prescribed open reservoir. Combustion uses approximate
  material/thermal properties; smoke is visual, not evolved conservative gas CFD.
- The pending fluid candidate uses PBF motion at an authored cadence, sampled
  hydrostatics/linear drag, declared cavities and controller swimming. Exterior
  fluid/body momentum is deliberately not conserved. Required approximate-mode
  gates pass; local particle agitation and sub-particle-scale equilibrium have
  explicit limitations. This is not a high-fidelity pressure-coupled solver.

## Remaining stabilization work

FTFT9 code work is complete as a candidate. The requested hard-solid/soft-skin
boundary fix passes, as do resolved-motion integration and non-displacing player
sampling. Required body/player/container gates pass. The one broad run passed
60/61 suites; a safe-anchor repair then passed the original boundary suite and
all nine affected fluid suites. That original broad FAIL remains in the evidence;
it is not relabelled. No second full matrix was run.

1. Operator review and FTFT9 checkpoint remain; no commit/push/tag is made here.
2. The final clean-from-scratch Release/stabilization gate and final repository
   signoff remain after review. This continuation used normal Release builds.
3. Six additional diagnostic failures remain visible: individual-particle
   micro-rest and sub-particle-sized-body settling. `--strict-diagnostics` still
   reports failure for them; the brief's physical tolerances were not widened.
4. PBF micro-agitation, filled local columns/internal-air-pocket resolution,
   sampling order, nonuniform missing-data interpolation and finite collision
   sampling are approximation limits. Unresolvable hard overlap fails explicitly.

Default production particle cadence is 30 Hz, rigid physics 60 Hz. Exterior
hydrostatic support owns the body's response; exterior particle collision is
one-way. Contained particles use declared geometry and finite-mass contact
exchange. The query-only player samples actual interior fluid and does not push
particles. Neither exact exterior momentum nor complete energy conservation is
claimed. The protected high-fidelity prototypes remain future research.

## Performance and evidence scope

| Workload | Measured value | Evidence/scope |
|---|---:|---|
| 1,500 resting crates | 14.266 ms median whole physics step | FTFT4 seven-run closure; not remeasured here |
| 32 active offset compounds | 6.279 ms mean-step median | FTFT4 closure; known costly case |
| Classic fluid, 125 particles | 7.085 ms particle median; 9.869 ms maximum | Final source; 360 rigid / 180 particle steps |
| Terrain fluid, 125 particles | 11.832 ms particle median; 15.538 ms maximum | Same run/count |
| Classic hybrid amortized cost | 4.325 ms / 60 Hz rigid frame | Includes coupling/field/player sampling |
| Terrain hybrid amortized cost | 5.746 ms / 60 Hz rigid frame | Includes coupling/field/player sampling |
| Larger body-reference pools | Approximately 15–38 ms particle medians | Stress fixtures, not shipped-125 performance scope |
| Normal editor/runtime frame | Not benchmarked here | Fixed-step CPU timing excludes renderer/frame cost |
| Async application | 12 cases / 246 assertions pass | Actual nonblocking GL application, not a latency benchmark |

[Current evidence](evidence/stabilization/ftft9/completion-candidate/RESULTS.md)
records 11 body, 12 player and 8 container cases, plus original boundary/coupling
regressions. Persistence, actual-GL async, editor Play/Stop, standalone startup,
scene gravity and four 900-step near/far harnesses passed in the broad run.
Source fingerprints distinguish that run from the one-file anchor correction
and final-source affected fluid reruns. Compiler warnings: zero. Human visual
validation: not run. Prior failed candidates and raw outputs remain unchanged.

## Future capabilities

Live origin rebasing, deformable structures, high-fidelity cut-cell fluids,
animation/AI/networking and render-to-texture/portals remain roadmap features.
The P1-C/P1-C-M/P1-PF research prototypes are preserved and are not integrated
into production. See [ROADMAP.md](ROADMAP.md).

**JUDAS STATUS: FTFT9 TESTED CANDIDATE — AWAITING REVIEW AND FINAL STABILIZATION SIGNOFF.**
