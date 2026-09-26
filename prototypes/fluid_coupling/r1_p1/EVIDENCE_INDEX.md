# P1-C / P1-C-M checkpoint contents

## Reproduce the accepted compiled path

From the repository root:

```sh
bash prototypes/fluid_coupling/r1_p1/run_p1cm.sh
```

Requirements: Bash, CMake >= 3.16, a C++17 compiler supporting the configured
GCC/Clang warning/optimization flags, Python 3 (standard library only), and Git.
The recorded compiler and platform are in `evidence/p1cm/current/RUN_METADATA.json`.
No engine dependencies, assets, OpenGL context or external Python packages are
needed for the compiled validation. The command builds into `.build-jsgt/` and
regenerates tracked evidence; timing/platform metadata changes on rerun are expected.
The recorder permits changes inside this prototype and rejects tracked/staged
changes elsewhere. The runner itself performs no Git mutations.

The executable runs 25 preserved P1-C fraction cases with momentum attached and
30 additional momentum cases: 55 runs / 1,655 timesteps. The required unchanged
fraction comparison uses the pre-edit baseline described below.

## Current acceptance evidence

- `main.cpp`, `vof.hpp`, `mass_momentum.hpp`, `momentum_validation.hpp`,
  `transport_oracles.hpp`, `nonlinear_transport_oracle.hpp`, `CMakeLists.txt`,
  `run_p1cm.sh`, `record_p1cm.py`: compiled path, independent oracles and runner.
- `P1CM_METHOD.md` and `P1CM_RESULTS.md`: mechanism and executed gate results.
- `evidence/p1cm/current/`: run log; per-case, per-step and per-stage CSV records;
  controls; exact tested source snapshots; SHA-256 source/evidence fingerprints.
  Its `RUN_METADATA.json` is authoritative for this checkpoint's execution.
- `METHOD.md`, `P1C_METHOD.md`, `P1C_RESULTS.md`: fraction method and earlier
  acceptance report, explicitly marked as the earlier fraction-only checkpoint.

`build/`, `.build-jsgt/`, Python caches and the machine-specific CMake build log
are ignored and excluded. Input research ZIP archives are retained as supplied
source provenance, not executable/build artifacts. Recorded compiler/platform
facts in run metadata remain part of reproducibility evidence.

## Preserved baseline and earlier failures

- `evidence/p1cm/baseline/`: original accepted P1-C sources and their fingerprints;
  `prior_results/` preserves the original results packet; `rerun_results/` plus
  `rerun_stdout.txt` preserve the passing pre-momentum rerun. The current runner
  compares summary fields except timing and requires byte identity of its three
  fraction stage/step/oracle CSV files against this rerun.
- `results/scalar_average_P1C_RESULTS.md` and `results/p1c_trace.csv`: FAILED
  scalar-predictor-average interpretation (negative empty-cell fraction).
- `results/continuous_predictor_archive/`: FAILED continuous-C predictor JSGT
  predecessor, including its source, report and raw evidence. This is not the
  passing frozen-WY method.
- `results/jsgt_initial_constant_fixture_*`: earlier diagnostic fixture evidence,
  not current acceptance data. Corresponding preserved copies in the baseline
  retain the same historical role.
- `evidence/p1cm/first_complete_run/`: earlier passing logs; its `NOTE.md` explains
  the source-copy timing limitation. Acceptance uses `current/` instead.
- `hydrostatic.hpp`, `cutcell.hpp`, `coupled_projection.hpp`, `triple_point.hpp`:
  inactive historical component code. The current target does not include these
  files, and they provide no integrated pressure or moving-body acceptance.

Historical fingerprint caveat: `results/jsgt_run_metadata.json` and
`evidence/p1cm/baseline/rerun_results/jsgt_run_metadata.json` are inherited records.
Their adjacent summary CSV timing fields were regenerated later, so those old
summary hashes do not certify the adjacent rerun summaries. They are preserved
unchanged as history. Use `baseline/prior_results/` for the original packet and
`current/RUN_METADATA.json` for current tested sources/evidence. Do not treat an
old hash as a newly verified execution.

## Supporting supplied reference

`evidence/p1cm/Judas_P1C_M_Astra_Handoff.zip` and extracted `handoff/` preserve the
supplied material and its integrity manifest. `handoff/evidence/rechecked/`
contains dependency pins (`numpy==2.3.5`, `shapely==2.1.2`) for the full supplied
reference. Moving-geometry examples in those archives are outside this checkpoint.

`reference_rerun/run_fixed_grid_reference.py` needs Python 3 and NumPy only. Run:

```sh
python3 prototypes/fluid_coupling/r1_p1/evidence/p1cm/reference_rerun/run_fixed_grid_reference.py
```

Its documented AST selection executes the unchanged fixed-grid reference
functions, omitting unused Shapely imports and moving-geometry functions. The
recorded rerun used Python 3.12.14 / NumPy 2.3.5 and verified 18 cases / 216 steps
plus two analytical witnesses. This supporting reference is not C++ acceptance.

## Scope boundary

JSGT with frozen Weymouth-Yue predictors is Judas's declared transport method,
not a claim of faithful van der Eijk-Wellens Eq. 15. This checkpoint contains no
integrated pressure solve, moving-body/cut-cell transport, gravity, buoyancy,
swimming, P1-D/E, 3D or production integration. Full P1 remains incomplete.

Checkpoint preparation changed only packaging/documentation and the evidence
recorder's repository-state audit. Numerical source, fixture/oracle source,
CMake configuration and the build/run script were not changed for checkpointing.
