# FTFT4A partial checkpoint

Operator-approved checkpoint of the tested geometry candidate, cumulative
cache/performance work and completed bounded allocation/lifetime pass.
This is **not full FTFT4 acceptance or production-performance acceptance**.

- Geometry correctness: tested candidate checkpoint, within the recorded scope.
- Bounded allocation/lifetime pass: complete; reviewed storage reuse retained.
- Production performance regression: **OPEN**. Small/mixed allocation-pass gains
  do not establish a broad speedup. Exhaustive-oracle comparison time belongs
  to correctness instrumentation and is reported separately from physics steps.
- FTFT4B impact timing: **OPEN**, including retained stopping, premature-bounce
  and impact-chain failures. Separate authorization is required.
- Previously documented unsupported/extreme cases are unchanged, including
  extreme quaternion arithmetic, represented-input precision limits and the
  approximate terrain path. No numerical guarantee is expanded here.

No numerical edits, experiments, rebuilds, tolerance changes or milestone tags
were made while checkpointing. Only the concise status and this checkpoint
record were added to the reviewed source/evidence set.

`allocation/final-source.json` and its passing measurement record identify all
227 current source/build/test/script files. Staged engine and test blob hashes
are checked against those fingerprints before commit. The production passing
run matches the final engine and existing test sources; the later benchmark
instrumentation is covered by its own executed build/run, as recorded there.

Historical results, pre-fix failures, source snapshots and manifests are
preserved unchanged. Their prior working-tree statuses remain historical
records; this document supplies the operator's current partial disposition.
FTFT1–3 source/evidence and all fluid prototypes remain unchanged.

Only reviewed source, tests, scripts, documentation and reproducible evidence
are staged. Source ZIPs and rendered evidence images are retained; executables,
object files, caches and machine-specific build products are excluded. Build
logs, compiler commands and run metadata are evidence, not build products.
