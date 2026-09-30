# Non-displacing player sampling volume

The controller samples immersion but does not exclude particles from its volume.
The shared hydrostatic evaluator nevertheless applied the solid-column occlusion
used by real rigid bodies. That invented a displaced-solid shadow inside a
query-only controller volume, omitting actual liquid donors and making rotated
common free fall disagree with its independent endpoint oracle.

The [pre-fix integrated run](../completion-candidate/narrow-extension/judas_player_fluid_integration_tests.log)
retains the failing rotated endpoint error, 0.0619589128 m against the unchanged
0.05 m requirement. The [before snapshot](before/) and [source fingerprints](before.json)
preserve the source used before the caller-contract repair.

`FluidVolumeGeometry` now explicitly distinguishes `DisplacedSolid` (the default
for real rigid geometry) from `NonDisplacingVolume`. `SamplePlayer()` selects the
latter. Quadrature, liquid field, gravity, acceleration estimate and force
equations are shared; only the fictitious solid-column occlusion is omitted.
The choice is a caller geometry contract, not an inferred mass, motion, density
or object-name classification. Real rigid solids retain their occlusion.

## Focused executed evidence

From the repository root, the [recorded runner](run.py) was executed once:

```sh
python3 docs/evidence/stabilization/ftft9/non-displacing-player/run.py
```

It built the real sampler/player targets, passed all 93 sampler checks, then
selected the two unchanged free-fall fixtures through their ordinary integration
path. [Exact commands, exits and before/after fingerprints](focused/results.json)
show source unchanged during the run. The player result is **498 checks / 240
ordinary steps / zero failures**; [raw log](focused/player-fall.log) and
[machine-readable observations](focused/fixtures/results.json) are retained.

| Fixture | Independent endpoint error | Synchronized liquid/player drift |
|---|---:|---:|
| Identity common fall | 0.0108085144 m | 0.0437962264 m |
| Rotated common fall | 0.0100666173 m | 0.0445996709 m |

Both satisfy their existing 0.05 m endpoint tolerance. Fixture geometry,
assertions and oracle arithmetic were not retuned; `--fall-only` only selects
these existing cases. This focused result does **not** claim the complete player
family or final FTFT9 regression has passed. The full candidate is being checked
separately under `completion-candidate/final-validation/`.
