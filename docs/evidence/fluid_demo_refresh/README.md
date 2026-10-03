# Production-fluid regression and current demonstration

Starting committed source: `fc6f44f233e82214c6400c05e695fa71774727db`.
This work does not implement M49 or alter protected historical evidence/research.

## Reproduction and source of the lag

`baseline/operator-preview.judas` preserves the exact ignored preview that was
launched after the operator asked for swimming. It was copied here without
editing the original historical fixture. It contained 1,280 particles at 0.25 m
spacing, five static boundary bodies, no cavities or dynamic bodies, uniform
gravity, 60 Hz rigid stepping and 30 Hz fluid. It was not a registered modern
project. `baseline/phases/` is the authoritative complete phase-timing run;
earlier `baseline/frames.csv` and `baseline/detailed/` record observer-wiring
limitations (empty steps / zero fixed-step timing respectively), not valid
complete phase evidence.

The normal Release Application loop used its measured wall clock. It reached
roughly 4 FPS and continually executed the eight-step catch-up cap. Fluid steps
cost about 43 ms (density/neighbour 16.8, boundary 14.4, velocity 10.0 ms), and
player fluid queries another 6.2 ms per rigid step. Surface extraction was about
3.7 ms and draw submission about 0.1 ms: rendering was not the original culprit.
No duplicate world or fluid step was found. The catch-up cap discarded backlog,
but the excessive physics cost kept forcing more work on following frames.

The new 6.5 × 6.5 × 2.6 m basin is substantially broader than that narrow fixture.
At 0.40 m spacing it still cost about 195 ms/frame, with 1,436 particles and
16 bodies. The accepted production configuration uses 380 particles at 0.65 m,
without shrinking the basin or changing solver iterations/substeps. All water
particles continue to simulate. The vessel is sized to resolve that coarse field.

## Generic engine changes

- Reusable flat neighbour adjacency; identical particle/cell visitation order.
- Prepared kernel constants, preserving arithmetic order and formulas.
- Cached conservative endpoint OBB bounds and exact local slab rejection before
  the existing contact/union/skin calculations; no distance cutoff or name branch.
- Prepared per-query solid SAT axes/support radii, compact sorted column intervals
  with original visitation order, and ordered occupied-cell traversal that skips
  empty hash searches. Scratch is query-local; fields retain no pose pointers.
- Dry hydrostatic rejection before column allocation/solid occlusion, using
  conservative AABB bounds then actual particle height intervals along **each
  query's own up direction**. Wet reconstruction remains the original path.
- Fluid revision invalidates presentation on additions/clear/steps. Held fluid
  state reuses its exact GPU mesh; world/resource destruction resets the marker.
- Surface grid follows fluid spacing instead of over-refining every coarse field
  on a fixed 0.40 m grid. Scale-1/scale-10 reference resolutions are preserved.
- Narrow passive timing fields expose density, boundary, velocity, hydrostatic
  preparation and player-query costs. Renderer remains the sole GL owner.

The planetary comparison exposed another genuine cost: global AABB height bounds
were too loose for an oblique field, leaving dry body queries at ~7.4 ms per tick.
The exact-direction interval rejection addresses that measured case generically.
The first clean-build attempt was stopped before suites ran when this was found;
`interrupted-build/` preserves its logs. The same initially clean build directory
is resumed after the correction; unrelated suites are not repeatedly executed.

## Evidence organization

- `baseline/`: original lag and complete phase timings.
- `optimization/`: measured intermediate candidates, including unsuccessful ones.
- `comparison/`: first isolated before/after comparison (planet still slow).
- `performance/`: final isolated baseline/current application comparison, commands,
  source and binary hashes, compiler/machine/GL details, raw frame/step CSV.
- `focused/`: existing fluid tests and scene/cup checks, including genuine failures.
- `final/`: single completed clean Release production gate, current editor/project
  smoke, real scene replacement/reload and moved M38 standalone package checks.

Wet-query profiling found 576 volume samples across nine overlapping-wall
partitions; column construction cost about 9.4 ms, SAT preparation 0.5 ms and
column response 2.2 ms. The final face-joined container has five solid pieces /
320 samples at the same sample density. Temporary phase profiling is removed
from final engine code; its reproducible instrumentation patch and raw output
are retained under `focused/`.

The first scoop test put the cup onto the thrown props. It acquired no particles;
that physical obstruction and the raw failure are retained. The unobstructed scoop
path uses ordinary carry forces/torques and tracks **the same particle indices**
through acquisition/carry/pour. No particle is teleported, generated or removed.
Final manipulation uses pitch about local X, matching the existing look control.

## Reproduction

```sh
python3 tools/CreateFluidDemo.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target judas_fluid_demo_probe judas_fluid_demo_integration_tests -j4
./build/judas_fluid_demo_integration_tests --output build/fluid-demo-checks
JUDAS_WORLD_STATE=none ./build/judas_fluid_demo_probe projects/fluid_demo/Scenes/pool.judas build/pool-timing 240
JUDAS_WORLD_STATE=none ./build/judas_fluid_demo_probe projects/fluid_demo/Scenes/planet.judas build/planet-timing 240
```

Use the desktop graphics session for interactive performance. The final regression
runner uses existing offscreen/software GL conventions for correctness and export;
those smoke timings are **not** interactive GPU performance measurements.
`fluid_demo_validation.py` refuses to overwrite prior evidence. Its recorded
`--resume-build` is only for completing this interrupted, initially clean build.

Run the actual demo normally via its `.judasproj`, not the probe. Current project
instructions and the human checklist are in `docs/FLUID_DEMO.md`. Human visual,
listening and interactive acceptance is pending; scripted checks do not claim it.
