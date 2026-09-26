# P1-PF independent reference rerun and RNG input export

This is supporting Python evidence, not acceptance of the C++ implementation.
The supplied handoff tree is unchanged. Its 16 outer manifest entries and all
8 entries in the original nested archive were verified (`integrity.json`).

## Reproduce

From `prototypes/fluid_coupling/r1_p1/`, with Python 3.12 or another compatible
Python and the supplied exact dependency versions:

```sh
python3 -m pip install --target .reference-p1pf numpy==2.3.5 scipy==1.17.0
PYTHONPATH=.reference-p1pf python3 prepare_p1pf_reference.py
```

The local dependency directory is ignored and is not a C++ dependency. The
recorded execution used the bundled Python 3.12.14 with NumPy 2.3.5 already
available and SciPy 1.17.0 installed with `--no-deps` into `.reference-p1pf/`.
`RUN_METADATA.json` records exact interpreter, versions, commands and hashes.

The utility copies the unchanged `projection_checks.py`,
`adapter_reference_checks.py`, `compatibility_checks.py` and requirements into
`execution/`. It runs the first two scripts as ordinary Python subprocesses with
`OPENBLAS_NUM_THREADS=1` and `OMP_NUM_THREADS=1`. It does not alter their physical
assertions or monkey-patch their solvers.

The current execution passes:

- 42 positive projection cases and five negative controls;
- six adapter cases, each after twelve transport steps (72 total);
- the original idempotence and physical gates inside those scripts.

Results are not bitwise identical to the supplied handoff results on this
machine. `*_comparison.json` reports every non-timing difference. All differences
are numeric, with no changed case names or structure. The maximum absolute
projection difference is approximately 6.09e-11; the maximum adapter difference
is approximately 3.33e-14. These include differences between small error metrics.
Both unchanged scripts pass all their original assertions. Runtimes are in the
JSON outputs and are not performance claims for Judas.

An initial successful execution is preserved under `initial_rerun/`. The current
utility was subsequently narrowed to copy only required source/dependency files,
so that inherited supplied metadata is not confused with rerun evidence. No
reference numerical algorithm changed. Current evidence is the parent directory's
`RUN_METADATA.json`, logs, comparisons and `execution/*_results.json`.

## C++ fixture input CSV

`fixtures/p1pf_random_inputs.csv` contains only seeded input arrays, as
`group,index,value` with 17 significant decimal digits. Its contents are not
pressure solutions, projected velocities, forces, acceptance thresholds or other
expected answers. `INPUTS_METADATA.json` records construction, array ordering,
source hashes and the CSV hash.

| Group | Values | Seed and sequence |
|---|---:|---|
| `random_fluid_rho` | 256 | 418; 16x16 uniform draw |
| `random_fluid_closed_w` | 144 | Same 418 stream after density |
| `random_fluid_open_w` | 144 | Same 418 stream after closed velocity |
| `covariance_rho_random` | 576 | 561; 24x24 uniform draw |
| `covariance_w` | 311 | Same 561 stream after density |
| `two_bodies_w` | 218 | 151; normal generalized velocity draw |

The density arrays use the projection reference's **x-major** indexing,
`rho[x,y]`, flattened as `x*ny+y`. Compute density from exported random input by
`rho=1+(contrast-1)*input`; the random-fluid reference resets seed 418 for each
contrast, so all contrasts share these input arrays.

Generalized velocities follow reference face order: axis, then x index, then y
index, omitting faces with no fluid neighbor. Body velocities follow in increasing
body-ID order, as world Vx, world Vy, omega.

Input export loads unchanged reference geometry/class definitions through an AST
selection solely to count velocity DOFs. It invokes neither the projection
method nor analytical output calculations. NumPy generates the random values.
This inputs-only operation can run without SciPy:

```sh
PYTHONPATH=.reference-p1pf python3 prepare_p1pf_reference.py --inputs-only
```

That command records `INPUT_EXPORT_ONLY`, not a reference projection PASS.
