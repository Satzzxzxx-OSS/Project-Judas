# FTFT4B-1 reproducible current-engine verification

Start with [RESULTS.md](RESULTS.md), [SOURCE_MAP.md](SOURCE_MAP.md), then raw [execution/results.json](execution/results.json). **A successful measurement runner is not physical acceptance.** Energy executable and full temporal checker return 1; their physical FAIL results are preserved.

## Reproduce from the repository root

Requires the normal Judas C++17/GLM/CMake build dependencies and Python with NumPy 2.3.5 (only seeded fixture generation uses NumPy). Existing environment command:

```sh
build/ftft4-reference-venv/bin/python scripts/ftft4b1_validation.py --output docs/evidence/ftft4b/replay
```

Use a fresh output directory. This first compiles/runs the unmodified supplied energy witness, then builds the real-engine diagnostic and regression targets. It does not copy research production snapshots. Baseline execution originally ran that witness separately and then used `--observations-only`; exact commands/flags, exit codes and timings are recorded. Binaries stay under ignored `build/`.

The default full runner needs the full checkpoint checkout (including existing regression sources and FTFT4 geometry oracle). The compact review ZIP contains current physics translation units and quoted-header dependencies sufficient to compile the energy/temporal/new probe directly with a system GLM installation; it is not a replacement full Judas checkout. Example, from the ZIP root:

```sh
mkdir -p build/ftft4b
c++ -std=c++17 -O3 -DNDEBUG -ffp-contract=off -Isrc \
  docs/evidence/ftft4b/baseline/research/code/production_energy_witness.cpp \
  src/RigidBody.cpp src/RigidBodyGravity.cpp src/Contacts.cpp \
  src/ContactSolver.cpp src/RadialTerrain.cpp src/Broadphase.cpp \
  src/Narrowphase.cpp src/PhysicsWorld.cpp -o build/ftft4b/research-energy
build/ftft4b/research-energy
```

Expected physical witness exit code is 1 for the recorded baseline. Replacing the witness source above with `tests/TimingDefectProbe.cpp` builds the supplementary probe (`energy`, `extended`, or `angular <angular-input.txt>`). No Python data feeds solver answers.

## Evidence provenance

- `initial.json`: clean starting HEAD, all production source hashes and original research ZIP hash.
- `research/`: supplied reference material, preserved unchanged; older `reference/source_snapshot/` deliberately excluded. Its original manifest describes the larger supplied archive. These independent/extracted results are **not newly executed engine evidence**. Compact export keeps only the report, source scope, original witness, rotation input specification and manifest.
- `adapters/`: unchanged previously supplied actual-engine temporal probe and checker.
- `original-energy*` and `compile-energy.log`: first actual-engine witness.
- `execution/`: newly executed logs, inputs, per-case records, independent oracle results, command/exit/timing metadata.
- `final-verification.json`, `SOURCE_SHA256.json`, `git-status.txt`, `HEAD.txt`, `changes.diff`: review provenance; no binaries. Archive has an additional per-entry SHA-256 manifest. `review-export.json` is a post-export sidecar, excluded from the ZIP to avoid a self-hash cycle.

Full-step timing fixes, geometry advancement, alternative multi-contact candidates and fluid work are out of scope. Existing unsupported/extreme geometry and performance debt remain unchanged.
