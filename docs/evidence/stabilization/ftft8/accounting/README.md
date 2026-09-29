# FTFT8 independent thermal accounting

## Reproduction

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target judas_thermal_accounting_tests -j4
./build/judas_thermal_accounting_tests --output <fresh-evidence-directory>/csv
```

The compiled test calls actual `CombustionWorld::Step`. It does not simulate a
replacement thermal model. `post-fix/run.json` contains the executed command,
source and executable SHA-256 fingerprints, exit status and wall time.
Binaries stay in ignored build storage. `pre-fix` and `stiff-pre-fix` preserve
failing outputs, nonzero exits, source snapshots and CSV records.

## What the checks establish

- Four 240-step coupled fixtures use unequal heat capacities, finite fuel,
  different retained/gas fractions, reciprocal radiation, finite heater
  interception and an open atmospheric reservoir. Variations reverse
  registration, rigidly rotate/translate geometry, or apply a common gas/body
  velocity boost. No rigid velocity is changed by a thermal step.
- Stored energy is independently computed from `sum(C_i * delta T_i)`;
  chemical release comes from the **actual fuel decrement times material
  heat release**. These state differences are compared against measured
  external heater/environment transfer and product heat leaving for gas.
  Pair exchange must cancel. The separate double-precision gas/profile,
  radiative and interception calculations do not call production gas or
  heat-transfer helpers.
- Four 180-step cases test cold fuel, zero oxidizer, vacuum and extinction
  on cooling with fuel still available. A separate overlapping-view case
  verifies finite total heater delivery.
- Six 24-step stiff cases cover passive cooling/heating, two unequal
  capacities and three unequal capacities, including reversed registration.
  Maximum/minimum temperatures and the state-based energy budget are checked
  independently of the new conductance-scale formula. Stiff reduction must
  be reported; ordinary coefficients must report scale 1 and zero withheld
  exchange. Withheld exchange is not energy delivered to any body.
- A positive 0.5 K inert body with no heat input must remain exactly unchanged.
  This is an identity check on accepted data, not a cryogenic material model.

### Fixed tolerances

The exchange-power comparison uses `2e-5` of the unsigned independently
computed power scale. State budgets allow float temperature rounding:
`2*epsilon*C*max(T_before,T_after)` per body plus `16*epsilon` times unsigned
step energy channels. The stiff budget uses `8*epsilon` times unsigned stored
and exchanged energy scales. `epsilon` is float machine epsilon. Cumulative
bounds are sums of per-step bounds; they are deliberately conservative
engineering allowances, not claims of exact energy conservation. No bounds
were widened after a failure. Temperature covariance permits 0.002 K and fuel
covariance `1e-8 kg`; unchanged-order fixtures match exactly.

## Executed results

| Run | Cases | Thermal steps | Checks | Failures | Exit |
|---|---:|---:|---:|---:|---:|
| Initial pre-fix | 10 | 1,682 | 18,393 | 1 | 1 |
| Stiff pre-fix | 14 | 1,778 | 18,685 | 42 | 1 |
| Corrected solver | 16 | 1,826 | 20,856 | 0 | 0 |

The original 0.5 K case manufactured **150 J**. Original stiff vacuum
cooling manufactured **3285.378895 J** in one step. The unequal-capacity
closed pair manufactured **3285.283447 J**; cold gas heating overshot
100 K to **33433.324 K** against a 300 K reservoir.

After the repair, the same first steps give respectively 0.5 K unchanged,
1500→750 K, pair 1500/100→800/107 K, and 100→199.999985 K. Largest observed
step energy residuals were `2.60e-5 J` for vacuum cooling, `3.81e-4 J` for the
pair, and `0.002889 J` for the three-body case.

The moderate combined fixture remained unchanged: chemical release
4500.0001555 J, solid stored change −2892.1234131 J, gas products
1199.9999761 J, intercepted heater 121.6651160 J, net environment
−6313.8165372 J. The four-second budget residual is **0.0278287 J**;
conservative accumulated float allowance is 25.4669 J. The largest independent
exchange-power discrepancy across covariance fixtures is `4.29e-7` relative.

## Limits

The prescribed atmosphere supplies oxidizer and receives/releases heat as an
open reservoir. There is no finite gas mass/energy inventory in this test or
in that model. The material has a fixed exposed-layer heat capacity and a
finite thin fuel coating; rigid carrier mass does not decrease with fuel.
The stiff conductance reduction is an explicit real-time approximation,
not time-accurate integration at arbitrary coefficients. External heater
and chemical energy are not subject to passive-temperature extrema. General
chemistry, CFD products, smoke mass conservation and phase changes are outside
this accounting. Actual scene/application integration is covered separately
by the FTFT8 runner and its integration target.
