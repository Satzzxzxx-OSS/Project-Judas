# Surface-candidate performance follow-up

Initial identical-quality workload: 1,585 nodes, 4 substeps, 4 iterations, 60 Hz
fixed step. Active median 394.70 ms, p95/max 406.65 ms; zero hot allocations.
Profiled last step: surface contacts 379.68 ms, including candidate construction
72.02 ms; internal constraints 6.32 ms. The initial sorted sparse grid performed
two binary searches per touched cell per vertex/edge at every solver iteration,
and repeatedly sorted all expanded primitive cell entries.

Correction: fixed-capacity open-addressed cell buckets and intrusive primitive
chains reuse preallocated storage. Candidate IDs are still sorted before solving,
so contact order remains deterministic. Quality, thickness, filtering and cadence
are unchanged. Overflow remains a diagnostic; no truncated successful solve.
The initial measurement and phase log are retained. Follow-up measurements are
recorded separately.


The hash prototype still spent excessive time on broad candidates. Follow-ups
replaced it with fixed static-topology BVHs, refitted to current/substep-start
bounds, plus immutable one-ring exclusions and a conservative self-pair cache.
Both current and previous-substep node motion are bounded by half contact
thickness; exceeding that margin rebuilds pairs, padded for BOTH primitives.
Narrowphase still tests current geometry every material iteration. No quality,
cadence or contact thickness reduction. Pair overflow reports failure.

Final same-workload active median 7.663 ms, p95/max 8.140 ms; sleeping 0.0194 ms,
no-deformables 0.000742 ms. Zero hot allocations in the measured native fixed
solver; internal strain evaluation became the slowest phase (~6.28 ms), then
EE contact (~1.04 ms). See `performance-cache.log` and final focused results.
The earlier hash approach is historical, not the shipped algorithm.
