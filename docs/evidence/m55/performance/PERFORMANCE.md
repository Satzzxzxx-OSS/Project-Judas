# M55 performance — final candidate

Native desktop X11 / NVIDIA GTX 970, driver 580.178.04. Release. Measurements
were sequential with no simultaneous build. Liquid runs every **60 Hz fixed step**.
No depth cap, reduced cadence, skipped body/cup coupling or smaller final pool.

## Actual normal application loop

75 requested frames, 15 warmup; 61 reported observations. Short, unattended
samples. Frame times include the actual rendering/swap path and OS scheduling.
Catch-up means more than one fixed step; capped means reaching the eight-step cap.
This is not a long gameplay soak or human responsiveness acceptance.

| Phase | Flat median / p95 / max ms | Radial median / p95 / max ms |
|---|---:|---:|
| Frame | 17.111 / 25.373 / 33.416 | 18.560 / 27.908 / 31.465 |
| Fixed step | 5.182 / 8.655 / 11.979 | 6.897 / 8.787 / 9.710 |
| Liquid total | 4.699 / 8.016 / 11.283 | 6.655 / 8.519 / 9.332 |
| Containers | .421 / .573 / 1.392 | .140 / .232 / .260 |
| Changed-solid storage | 2.262 / 3.599 / 7.004 | 2.887 / 3.850 / 4.869 |
| Body loading | 1.135 / 2.476 / 3.504 | 2.655 / 3.650 / 4.487 |
| World render | 5.708 / 7.928 / 12.554 | 10.830 / 12.555 / 14.417 |
| Catch-up frames / capped | 3 / 0 | 12 / 0 |

Reciprocal median frame times are approximately 58 / 54 FPS, not whole-run mean
FPS. The historical initial native logs (`*-desktop.log`, `desktop.json`) precede
the final caches: radial was 32.208 ms median with 53 catch-up frames. They are
retained as initial measurements, not mislabeled final results. Earlier truly
pathological geometry/refinement/catch-up probes are in development evidence.

## Heavier real scoop/carry/pour and swimming fixture

The application test advances ordinary physics, uses opening exchange and parcels,
then takes 60 fixed-step and 10 rendered measurements. The fixture positions the
cup to exercise geometry; it never sets liquid quantity. These render measurements
include native swap/vsync at 640×360 and are separate from fixed-step timing, so
**do not add/convert them into a claimed interactive frame rate**.

| Measurement | Flat median / p95 / max ms | Radial median / p95 / max ms |
|---|---:|---:|
| Fixed step after interactions | 5.087 / 5.916 / 6.401 | 13.364 / 15.797 / 16.116 |
| Native render + swap | 17.042 / 17.555 / 17.809 | 17.132 / 17.808 / 19.427 |

The radial pre-follow-up interaction was 18.489 / 21.681 / 22.907 ms per fixed step.
Final last-step phase samples: radial liquid 11.407 ms, containers 1.325, storage
6.479, solve .789, loading 2.814. Flat: liquid 5.587, containers .398, storage 2.555,
solve .895, loading 1.738. These single last-step phase values are not medians and
need not equal the above distribution statistics. Moving radial solid geometry
remains the significant cost, close enough to the fixed-step budget that extended
human interaction must judge responsiveness.

| Final content/work | Flat | Radial |
|---|---:|---:|
| Main lake initial quantity | 160 m³ | 65 m³ |
| Main lake surface cells / faces | 384 / 728 | 192 / 356 |
| Rigid bodies | 41 | 11 |
| Parcels at measured endpoint | 2 | 0 |
| Main available / original tetrahedra | 6028 / 2717 | 28727 / 24862 |
| Draw calls in application view | 99 | 32 |
| Bucket real acquired volume | 25.088 L | 25.088 L |
| 100 point queries | .040 ms | .989 ms |
| 32×18 optical paths | .952 ms | 1.993 ms |
| Main cap mesh | 17439 vertices / .662 ms | 42453 vertices / 2.091 ms |

Counts at that endpoint change naturally with moving solids/parcels. Basin count
is 8 flat / 2 radial; one authored bucket cavity in each scene. No bulk liquid
particles or depth-proportional dynamic layers. The separate graph-water ledger
is deliberately distinct from the water ledger displayed in the main totals.

## Depth comparison, fixed surface resolution

Same 128 cells / 232 faces, same settings, 120 steps:

| Depth | Mean step | Mean pressure iterations | Initial / final disturbance energy |
|---|---:|---:|---:|
| 10 m | .1151 ms | 4.87 | .8000 / .21994 |
| 100 m | .1141 ms | 7.08 | .0800 / .02105 |

This demonstrates bounded surface work without dynamic depth layers, not identical
physics or exact spectral deep-water dispersion. The first-order local-inertia
hydrostatic model and exponential friction approximate long waves.

Normal flat bake is ~.33 s; radial fixture bakes both owners in ~22.47 s at the
clean gate. Curvature/refinement work is offline, not free. The M54 regression's
separate stress fixture with 100 rotating containers measured ~23.07 ms; it is
not claimed to be a cheap large moving-container workload. The final affected
rerun measured 28.10 ms for that same 100-container fixture (timing variance is
retained, not discarded).

Final export: 22 asset records, 16,914,698 bytes (~16.13 MiB), about .65 s.
No file-copy optimization claim. Logs and exact timings are in `../followup/`.
