# Project disk-preservation oracle repair

The combined run reported `the scene file on disk is untouched` as a failure.
That test compared the original authored file to a new canonical serialization,
which now includes explicit default fluid metadata absent from the original file.
The canonical representation differed before Play; there was no demonstrated disk
mutation.

The repaired test captures actual disk bytes before loading/Play and compares
actual disk bytes after session/world teardown. The independent canonical
before/after authored-memory check is retained. A readable-input assertion was
added. No scene fixture, serializer, tolerance or production mechanism changed.

`ProjectTests.before.cpp` and `BEFORE_SHA256.json` preserve the old source. The raw
old failure is in `../validation/regressions/production/judas_project_tests.log`.
The repaired test was built and run once, with exit 0 and no failures; see
`../final-targeted/project.log` and its source-fingerprint/command record.
