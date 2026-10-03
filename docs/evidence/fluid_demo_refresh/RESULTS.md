# Measured result — production-fluid demo refresh

Starting / unchanged HEAD: `fc6f44f233e82214c6400c05e695fa71774727db`.
Candidate is uncommitted. No M49 work, commits, pushes or tags.

## Original regression

The exact old ignored preview is retained in `baseline/operator-preview.judas`.
It contained 1,280 particles / five static bodies / zero cavities, with 60 Hz
rigid and 30 Hz liquid updates. Its median frame was **229.37 ms (~4.4 FPS)**;
all steady frames hit the eight-fixed-step catch-up cap. Executed liquid median
was **43.41 ms**, including ~16.8 ms density/neighbour work, ~14.4 ms boundary
work and ~10.0 ms velocity/smoothing. Player hydrostatic queries added **6.23 ms
per rigid tick**. Rigid collision was ~0.003 ms, extraction ~3.75 ms and scene
submission ~0.1 ms. It was CPU fluid work plus catch-up, not a rendering-only bug.

This was both inefficient engine work and an unsuitable fine-resolution
reference fixture being used as a current swimming demonstration. No production
profile previously established that fixture as an interactive large pool.

## Repair

Engine fixes reuse neighbour adjacency/kernel constants, reject definitely
irrelevant solid bounds, skip impossible hydrostatic heights along arbitrary
query-up, traverse occupied field cells in the original order, store column
intervals compactly with original accumulation order, prepare column/solid SAT
constants, and avoid extracting/uploading unchanged held liquid meshes.
Presentation grid spacing now follows fluid spacing; scale-1/10 reference grids
are retained. No new forces, collision laws, particle deletion, quality changes
by object/scene name, research solver, simulation duplicate or controller rewrite.

Content keeps a **6.5 × 6.5 × 2.6 m** initial water volume but uses practical
0.65 m spacing: **380 live particles, 16 bodies (3 dynamic), one empty cavity**
in each scene. The face-joined 1.94 m vessel avoids overlapping-wall partitions
(5 pieces / 320 volume samples, rather than 9 / 576), at the same quadrature
quality. Mass is 1,500 kg. Each particle represents approximately 274.625 kg.
This is a resolved bucket, not a claim that tiny mugs work at coarse spacing.

Both scenes **explicitly author 20 Hz liquid**, with rigid physics/input at
60 Hz. The engine default remains 30 Hz; all particles execute ordinary substeps
and density iterations. The 30 Hz candidate remained inadequate during scooping
(planet upright phase ~10.6 FPS), which is preserved in `performance-final/`.
The lower authored cadence is an honest approximation/performance tradeoff,
not an engine branch or a frozen/decorative subset of the water.

## Final interactive performance

Intel i7-4790 / NVIDIA GTX 970; GCC 15.2 Release (-O3, NDEBUG), normal desktop GL.
Measured real application clock, no forced render/fixed timing, no concurrent
builds/tests. Stationary summaries omit ten initial frames; operation summaries
include every frame. Raw frames/steps/source hashes are in `performance-20hz/`.

| Scene | Stationary FPS | Frame median ms | Whole fixed mean / max ms | Executed liquid median / max ms | Slowest operation FPS | Operation frame p95 / max ms |
|---|---:|---:|---:|---:|---:|---:|
| pool | 58.7 | 17.03 | 4.37 / 18.77 | 11.33 / 17.83 | 54.4 (upright) | 28.33 / 36.87 |
| planet | 58.7 | 17.17 | 5.39 / 19.33 | 12.58 / 18.03 | 48.4 (dip) | 34.53 / 47.49 |

No final measured operation frame hit the eight-step cap. These are machine-
specific observations, not a universal FPS guarantee. Surface extraction is
still roughly 10 ms when performed; held frames reuse the mesh. Wet compound
hydrostatic queries and finite-mass cavity contact remain the expensive physics
work. Optional micro-diagnostics / historical limits were not promoted to gates.

The same new 380-particle scenes with the preserved pre-optimization engine at
30 Hz ran at roughly 8 FPS (`performance-final/before-*`). The earlier oblique
30 Hz intermediate still lagged (`comparison/`), and all failed/imperfect timing
runs remain available. Final gains combine generic engine fixes **and** explicit
content/cadence choices; they are not claimed as an unconditional engine speedup.

## Executed correctness and lifecycle

- Focused current-scene check: **49 checks, zero failures**, both scenes through
  ordinary `StepPlayedWorld`. Input enters water, swims and climbs out.
- Thrown low/high-density blocks retain float/sink ordering.
- Flat bucket acquired **6 particles**, carried those same **6**, poured to **0**.
- Planet bucket acquired **4**, carried those same **4**, poured to **0**.
- Exact particle inventory and summed represented mass preserved in both scenes.
  Dry buckets stayed empty; destroy invalidated the old body handle; subsequent
  steps and reconstruction worked. No unresolved real-solid geometry recorded.
- Planet uses a real 30 m sphere as its floor and real radial gravity. Direction
  changes across the basin. No rotated flat simulation is used.
- Existing focused hydrostatics: **93 checks, zero failures**. Existing fluid,
  cavity, swimming, rigid-contact, lifecycle and other regressions are included
  in the final production gate.
- Final Release build: zero compiler warnings. Initially empty build directory;
  a pre-test partial build was interrupted for the measured planetary fix and
  resumed with normal dependency rebuilds. No prior build objects were copied.
- **90 production suites, all passed**. The unchanged runner also executes the
  real async integration: **12 cases / 246 checks, zero failures**.
- Existing editor async smoke and current project Play/Stop both passed; authored
  state restored identically. Normal standalone then moved M38 package loaded
  pool -> planet -> reloaded planet -> pool through public JS/M43 APIs.
- Package size **7,451,346 bytes**, export **0.022 s**. It was launched from `/tmp`,
  outside repository/build/package working directories. Smoke JS is injected
  only into ignored copied projects; current authored assets remain unchanged.

## Explicit remaining limitations

The accepted model remains coarse PBF plus approximate exterior hydrostatics /
drag and finite-mass cavity contact. Exterior momentum/energy is not exact.
Surface reconstruction can visually extend beyond individual particles/walls;
small droplets/cups are resolution limited. This is a local planetary basin,
not a global ocean. No live origin rebasing or new player controller is added.

A forced planetary dip produced a **brief 59.78 m/s particle velocity peak**
(with zero hard-geometry escape displacement). It remained finite; at the end
of the full run every particle was within **6.24 m** of the basin origin, and
mass was unchanged. This numerical transient is disclosed, not called accurate
physical motion or silently clamped. Human review must judge visible interaction.

**Human interactive/visual acceptance: PENDING.** Automated state/performance
checks do not prove that the demonstration feels good. Use the explicit flat /
planet / exported-game checklist in `docs/FLUID_DEMO.md`.

## Reproducibility / handback

- Current project: `projects/fluid_demo/fluid_demo.judasproj`.
- Scenes: `projects/fluid_demo/Scenes/pool.judas`, `planet.judas`.
- Instructions: `docs/FLUID_DEMO.md`; F1 flat, F2 planet, G pickup/drop, H throw,
  Space swim/jump, R reload, Escape project menu.
- Commands / raw results / hashes: `README.md`, `final/`, `performance-20hz/`.
- Final changed implementation fingerprints: `final/SOURCE_SHA256.json`.
- Exact changed-file list: `CHANGED_FILES.txt`.
- Protected historical FTFT/P1/M33–M48 evidence and prototypes remain unchanged.

Proposed commit after operator review:
`Fix production fluid performance and restore fluid validation demo`
