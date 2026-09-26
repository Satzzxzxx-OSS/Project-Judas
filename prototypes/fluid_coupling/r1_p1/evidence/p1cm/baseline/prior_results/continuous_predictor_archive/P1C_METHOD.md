# P1-C: Judas Symmetric Geometric Transport (JSGT)

The authoritative method description is [METHOD.md](METHOD.md). JSGT intentionally replaces scalar-average/re-PLIC final fluxes with the mean of separately reconstructed geometric fluxes. It is not claimed to faithfully reproduce COSMIC Eq. 15.

| Operation | Actual code |
|---|---|
| Original liquid geometry R(Cn) | `reconstruct(g,g.c)` in `jsgtStep()` |
| Full x/y predictors from Cn | `predictor()`; original face fluxes and continuous-C directional divergence |
| Predictor PLIC geometry | `rcx=reconstruct(g,cx)`, `rcy=reconstruct(g,cy)` |
| Transverse geometry fluxes | `directionalFluxes(g,rcy,Axis::X,...)`, `directionalFluxes(g,rcx,Axis::Y,...)` |
| Final flux composition | `finalX=.5*(fluxXn+crossX)`, `finalY=.5*(fluxYn+crossY)` |
| One conservative update | `finalUpdate(g,g.c,finalX,finalY,...)` |
| Uniform-flow composition identity | Diagnostic comparison with two complete directional sweep orders; never used as an update |
| First rejected predictor trace | `printPredictorFailure()` prints phase fluxes, face velocities, conservative contribution and dilatation |
| Independent shape expectations | `transport_oracles.hpp` analytical integration, no solver geometry calls |
| Per-stage and per-run evidence | `results/jsgt_trace.csv`, `results/jsgt_summary.csv` |

The prior scalar-average experiment and its failure remain historical evidence in `results/p1c_trace.csv`. The sequential Weymouth–Yue function remains source-only comparison material and is never acceptance evidence. All gates use one JSGT implementation with explicit prescribed velocity arrays; there are no test-name/shape branches in the transport operator.

For a full reproduction from the repository root:

```
cmake -S prototypes/fluid_coupling/r1_p1 -B prototypes/fluid_coupling/r1_p1/.build-jsgt
cmake --build prototypes/fluid_coupling/r1_p1/.build-jsgt -j2
cd prototypes/fluid_coupling/r1_p1
.build-jsgt/judas_r1_p1
```

The runner stops at the first failed gate. No final-time geometric error is reported for an aborted trajectory.
