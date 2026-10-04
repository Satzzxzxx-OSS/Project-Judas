# Human-reproduced blocking failure

After the M55 candidate handoff, the operator drained the ordinary flat pool
using authored controls and then refilled it. The surface reported:

> surface pressure/continuity solve exceeded bounded iteration budget

The operator reported the displayed M54 ledger as TOTAL 161000.000 L,
ERR 0.0e+0 L. Ordinary filled operation otherwise looked good. No screenshot
file was supplied with this follow-up request; this records the operator's report.

The original candidate fingerprints are preserved in
`INITIAL_SOURCE_FINGERPRINTS.json`. `flat-first-reproduction.log`,
`flat-baseline.log` and `solver-trace-before.log` preserve local reproductions
before correction. The first local reproduction fails already at the almost-dry
step with a line-search error; continuing the authored refill despite that error
reproduces the same surface-only failure family. M54 total remains conserved.

`momentum-only-failure.log` preserves the intermediate correction's exact
bounded-iteration error. `boundary-only-follow-up.log` and
`paired-transfer-roundoff-failure.log` preserve the later repeated-cycle failure
and identify negative roundoff from overspent face budgets. Those are failures,
not accepted results. The temporary solver tracing was removed from source after
identification. The narrow test-build error is retained separately; its correction
uses the public allocation planner rather than exposing private connectivity.
