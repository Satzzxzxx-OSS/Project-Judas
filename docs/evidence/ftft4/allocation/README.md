# FTFT4A-P allocation/lifetime evidence

Read [RESULTS.md](RESULTS.md), [MECHANISM.md](MECHANISM.md), and `SUMMARY.json`.

Persistent evidence is outside `build/`. All binaries and object files live only in `build/ftft4alloc/` or the existing CMake build. The previous optimized candidate remains unchanged in `../performance/` and is copied verbatim into `before-source.zip`.

From the repository root, on a fresh evidence output (each runner mode deliberately refuses to overwrite existing evidence):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-ffp-contract=off
cmake --build build --target judas_contact_storage_tests judas_contact_cache_tests judas_contact_geometry_tests judas_contact_lifecycle_tests -j4
build/judas_contact_storage_tests
build/judas_contact_cache_tests
python3 scripts/ftft4alloc_validation.py build
python3 scripts/ftft4alloc_validation.py lifetime-build
python3 scripts/ftft4alloc_validation.py measure
python3 scripts/ftft4alloc_validation.py regressions --jobs 4
python3 docs/evidence/ftft4/geometry_oracle.py --binary build/judas_contact_geometry_tests --output docs/evidence/ftft4/allocation/geometry
build/judas_contact_geometry_tests --invalid
build/judas_contact_lifecycle_tests
```

For an existing evidence tree, choose a new `OUT`/`BUILD` location through an importing Python driver before invoking its modes; do not delete accepted evidence. Source snapshots/metadata, generated wrappers, exact compiler commands, binary hashes, raw stdout and all-step JSON live in `build/`, `lifetime-build/`, `measure/`, `regressions/` **beneath this evidence directory** (these directories contain logs/text, not binary build output). The actual compiled binaries are under the repository-root `build/ftft4alloc/`.

Measurement wrappers contain only delimited timing/counter insertions. Removing those insertions recovers the original fixture source byte-for-byte; generation asserts this. Both branches link their actual complete engine sources with identical flags. Instrumented heap executables are separate and never supply timing-acceptance numbers. No missing work is pushed outside the whole-fixture timer.

Sanitizer command and metadata are in `sanitizer/results.json`. Source-adapter commands remain in the supplied `../handoff/code/run_source_probes.sh`; new stdout/CSV is preserved here. The full temporal checker is expected to fail its existing FTFT4B cases, with its failures retained in `source-full-open-ftft4b.log`.

The required full regression run happened before adding the final mixed-lifetime timing header; final audit separately verifies unchanged engine/test hashes and the subsequently executed benchmark hashes. No protected evidence output was overwritten.
