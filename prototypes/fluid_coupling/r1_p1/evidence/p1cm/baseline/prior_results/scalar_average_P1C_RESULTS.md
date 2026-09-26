# R1-P1-C isolated COSMIC results

**Status: FAILED / INCOMPLETE.** The aligned-block case produces a material out-of-range fraction during its first Eq. 15 corrector. The prototype must not be used as a basis for moving-solid or coupled-pressure work until this is resolved.

## Scope actually run

Only the isolated liquid/gas path was built and run. The prototype was compiled with the repository's CMake target (`-O2`, C++17). No production Judas file was touched. No P1-D/P1-E or moving-body case was run. The test runner now stops at the first failed transport fixture; the slotted, reversal, and rotation fixtures did not run.

## Results

| Case | Grid / timestep | Result |
|---|---|---|
| Constant gas, `alpha=0` | 12×9, 3 steps, uniform prescribed velocity, gas inlet | Remained exactly zero; volume error 0; runtime `0.000245 s` |
| Constant liquid, `alpha=1` | 12×9, 3 steps, uniform prescribed velocity, liquid inlet | Remained exactly one; volume error 0; runtime `0.000184 s` |
| Aligned rectangular block | 24×24, `dt=0.0833333`, max directional CFL 0.34 | Failed at first Eq. 15 update; runtime `0.002117 s`; candidate min alpha `-0.001313257421407199`, max `1.0145` |
| Oblique rectangle diagnostic from the earlier batch | 64×64, `dt≈0.06667`, max directional CFL 0.299 | Eq. 14 x predictor exceeded the configured numerical-roundoff diagnostic tolerance; not accepted as a completed test |

The rectangular block starts with liquid volume `0.06`. The first strict excursion outside `[0,1]` is at the Eq. 14 x predictor, cell `(4,7)`, `C=-3.9690473130349346e-15`, a roundoff-scale excursion that is recorded and not changed. The first material failure is the Eq. 15 final update below. Its volume residual was `-1.39e-17`, so global flux conservation closed while local boundedness failed. The material-failure cell is `(i,j)=(4,6)`, initially empty; the analytically translated rectangle also has zero liquid area in that cell after the step.

At that cell the actual Eq. 15 inputs were:

```
Cn = 0
Qx = 0.0068750000000010747
Qy = 0
x-left flux  = 0
x-right flux = 1.1935763888890755e-05
 y-bottom flux = 0
 y-top flux    = -9.6558030878365897e-06
cell area = (1/24)^2
Cn+1 = Cn - [(FxR-FxL)+(FyT-FyB)]/cell_area
     = -0.001313257421407199
```

The x outflux is larger than the incoming y flux, leaving negative liquid area in the cell. The first bad operation is therefore the Eq. 15 cross-predicted final flux divergence, not geometric advection in either Eq. 14 predictor. No alpha clamp was applied.

The three stage rows for this first step are:

| Stage | Min / max alpha | Strictly outside `[0,1]` | Beyond `256*epsilon` | Max face flux (area) | Update residual | L1 geometry error |
|---|---:|---:|---:|---:|---:|---:|
| Eq. 14 x predictor | `-3.97e-15 / 1` | 4 | 0 | `5.90278e-4` | `-2.78e-17` | `7.27438e-4` |
| Eq. 14 y predictor | `0 / 1` | 4 | 0 | `3.81944e-4` | `1.39e-17` | `6.54251e-4` |
| Eq. 15 final candidate | `-1.31326e-3 / 1.0145` | 9 | 4 | `5.90278e-4` | `-1.39e-17` | `1.28216e-3` |

For the same initial state, the preserved sequential Weymouth–Yue comparison path gave `Cn+1=0` at that cell (its global min/max were `-3.97e-15` and `1.0000000000000184`, roundoff-scale excursions). Its maximum cellwise difference from the COSMIC output was `0.0267887191`. This is differential evidence only; it does not establish that the former method is the desired method.

## Refinement and instrumentation

An earlier diagnostic batch reached the first timestep at resolutions 24², 48², and 96²:

| Grid | `dt` / max CFL | First failing operation | First material excursion | Runtime |
|---|---|---|---|---:|
| 24² | `0.0833333` / `0.34` | Eq. 15 final update | cell `(4,6)`: `-0.0013132574` | `0.002075 s` |
| 48² | `0.05` / `0.408` | Eq. 14 y predictor | cell `(17,27)`: `1.0000000000000668` | `0.000931 s` |
| 96² | `0.025` / `0.408` | Eq. 14 x predictor | cell `(20,32)`: `1.000000000000115` | `0.002403 s` |

At 48² and 96² the over-tolerance excursions are small (`6.68e-14` and `1.15e-13` against the diagnostic threshold `256*epsilon`). They are reported, not altered. None of the three resolutions produced a valid completed translation. The end-of-run L1 values printed by that earlier runner compared unchanged initial arrays with final-time geometry after a step was rejected; they are not valid transport errors and are omitted as convergence evidence. The current runner stops after the first failed aligned-block resolution, so it will not proceed to the later resolutions or fixtures.

`results/p1c_trace.csv` records, for each executed predictor/final stage, volume before/after, min/max alpha, maximum absolute face flux, strict out-of-range cell count, beyond-tolerance count, update residual, L1 geometric error, and first out-of-range / first beyond-tolerance cell. A small pressure residual is not applicable because this test has no pressure solve. Kinetic-energy change is not applicable because no velocity/momentum field is advected by this mass-only transport prototype.

Because the basic aligned translation failed on its first corrector, no refinement-convergence claim is made, and the slotted, reversal, and rotation fixtures remain unrun. P1-C is not green; do not resume solid occupancy, triple-interface reconstruction, cut-cell pressure, or fluid/body coupling.
