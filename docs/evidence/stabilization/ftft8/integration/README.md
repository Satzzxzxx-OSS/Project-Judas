# FTFT8 scene/runtime integration evidence

The same test source was executed before and after the production fixes. `pre-fix-CombustionIntegrationTests.cpp` preserves it byte for byte; source fingerprints and raw nonzero exits are retained.

- Before: **16 fixtures, 29,619 checks, 2,692 fixed steps, three failures**. The separately captured two-scene vacuum witness has three failures among 41 checks; exit 1 is the actual physical failure.
- After: **16 fixtures, 29,619 checks, 2,692 fixed steps, zero failures**, exit 0. Elapsed 0.036962 s is tiny test-world execution, not a rendering benchmark.

## Reproduced defect and repaired path

`StepPlayedWorld` formerly skipped combustion entirely when a scene had no atmosphere. A finite coating initialized at 300 K therefore neither radiated into vacuum nor received the ordinary operator heater. The no-atmosphere scenes use supported body/combustible/player-start components and no planetary object.

After the nullable-atmosphere call-path repair, the same scene cools to 299.979583740 K in one 1/60 s step. The independent Stefan–Boltzmann prediction is recorded in `RUN_METADATA.json`. Pressing the ordinary `UseIgniter` action supplies 716.197265625 W by isotropic interception and reaches 300.059173584 K. Neither case burns fuel: vacuum has zero oxidizer.

## Integrated coverage

Every case serializes its scene, reloads it through `LoadSceneFromString`, constructs `RuntimeWorld` and `GameSession`, and advances the ordinary `StepPlayedWorld` path. Window automation changes only input state. No test registers a substitute thermal body or invokes a replacement thermal loop. Serialized scenes and step-by-step measurements are retained.

The atmospheric cases cover warm finite-fuel exhaustion, zero oxygen despite adequate temperature, cold fuel despite adequate oxygen, and actual rigid-body travel out of the prescribed atmosphere. Identity, arbitrary rotation/local translation, and fixed far absolute origin are separately authored and compared. A real operator heater crosses ignition at step 30; removing input at step 80 permits cooling and extinction at step 130 while fuel remains. A rotated/translated yaw-equivalent heater case uses the actual player look/input path. Both linear and angular carrier velocity stay exactly unchanged throughout all collision-free steps.

## Accounting and scope

Every step records temperature, finite fuel, chemical/product power, external heater, pair/environment exchange and oxygen access. Independent accounting uses actual fuel decrement times 16 MJ/kg, subtracts power delivered to the gas reservoir, adds measured heater/environment transfer, and compares with authored heat capacity times temperature change. Chemical-power and storage errors must fit the declared binary32 measurement/temperature budgets in the test; no thresholds changed after execution. Independent vacuum radiation/interception calculations establish an external oracle for the reproduced path defect. Lower-level FTFT8 accounting tests separately validate exchange formulas and stiff-coefficient safety.

The atmosphere is prescribed and open: incoming oxygen and outgoing product energy do not deplete/evolve it. These fixtures do not claim finite atmospheric oxygen conservation, physical gas/smoke CFD, phase change or changing rigid mass as a thin coating burns. There is no renderer or human visual validation in this suite.

Reproduce with the build/run commands in `RUN_METADATA.json`, choosing a fresh output directory. Binaries/build caches are excluded. `post-fix-fingerprints.json` identifies the final tested sources; the final FTFT8 runner reruns against the full frozen checkpoint.
