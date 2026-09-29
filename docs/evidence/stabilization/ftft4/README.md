# FTFT4 stabilization evidence

Status: **FTFT4 CLOSED under the documented approximations**. See [RESULTS.md](RESULTS.md).
Passing source-matched runs: `correctness7/`, `performance3/`, `regressions2/`.
The current archived source is `stabilization-source.zip`; `tested-source.zip` is historical.

Reproduce each phase from the repository root with a **new** output directory:

```sh
python3 scripts/ftft4_closure_validation.py correctness --output docs/evidence/stabilization/ftft4/new-correctness
python3 scripts/ftft4_closure_validation.py performance --output docs/evidence/stabilization/ftft4/new-performance
python3 scripts/ftft4_closure_validation.py regressions --output docs/evidence/stabilization/ftft4/new-regressions
```

The runner records commands, raw exits, source hashes and protected-file
checks. It uses the existing C++17/GLM production build and independent
Python oracle environment at `build/ftft4-reference-venv/bin/python`.
Binaries and object files remain in ignored build output.

Earlier directories are immutable development evidence. In particular,
`performance/` failed the absolute 15 ms gate; `correctness4/` records the
rotating grazing conservative-advancement exhaustion. A newer passing run
must identify its own exact source fingerprints; it does not erase these
failures. `tested-source.zip` belongs to `correctness3/`, not later edits.

See [METHOD.md](METHOD.md) for the selected approximations. Historical
FTFT4B diagnostic and rejected PLUS/Poisson evidence remain in their
original directories.
