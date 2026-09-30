# Final Judas stabilization gate

**JUDAS STATUS: READY FOR NEW FEATURE DEVELOPMENT**

Verified checkpoint: `1b3a134bdb3d7876d8ed8261571311ca7880e386`. Local HEAD, main
and live origin/main matched, and the tree was clean before evidence creation.
The existing build was moved to ignored `.cache/final-gate-prior-build-1b3a134`;
`build/` was absent at configuration. GNU 15.2.0 Release (`-O3 -DNDEBUG`) configured
and built successfully: **zero compiler warnings**. No engine/test implementation
changes or mandatory failures occurred.

## Executed once

- **61/61 production suites** passed: [per-suite commands, exits, times and binary
  hashes](validation/regressions/production/results.json).
- Actual nonblocking application: **12 cases / 246 assertions**, real IO, decoding
  and GL operations; default-async editor Play/Stop and standalone blocking
  reference startup passed. The latter is not async evidence.
- Current-schema FTFT1 valid/mismatch/legacy runtime/editor checks passed.
- Four **900-step** classic/terrain near/far harnesses completed with identical
  corresponding physical payloads.
- [Full gate](validation/results.json) passed, including unchanged source and
  protected-file checks. The existing M31 stress tool remains supporting evidence.
- [Shipped performance](performance.log): classic/terrain particle medians
  **6.961472 / 11.7904085 ms**, maxima **10.008286 / 14.245437 ms**; hybrid
  amortized costs **4.19877820833 / 5.66133594722 ms per 60 Hz frame**. Each scene:
  360 ordinary rigid steps, 180 particle steps, 125 particles. Both existing gates
  passed; no contact caps or normal-acceleration attempts.
- Existing broadphase fixture reported a **12.059 ms** 1,500-crate step. This is
  its final-step measurement, not a new seven-run median. The previous accepted
  seven-run median remains 14.266 ms. No new performance campaign was run.

Total final gate: **569.738 s**, including **217.849 s** fresh compilation.
Human visual validation was not run. Renderer frame timing was not benchmarked;
fluid figures are CPU simulation measurements. Runtime offscreen EGL messages
remain in raw logs.

The six optional FTFT9 micro-diagnostics remain accepted known limitations, not
mandatory failures. No tolerances, fixtures, diagnostics or approximation claims
were changed. Prior failed runs/rejected experiments and protected fluid prototypes
remain intact. See [accepted scope and limitations](../../../STABILIZATION_STATUS.md).

## Reproduction

From a separate checkout of the verified checkpoint with required existing SDL2,
GLM, CMake, C++ compiler and software-GL dependencies available:

```sh
python3 docs/evidence/stabilization/final-gate/run.py > docs/evidence/stabilization/final-gate/run.log 2>&1
```

The runner is supplied by this final verification commit; copy only this evidence
runner into that checkpoint checkout before execution. Use a new evidence directory
by copying the runner there, preserving its depth below the repository root, or
move aside existing evidence in the separate reproduction checkout. Its guard
rejects reuse of an existing validation output or prior-build backup.
It invokes the unchanged `scripts/ftft9_validation.py` once, adding only the explicit
Release configure flag, then the existing shipped-fluid performance target once.
No production simulation loop is duplicated. [Metadata](RUN_METADATA.json) and
[raw run log](run.log) record execution; [manifest](SHA256.json) preserves evidence.
