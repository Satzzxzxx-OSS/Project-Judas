# FTFT4A application trajectory changes

These are explanations and limits, not a replacement for the application regressions and not a new numeric acceptance baseline. Source was unchanged during diagnosis. `analysis.json` contains object mappings, first changed samples, vector maxima, source/binary fingerprints and diagnostic comparisons. `analyze.py` reproduces those calculations. Build/run commands and timings are in `execution.json` and `terrain-runtime-execution.json`; binaries remain in ignored `build/`.

The application CSV prints authoritative positions/velocities to four decimals after every tenth step. “First changed step” below means first **recorded** change; changes smaller than the printed precision may begin earlier. Signed-zero spelling differences are ignored in numeric comparisons.

## Classic: identified mechanism changes

CSV objects follow ordinary Scene dynamic-body creation order. Key mappings: obj0=id14 “Cube (Planet A)”; obj2=id16 “Cube (plank)”; obj4=id18 “Cube (Planet B)”; obj6=id20 spacecraft; obj7/8=id21/22 orbital bodies; obj9/10=id23/24 cups.

The player, spacecraft, orbital bodies, Planet A ball, plank ball, and Cup B are identical at every recorded sample. Cup A positions remain identical; its velocity difference reaches 0.002015 m/s. Planet B ball differs by at most 0.0004124 m. The large changes are the three cubes.

A standalone diagnostic builds the two affected authored support/body pairs with the actual baseline `PhysicsWorld` sources and current compiled engine. Body parameters and gravity are unchanged. Each version matches its corresponding application's first seven recorded rows for both pairs: **168 position/velocity scalar comparisons, zero mismatches**. This establishes that those early differences are reproduced by contact mechanics, without the rest of the scene.

### Plank cube: duplicate edge axis was chosen for a face contact

Both versions have identical state through step41 and at detection on step42. The cube is 0.0393257141113 m above the plank; reach is 0.117174975574 m. Baseline `ClassifyBoxBox` tests:

`bestEdge < 0.95 * bestFace - 0.001`

With both overlaps `-0.0393257141113`, the right side is `-0.0383594284058`: an edge axis identical to the face normal wins merely because the overlaps are negative. The old manifold therefore contains one corner point despite aligned face geometry. The new manifold contains the four actual face corners, keeping all positive separations.

| After step42 | Baseline | FTFT4A |
|---|---:|---:|
| Velocity X | 1.6018542 | -0.00018675 |
| Velocity Y | -3.7607269 | 1.0546821 |
| Velocity Z | 1.6018542 | 0.00067454 |
| Angular X | 4.8147683 | 0.00129764 |
| Angular Z | -4.8147683 | 0.00035960 |

The original off-centre impulse injected substantial spin and horizontal motion into a symmetric vertical face impact. The new four-point response explains the changed trajectory: maximum recorded position-vector difference is 0.068167 m. **The positive-gap rebound still happens early under the unchanged temporal rule; that remains OPEN FTFT4B.** Improved manifold geometry is not evidence that impact timing is correct.

### Planet cubes: separated contact uses a common midpoint

Planet A first differs at actual step28 (recorded step30); both runs have identical detection pose. Baseline sphere/box contact uses the box corner `(13.4290991,13.4290991,-6.4645495)`. The new common contact point is the midpoint of the surface witnesses `(13.4084482,13.4084482,-6.4546084)`. The preserved gap is 0.0616999562 m (old float result 0.0617008209 m).

The midpoint shift is approximately along the contact normal. It **does not** change the normal impulse's torque in exact arithmetic. It changes the lever arm for tangential friction, and friction and normal responses couple in the finite sequential solve. Observed initial spin magnitude per X/Y component changes from 0.9967576 to 0.9418064 rad/s; early recorded application motion is reproduced by both isolated runs. Later maximum position-vector differences reach 0.140107 m for each planet cube. This is a demonstrated contact/anchor-path change; it does not certify the long speculative-impact trajectory as exact mechanics. Planet B is the scene's translated/mirrored counterpart; its first recorded change is also step30.

## Terrain: bounded observations and remaining attribution limit

Dynamic mappings: obj0=id3 dense block, obj1=id4 Fuel A, obj2=id5 Fuel B, obj3=id6 Far fuel, obj4=id7 spacecraft. First recorded differences are step40 for Far fuel/spacecraft, step50 for dense block/Fuel A/Fuel B, and step60 for the player. Maximum position-vector differences are respectively 0.032380, 0.029207, 0.0001733, 0.091239, 0.090631 and **0.040142 m for the player**. These vector norms differ from the runner's per-coordinate maxima.

A 90-step diagnostic uses the actual loaded terrain Scene, `RuntimeWorld`, `GameSession` and `StepPlayedWorld`, with the original walk's W-at-step30 input. It reproduces the final application's player position at all nine recorded rows (**27 scalar comparisons, zero mismatches**). Ground support at steps40–80 is the terrain, scene id1. An additional read-only forward sweep meets the spacecraft, scene id7, beginning before the player's first recorded divergence and continuing through step80. This exposes a real path by which an already changed spacecraft pose and corrected box query arithmetic can alter player movement while terrain remains its support.

This forward sweep is a post-step observation, not instrumentation of the internal move-and-slide query. We did **not** run controlled ablations separating removal of gamma zero-collapse, local-anchor arithmetic, and changed capsule/box closest-point arithmetic. The approximate terrain contact generator is unchanged, but its contacts now use the altered common solver arithmetic and no global zero-collapse. Those facts explain why byte identity is not an appropriate geometry acceptance requirement; they do not prove which operation accounts for every later centimetre. The existing unchanged terrain/physics, lifecycle, player and application regressions remain the actual acceptance evidence. No terrain oracle accuracy claim is added.

## Diagnostic limits

- The baseline legacy manifold may contain an inactive `hit=false` placeholder. In the diagnostic JSON, actual contacts are identified by `solver_gaps`, not by raw array length.
- The initial runtime diagnostic build lacked the normal GLAD include path; that compile failure is retained. The second build supplied the existing GLAD/SDL include paths and passed.
- No pressure/fluid implementation was changed or researched. Contact changes can propagate into existing coupled scene systems, whose historical limitations remain unchanged.
- No trajectory was substituted into a golden test, no tolerance was widened, and no temporal-impact requirement was marked solved.
