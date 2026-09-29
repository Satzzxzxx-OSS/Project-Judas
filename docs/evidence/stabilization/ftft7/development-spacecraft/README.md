# FTFT7 spacecraft/session development verification

`run1`: **21 fixtures, 228 checks, 1,164 fixed steps, zero failures**. The run took 0.009889 s; this is tiny collision-free test execution, not a renderer/frame benchmark. The source was unchanged after this passing run. The final FTFT7 runner separately rebuilds/runs this suite with all engine regressions.

## Executed path

Every fixture serializes a regular `Scene`, reloads it using `LoadSceneFromString`, constructs `RuntimeWorld` and `GameSession`, and advances `StepPlayedWorld`. Window automation supplies key state/edge requests only. There is no replacement force, gravity, attachment or integrator loop. Serialized `.judas` files and every vector observation are in `run1/`. GL/rendering is outside this focused suite.

The cases cover identity, arbitrary rotation, and rotated/translated local coordinates with a fixed `(1e12,-2e12,3e12)` absolute origin. This does not implement or test live rebasing.

- 80 kg and 240 kg vehicles: body-local force gives `delta-v=1200*dt/m`; removal of input preserves coast. A 450 N m pitch torque gives `delta-omega=450*dt/I_x`, with cuboid inertia independently calculated from full authored dimensions.
- SAS enable leaves orientation and angular velocity bit-exact. Its first update at the captured attitude matches documented `-10*omega` damping; following real steps continuously return to target despite a held turn key. Translation is unchanged and SAS continues after normal pilot release. This checks the declared controller/integrator approximation, not exact asymmetric-body torque-free dynamics.
- Local vehicle plus dynamic celestial source receives Newtonian pair force and the local field. The other body's reciprocal velocity and total momentum after subtracting the external local-field impulse are independently checked. The local field's radius excludes the source, avoiding cancellation of a huge external source impulse.
- Celestial vehicle plus static point source receives that field, excluding local gameplay gravity. A static authored source is deliberately one-way and is not presented as a closed momentum system.
- Attached player follows moving/rotating craft. Ordinary frame-input release inherits `v + omega cross (player-position - ship-position)`, independently calculated in double from authoritative pre-release state. Both zero gravity and actual gravity clearance run; clearance visibly changes the position, and using that new offset would give a detectably wrong inherited velocity. Neither release changes ship velocity/spin.

## Numerical gates

References use double arithmetic from represented authored inputs, not production force/attachment helpers. Absolute vector tolerances are 2e-6 for force/control velocity, 2e-7 for one-step gravity, 2e-18 for the massive source's tiny reciprocal velocity, and 2e-5 kg m/s for combined momentum. Pose/release tolerances are 3e-6 to 5e-6 m or m/s, allowing binary32 rotation and translated coordinate arithmetic. SAS long-term spin gate is 2e-4 rad/s. Raw vectors, errors and thresholds are recorded; no tolerances were changed after running.

`RUN_METADATA.json` records commands, build flags, execution count and measured peak errors. `source_fingerprints.json` identifies tested sources. Binaries are excluded. `initial-unconfigured-target.log` is a build-wiring failure before CMake regeneration, not a physical witness or failed test execution.
