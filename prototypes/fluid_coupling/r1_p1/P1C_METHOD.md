> Preserved P1-C fraction checkpoint: the active executable now also runs P1-C-M. See `P1CM_METHOD.md` and `P1CM_RESULTS.md`. Statements below about executable scope describe the earlier fraction-only checkpoint. Its full original source/report is preserved in `evidence/p1cm/baseline/`.

# P1-C code mapping and reproduction

See `METHOD.md` for the complete declared method and its limitations. **JSGT now uses frozen WY directional correction; van der Eijk-Wellens Eq. 14 is not retained unchanged.**

| Operation | Code |
|---|---|
| Freeze Cn>0.5 once | `jsgtStep()` local const `frozenColour` |
| Sufficient cumulative CFL gate | `jsgtStep()`; harness `cumulativeCourant()` with timestep halving |
| Original geometry and face fluxes | `reconstruct()`, `directionalFluxes()`, `geometricFaceFlux()` |
| Frozen-WY predictor | `predictor(base,frozenColour,flux,axis,dt,...)` |
| Separate X/Y reconstruction | `rcx`, `rcy` in `jsgtStep()` |
| Final symmetric flux average | `finalX`, `finalY` |
| Accepted update | `finalUpdate()` then `g.c.swap(next)` after all gates |
| Both full frozen-WY orders | `xy`, `yx`; same frozenColour; diagnostic only |
| Identity and divergence defect | `orderAverageError`, `divergenceAdjustedIdentityError` |
| Bound/budget failures | `inspect()`, `acceptStage()`, `printPredictorFailure()` |
| Per-step evidence | `writeStep()` / `results/jsgt_steps.csv` |
| Independent expectations | `transport_oracles.hpp`, `nonlinear_transport_oracle.hpp` |
| Fixture/refinement and boundary oracles | `runCase()` and `run()` |

`wySequentialReference()` is retained historical comparison code and remains unused by the acceptance runner. The actual diagnostic complete orders are constructed within the active JSGT step. No fixture name controls the transport laws; names and optional cell probes only label evidence.

From the prototype directory:

```sh
cmake -S . -B .build-jsgt
cmake --build .build-jsgt -j2
.build-jsgt/judas_r1_p1 > results/jsgt_stdout.txt 2>&1
```

The build directory is disposable. Sources and evidence remain outside the repository's root build directory. The runner stops at its first failed gate. Each rerun replaces current CSV/log files; prior continuous-C results remain archived. `results/jsgt_run_metadata.json` fingerprints the reported run and must be refreshed when reporting a later build.
