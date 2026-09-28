# FTFT4A-P — equivalent geometry work, measured in the engine

This pass starts from the **uncommitted, correct-but-slow FTFT4A** source,
not from HEAD. HEAD remains `3404388f1af48419505c802fc11dbf8a0b577f38`.
The previous FTFT4A evidence stays in its original directories. No contact
law, restitution target, warm-start rule, solver iteration count or impact
step order was changed. FTFT4B remains OPEN.

## Provenance

`pre-optimization-source.zip`, `pre-optimization.json`, the binary Git diff
and status preserve the source before this pass. All 321 repository entries
in the supplied export matched before editing; all 377 supplied package
entries verified. `handoff/` preserves that research package. Its scalar
storage adapters are supporting references and are **not engine dependencies**.

## Implemented reuse and invalidation

- `ContactPreparedOrientation` owns the interval and ordinary homogeneous
  quaternion data, proper binary64 rotation and normalized interval entries.
  Its key compares the four actual float representations, including signed
  zero. Exact expansions remain lazy and use original parent poses/offsets.
- `PreparedShapeBounds` stores each rotated child-offset product separately
  and the original accumulated extent. Translation starts from the supplied
  position and adds the three products in the original order. The original
  uncached AABB function remains available for exact comparison.
- PhysicsWorld owns caches by body slot value, with no pointer to a movable
  vector element inside the geometry cache. Shapes are immutable within a
  slot generation; destruction/reuse installs an entirely new Body and shape
  radius. ResetBody updates either dynamic or static bodies normally.
- Current and previous endpoint bounds have independent exact pose keys.
  Step captures the previous endpoint before integration. Position correction
  precedes the final proxy refresh. Orientation changes invalidate prepared
  bounds; translation changes invalidate translated bounds only.
- Solver frames are shared by body within a solve. Inertia is evaluated once
  for the velocity phase; post-integration rotations are refreshed from the
  same keyed body preparation before the position phase. Clear drops all
  frame pointers and index entries. The existing constraint lifetime contract
  still requires bodies to outlive their solve.
- `SatWorkspace<Iv>` factors the **interval filter only** using the supplied
  proper-quaternion identities and nine cross-body dots. The original exact
  polynomial and ordinary gap construction remain. A numerical gap that
  contradicts its certified sign is recomputed with the exact numerator;
  arithmetic failure remains visible, never replaced with zero.
- Optional exponent-bit guards, quaternion-product hoisting and exploratory
  sign-selected interval arithmetic were **not installed**.

## Diagnostics and measurement scope

StepStats counts actual within-Step cache hits/rebuilds and solver frames.
Shape-radius construction occurs at CreateBody, because proxy construction
already requires a bound. Mixed-workload setup time explicitly includes
creation and this preparation. Moving-static workload frame timers surround
ResetBody, forces and Step, so ResetBody preparation is not hidden.

Cache bytes report retained vector capacity and the hash node/bucket payload
estimate, excluding allocator bookkeeping, struct padding and the two new
per-constraint frame indices (`2*sizeof(size_t)` per reserved constraint;
16 bytes on this host). Process peak RSS is also recorded but includes the
whole process and cannot isolate cache payload. Allocation counts record body
frame hash nodes, bucket changes, frame-vector growth and prepared compound
storage allocations within Step. They are cache-specific counts, not a
whole-process allocator profiler. Creation/ResetBody work outside Step is
covered by the outer measured fixture time and is not mislabeled as Step work.

The historical, slow-correct and optimized engine builds use the same
compiler and `-O3 -DNDEBUG -std=gnu++17 -Wall -Wextra -ffp-contract=off`.
No fast-math or native-ISA switch is used. The original TestScaleStatistics
setup, 30 steps and assertions remain unchanged. Whole-step time is
primary; component sums omit initial velocity/reach work. A supplemental
complete-fixture timer also includes creation, destruction and the original
exhaustive correctness comparison; it is **not** pure simulation time.

## Reproduction

With the repository's usual C++17, CMake, GLM, SDL2/OpenGL dependencies and
Python 3, run from the repository root:

```sh
python3 scripts/ftft4p_validation.py --build-references
python3 scripts/ftft4p_validation.py --components
python3 scripts/ftft4p_validation.py --build-optimized
python3 scripts/ftft4p_validation.py --build-scale-wrappers
python3 scripts/ftft4p_validation.py --regressions --jobs 4
python3 scripts/ftft4p_validation.py --benchmarks --trials 7
```

Each mode refuses to overwrite completed evidence. Reference sources are
extracted into disposable build directories from the persistent snapshots;
no checkout/reset is performed. Preserve existing result directories before
an explicitly requested rerun. Tests use offscreen real GL, not a claim of
human visual validation. FTFT2's M31 stress output remains a supporting
non-deterministic diagnostic; its 12-case integration harness is the actual
async acceptance path. Protected FTFT1–3 evidence output is redirected.

See `cache/README.md` for cache/oracle commands and `RESULTS.md` for executed
counts, timings, limitations and the final source manifest. Protected fluid
prototypes are hash-checked and are not rebuilt or modified.
