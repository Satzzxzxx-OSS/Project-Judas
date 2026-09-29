# FTFT8 — CLOSED: combustion and thermal accounting

## Reproduced defects and repairs

The preserved pre-fix source and actual-engine outputs show:

- A valid 0.5 K body with no heat input became 1 K: **150 J created**.
- Stiff radiative cooling hid explicit overshoot behind that same floor:
  1500 K / 1 J/K became 1 K despite reported loss 4784.3789 J, leaving
  **3285.3789 J unexplained**. Stiff reservoir heating overshot its only
  300 K source to 33433.324 K.
- Two ordinary serialized scenes without an atmosphere skipped the entire
  thermal step: no vacuum radiation and zero heater absorption despite
  active input. **3 failures / 41 checks / exit 1** are preserved.

`CombustionWorld` now limits stiff passive conductance using one reciprocal
edge scale and removes the post-temperature floor. Actual transferred power
is reported, with explicit limiting diagnostics. `Simulation` invokes that
same mechanism with a null/vacuum atmosphere for ordinary non-planetary scenes.
The finite-fuel reaction and physical carrier velocities are unchanged.
[The method](METHOD.md) states the convex-update argument and approximation.

## Final executed gate

`validation/results.json`: **PASS, 110.830 s**, source fingerprints unchanged,
protected paths unchanged, and no build warnings. One reproducible command:

```
python3 scripts/ftft8_validation.py --output docs/evidence/stabilization/ftft8/new-validation
```

The output directory must be new. Required C++/SDL/OpenGL/GLM dependencies and
Release build arrangement are the existing project setup; real GL integration
uses the offscreen software context. No human visual validation is claimed.

| Coverage | Executed result |
|---|---|
| Independent combined thermal accounting | 16 cases / 1,826 steps / 20,856 checks; PASS |
| Serialized scene / ordinary fixed-step thermal integration | 16 cases / 2,692 steps / 29,619 checks; PASS |
| Current production suites | 51 suites; all PASS |
| Actual async application | 12 cases / 246 assertions; PASS |
| FTFT1 runtime/editor persistence | PASS |
| FTFT3 construction tests through suite discovery | PASS |
| Classic/terrain near/far | Four 900-step walks; identical near/far physical payloads |

The integration source/fixtures/assertions were byte-identical before and
after the repair. Existing combustion tests also pass unchanged. Tests cover
fuel exhaustion, absent oxidizer, below-ignition reaction, extinction on
travel into vacuum, heater input, chemical/product heat partition, reciprocal
pair exchange, open-reservoir exchange, rotation/translation/common boost,
registration order and zero thermal impulse to the rigid carrier.

### Raw budgets and numerical scope

- Moderate combined case: chemical **4500.000156 J**, gas **1199.999976 J**,
  absorbed heater **121.665116 J**, environment **-6313.816537 J**, stored
  change **-2892.123413 J**. Residual **0.0278287 J**, within the accumulated
  binary32 temperature/power representation budget (25.4669 J worst-case sum).
  Maximum independent power relative error **4.29e-7**. All moderate cases
  report scale 1 and zero withheld exchange; their prior outputs are identical.
- Zero-input 0.5 K remains **0.5 K; 0 J unexplained**.
- Stiff vacuum cooling: **1500 -> 750 K**, budget residual **-2.60e-5 J**.
- Stiff reservoir heating: **100 -> 199.999985 K**, staying below 300 K.
- Stiff unequal-capacity pair: **1500/100 -> 800/107 K**, first-step closed
  energy conserved. Three-body and reversed-registration cases also pass.
- Actual vacuum scene: **300 -> 299.979583740 K**; heater scene absorbs
  **716.197265625 W**, reaches **300.059173584 K**. Independent predictions
  differ by at most **9.46e-6 K**; fuel remains unchanged with no oxidizer.
- Runtime integration worst per-step storage residual **0.00458333 J**;
  chemical diagnostic residual **3.12844e-5 J**, within declared float budgets.

Final focused process times: accounting 0.016 s, scene integration 0.064 s,
existing combustion 0.064 s (including setup). Existing 10,000-step thermal
microbenchmark after the repair: **0.00027 ms/step** in the recorded focused
run. This tiny thermal measurement is not an engine frame/GPU benchmark.

## Limits and protected work

Atmosphere/oxygen is a prescribed **open reservoir**, not finite evolved gas.
Smoke/flame is presentation, no CFD mass claim. Radiation lacks occlusion;
coating heat capacity and carrier mass/inertia are fixed. Stiff exchange is
stable and budgeted but its uncapped physical rate is not accurately resolved.
Extreme source power can exceed float range and raises an explicit error;
no silent floor repairs it. No phase change, chemistry expansion or weather.

Production files changed: `CombustionWorld.cpp/.h`, `Simulation.cpp`.
New compiled tests, CMake targets, validation runner, README/architecture/ledger
notes and this evidence complete the checkpoint. FTFT1–3 historical evidence,
contact mechanics and all protected fluid prototypes remain unchanged. FTFT9
has not been implemented by this checkpoint.
