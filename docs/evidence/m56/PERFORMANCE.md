# M56 lightweight performance evidence

Release, SDL offscreen, Mesa software GL, swap interval disabled. Operator apps were left running; see `development/CONCURRENT_LOAD.txt`. Each project used interleaved A/B/C/B/A/C trials, 120 rendered frames and 120 identical fixed steps per trial. Table excludes the first 31 frames and pools two trials per mode. These are elapsed observations, not desktop FPS or CPU-utilization claims. Final nonfunctional UI placement/column tidy received the separate editor interaction smoke.

| Project / mode | Frame median ms | Frame p95 ms | Frame max ms | Before profiler UI median ms | Fixed median ms | UI CPU submission median ms |
|---|---:|---:|---:|---:|---:|---:|
| liquid_surface_demo / off | 38.179 | 42.212 | 57.861 | 38.160 | 4.945 | 0.000 |
| liquid_surface_demo / capture_hidden | 38.119 | 45.892 | 64.808 | 38.038 | 5.129 | 0.000 |
| liquid_surface_demo / capture_visible | 40.764 | 44.039 | 60.369 | 39.927 | 4.852 | 0.681 |
| shooter_game / off | 18.343 | 21.206 | 30.311 | 18.324 | 1.272 | 0.000 |
| shooter_game / capture_hidden | 18.216 | 20.849 | 22.237 | 18.131 | 1.331 | 0.000 |
| shooter_game / capture_visible | 21.826 | 24.983 | 28.945 | 20.955 | 1.336 | 0.754 |

Hidden-capture differences are smaller than the trial-to-trial noise (including negative apparent differences); they are not speedups. Visible UI has real additional CPU/driver/GPU cost. Before-UI elapsed includes driver calls and excludes the explicit profiler UI/swap/end-frame collection; it is not a measure of CPU utilization. Frame interval includes collection, swap and UI. p95 is the lower empirical quantile. Maxima are retained, not hidden behind averages.

## Instrumentation-only probe

400 warmed native scopes/frame, 160 frames/trial, ABBA: disabled 0.001832 / 0.001084 ms per frame; enabled 0.072674 / 0.048359 ms per frame, **including frame collection**. Enabled cost is about 121–182 ns per recorded scope in this small probe. Separate warmed 1,000-scope allocation check reports zero heap allocations during ordinary recording. No zero-overhead claim.

Profiler reserved CPU capacity estimate in the retained captures reaches approximately **70.8 MB (67.5 MiB)**. It is bounded, excludes consumer/allocator/GL-driver storage, and is not process RSS or VRAM. No drops/truncations/GPU drops occurred in the representative final application captures. Deliberate overflow/freeze tests are separate.

## What the current captures expose (not optimized)

Spring Range: 45 physics bodies, 6 navigation agents. Main-thread median observed frame contributions: JavaScript fixedUpdate ~0.547 ms, motors ~0.315 ms, navigation ~0.190 ms. Existing render/driver work dominates this software-GL workload.

M55: 41 physics bodies, 9 liquid owners, 576 surface cells / 1,064 faces observed per executed solve. **Solid excluded storage rebuild ~1.860 ms**, **Dynamic surface solve ~0.688 ms**, body loading ~0.862 ms, optical paths ~2.672 ms, surface mesh ~1.375 ms. Water-boundary texture upload ~16.948 ms and surface upload/submission ~8.727 ms under this software driver. These nested elapsed intervals overlap; they are not additive independent costs. The same boundary upload also runs in the dry Spring Range render path (~13.838 ms here). This is real existing render/driver work revealed by instrumentation, not a new liquid algorithm.

The collector deliberately leaves these expensive phases intact. Coarse GPU timestamp intervals report the actual delayed GL measurements; CPU submission, GPU elapsed and waits must not be added. Completed GPU records retained their originating frame; the latest passes may remain pending.

Full medians, p95/max, counters, per-trial values and source paths: [PERFORMANCE.json](PERFORMANCE.json). Paired raw CSV/state/captures: `final/observation-followup/`. All six deterministic states per project match. The bucket workload physically acquires liquid through the ordinary opening/owner transfer model.
