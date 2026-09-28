# Judas FTFT4 independent research

Read RESEARCH_REPORT.md first. This is **not a production pass or ready-to-apply patch**.

The pack contains exact geometry oracles, interval-filter checks, represented-float gamma8 counterexamples (Python and C++), impact timing/impulse checks, shape-sweep examples, and tree query fixtures.

Reproduce:

```sh
python code/run_all.py
```

Python dependencies are in requirements.txt; the scalar compiled probe also requires g++.

Current source could not be retrieved. `SOURCE_EXPORT.md` gives a read-only source export so the final brief can be mapped to actual Judas code.

Every result file and RUN_METADATA.json labels its scope. In particular, 400 generated tree query oracles are NOT 400 passing Judas queries.

No production or protected fluid prototype changed. No prompt in this package authorizes changes to the user's repository.
