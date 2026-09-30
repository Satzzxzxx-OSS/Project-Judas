# FTFT9 actual player/particle-field integration

## Acceptance correction — strict prerequisite restored

The later automatic approval review rejected demoting the original mean-particle-speed `<0.05 m/s` acceptance assertion to diagnostic-only status under the no-weakened-assertions rule. The source now restores that exact hard assertion; bulk velocity and centroid observations remain additive. `run3` retains its raw exit 0 and all measurements as historical output, but it is **NOT complete acceptance evidence**: its reported zero physical failures omitted the original prerequisite, which failed in six pool cases. The prior acceptance wording below is superseded by this notice. All original logs, sources, fingerprints and failed strict observations remain unchanged. No further test reinterpretation, threshold change or production change was made in this restoration. A genuine mechanism repair and a fresh complete run are required.

## Executed result

`run3/results.json` and `run3.log`: **12 fixtures, 5,654 physical acceptance checks, zero failures, 5,040 ordinary played-world steps, exit 0.** This is automated numerical evidence, not human visual validation.

The seven stricter particle micro-rest diagnostics have **six failures** and are explicitly not claimed to pass. The original `<0.05 m/s` mean-particle-speed predicate is unchanged. The approximate production model permits local PBF agitation; actual reference-pool acceptance independently verifies mass-weighted bulk speed <0.05 m/s and centroid travel <0.05 m over the final one second of a fixed four-second warmup. See `ORACLE_CORRECTION.md` for the authorized distinction and original failed runs/source snapshots. No particle state, player state after spawn, velocity or fraction is repaired to satisfy a check.

## Reproduce

```
flock build/ftft9-build.lock cmake --build build --target judas_player_fluid_integration_tests -j2
build/judas_player_fluid_integration_tests --output <new-persistent-evidence-directory>
```

The fixture serializes an ordinary scene, reloads it, builds `RuntimeWorld`, starts `GameSession`, and invokes `StepPlayedWorld`. It supplies movement/jump through `Window` actions. It never sets a fluid sample or runs an alternative fluid/player loop. Density pools use one fixed deeper geometry for every density, with a distant player during ordinary warmup and the normal release/spawn position/velocity API for initial placement at the settled liquid centroid. All 4-second density response thresholds are unchanged from the initial test.

## Measurements

| Fixture | Physical failures | World-Y displacement (m) | Final immersion | Maximum speed (m/s) | Wall time (s) |
|---|---:|---:|---:|---:|---:|
| density-20-500 | 0 | 1.58119 | 0.60425025 | 3.265646 | 17.740 |
| density-20-1000 | 0 | 0.0038592815 | 1 | 0.01719383 | 17.907 |
| density-20-2000 | 0 | -0.72589296 | 1 | 1.6818526 | 18.140 |
| density-25-500 | 0 | 1.4893196 | 0.59758621 | 3.1831825 | 8.807 |
| density-25-1000 | 0 | 0.0028041601 | 1 | 0.0062783998 | 8.839 |
| density-25-2000 | 0 | -0.63789511 | 1 | 1.582422 | 8.725 |
| rotated-density-25-500 | 0 | 1.1475784 | 0.60317773 | 3.1893415 | 8.833 |
| common-fall-25-1000 | 0 | -19.759169 | 0.61359519 | 19.596567 | 1.197 |
| rotated-fall-25-1000 | 0 | -15.170635 | 0.6090979 | 19.596735 | 1.290 |
| zero-g-25-500 | 0 | 0 | 0 | 4.2465676e-06 | 2.202 |
| swim-through-20-950 | 0 | 0.027222633 | 0 | 4.000001 | 9.541 |
| swim-through-25-950 | 0 | 0.023863912 | 0 | 4.0000014 | 4.674 |

The rotated-density world-Y displacement is not its gravity-frame rise: the latter is 1.49270666 m and is recorded in `run3.log`. The identity light-body rises are 1.58119/1.48932 m. Neutral-player four-second drift is 0.0038593/0.0028042 m at 0.20/0.25 m particle spacing. Dense players sink 0.72589/0.63790 m toward the physical floor. No explosion/speed gate fails.

The freefall comparison uses the last **executed** fluid frame so 30 Hz particle holds are not compared to a later 60 Hz player endpoint: relative drift 0.030542/0.030625 m. The independent semi-implicit constant-gravity endpoint errors at two seconds are 0.024334/0.024257 m over approximately 20 m of fall. Raw final held-particle mismatches are still reported (approximately 0.357 m).

Both traversal cases enter and leave the actual field. Existing step-120 jump inputs occur at immersion 0.935879/0.671023 and add 0.042763/0.030933 m/s vertical velocity, within the configured 4 m/s² propulsion budget. Every particle's position, velocity and acceleration exactly equals a separate ordinary-world control with the player distant: swimming does not use particle contact pushing. Zero gravity shows no invented buoyancy.

At 0.20 m spacing the pool's bulk speed is 0.00437013 m/s and one-second centroid travel 0.00080711 m, while local mean particle speed is 0.0871707 m/s. At 0.25 m those bulk quantities are 0.00207822 m/s and 0.00095726 m, while local agitation is 0.0595609 m/s. The distinction is measured, not inferred from the player's outcome.

These development timings include scene creation, warmup, measured steps, CSV output and, for traversal, an entire second reference world. They are **not** a production fluid-step performance benchmark; use the final serial workload evidence for performance acceptance.

## Evidence lineage

- `run1`: initial unrelaxed/shallow pool and mismatched freefall timestamps, seven failures; preserved unchanged.
- `smoke2`: settled-reference fixture exposed genuine multi-wall/exact-touch particle containment defects, four failures; fixed by the parent production collision changes, not by changing the player.
- `smoke3`/`run2`: containment repaired; strict local micro-rest fails at 0.20 m while all physical response checks pass.
- `smoke4`: passive eight-second agitation measurement confirms the stricter continuum-rest premise is unsupported.
- `run3`: explicit approximate bulk-rest acceptance plus unchanged physical response gates and additional real-input jump checks. Strict micro-rest failures remain visible and separately counted.

`run3-source-fingerprints.json` records the source and linked binary/archive hashes before execution. `run3-PlayerFluidIntegrationTests.cpp` is the exact tested source. No binaries are stored in this evidence tree.

## Complementary player evidence

`../player-development/` contains 89 direct controller checks /2,475 steps (manufactured field component tests only), plus a 2,160-step dry-locomotion trace compiled against the previous and new PlayerController implementations. Both dry traces are byte-identical, SHA256 `d54395c1555df78857b6bc1400d815c768d16d20d1920959e29dd925f288ba16`.

The production controller remains custom, uses the oriented collision capsule's bounding-box immersion approximation, applies density/effective-gravity buoyancy and stable relative drag, and receives no particle push. Exact exterior fluid/player momentum conservation is not claimed.
