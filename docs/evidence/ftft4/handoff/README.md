# Source-specific FTFT4 handoff

**Give Astra `ASTRA_FTFT4A.md`. Implement FTFT4A only and report back.**
The source-mapped review is `SOURCE_REVIEW.md`.

This package authorizes a bounded first geometry/broadphase repair, not the
full time-of-impact integration at the same time. FTFT4 remains open for the
separate event-timing pass. It is not a completed engine patch.

## Evidence labels

- Reviewed snapshot: 3404388; original archive/comment/source hashes checked.
- Previous independent exact geometry/impact research: rerun successfully.
- Actual source's scalar restitution-target branch: extracted, compiled, run.
- Actual linked PhysicsWorld/DynamicAabbTree adapters: **NOT RUN** here;
  GLM headers unavailable. The failed build log is preserved.
- No original source files modified.

`world_probe.cpp` is a diagnostic driver, not an unconditional-success test
masquerading as acceptance. Use `check_world_trace.py --mode baseline` to check
pre-fix reproduction, `--mode geometry` for the FTFT4A no-ghost-gap witness,
and later `--mode full` for the selected FTFT4B event checks. Other required
oracles/regressions remain necessary regardless of this small driver's exit.

## On the complete developer repository

```sh
bash code/run_source_probes.sh /path/to/Project-Judas /path/to/probe-build
python3 code/check_world_trace.py /path/to/probe-build/world_probe.csv --mode baseline
```

The raw tree replay has actual assertions and failure exit status. It uses
400 exact proxy-query expectations. At PhysicsWorld level, fat-bound extras
are permitted; compare conservative supersets, not false-positive count zero.

## Independent reference rerun

```sh
bash code/run_independent.sh
```

Dependencies: Python, numpy, mpmath, C++17 compiler. The original reference's
recorded versions were numpy 2.3.5 and mpmath 1.3.0. Production source probes
add the same genuine GLM headers the engine uses; no substitute is included.

No claim of renderer, application, sanitizer or full-regression execution is
made by this handoff. Files under .build-* are disposable; sources and evidence
are in the package and should be kept durably.
