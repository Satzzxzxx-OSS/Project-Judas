# M37 candidate results

Baseline: `2098949f003e1b8e5588ce8651260db26e7ba04a`.
One genuinely clean Release build completed with **zero compiler warnings**.
One full production gate executed **68 suites, all passing**. Focused visibility/
particle checks: **28/28**. Actual async Application demo: **11/11**. Existing
M33 camera checks: **52/52**. Real async integration: **12 cases / 246 checks**.
Editor async Play/Stop/save/reopen and standalone startup passed. The standalone
scripted smoke is a labelled blocking reference, not async evidence.

The build/full-suite/smoke invocation took **673.150 s**; this is validation wall
time, not frame cost. Exact sources remained unchanged during the run. Protected
FTFT/P1/M33–36 evidence and fluid prototypes remain unchanged. `SOURCE_SHA256.json`
records final source/content fingerprints; original development compilation
failures remain as raw logs. No commit/push/tag was made.

## Local rendering measurements

Colour-pass CPU submission only, actual Renderer calls, 300 ordinary boxes,
20 frames per row; clear is outside the measured submission interval. No claim
of universal GPU/frame latency or broad speedup is made.

| Culling | Camera | Median CPU ms | Draws | Culled |
|---|---|---:|---:|---:|
| Disabled | Facing boxes | 1.930 | 300 | 0 |
| Disabled | Facing away | 0.752 | 300 | 0 |
| Enabled | Facing boxes | 2.410 | 300 | 0 |
| Enabled | Facing away | 0.0217 | 0 | 300 |

This run shows a higher CPU cost when every box is visible (including normal
driver/timing variation); culling is not claimed to accelerate all-visible work.
Facing away removes all submissions. Eight small emitter submissions (32 total
particles): visible **0.169 ms / 8 draws**; offscreen **0.000345 ms / 0 draws**.
This is a lightweight sanity workload, not a large-particle benchmark.

## Limitations / human gate

Particles sort only within each emitter, not globally with other transparency.
No collisions, shadows, lighting or separate particle interpolation. Visual pool
state restarts on reconstruction/Play and is not saved gameplay state. Emission
at capacity discards excess new particles. See `docs/M37.md` for exact semantics.

The real demo screenshot is under
`final/fixture-details/judas_particle_application_tests/demo.png`.
It shows rendered particles and a secondary-camera surface, but human visual
acceptance remains **PENDING**. Machine GL evidence is not operator acceptance.
