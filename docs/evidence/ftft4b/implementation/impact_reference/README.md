# FTFT4B contact-response research — NOT a production patch

Read `RESEARCH_REPORT.md` and `SOURCES.md` first.

The current production defects were verified by the operator's agent at physical
source checkpoint `422fd275...`; diagnostic commit `c43af4c...` does not change
that physics. User-supplied current-engine results are preserved under `reference/`.
This package adds independent reference research, not a fresh full Judas build.

## Reproduce

Python 3.10+ with NumPy and SciPy, plus a C++17 compiler:

```sh
python code/run_all.py
```

To also rerun the deliberately rejected midpoint prototype from its unchanged
source snapshots:

```sh
python code/run_all.py --replay-rejected
```

The runner records its actual interpreter, library/compiler versions, commands,
source hashes and outcomes in `results/RUN_METADATA.json`. Build output goes
under `.build/`, excluded from the distribution. Numerical results are in JSON;
stdout/stderr are preserved beside them.

A successful runner means the declared research results were reproduced. It does
NOT mean FTFT4B is fixed. In particular, the no-capture event-budget exit and the
rejected midpoint candidate are explicitly NOT physical passes.

## Programs

- `event_progress.py`: exact rational 1D closing/resting-first normal impact
  sequence; reproduces the previous 360 cases, the three former 128-event exits,
  the current three-body counterexample, and a no-capture capped experiment.
- `normal_blocks.py`: fixed-geometry frictionless 3D normal blocks; exhaustive
  active sets are an oracle, not an engine implementation strategy.
- `energetic_point.py`: planar single-contact energetic restitution with analytic
  sliding/sticking interval endpoints; includes Newton/Coulomb counterexamples.
- `spatial_point.py`: own adaptive 3D *single-contact* impulse discretization;
  matches energetic work with backward Coulomb cone subproblems and explicit
  nonpositive friction work along each accepted linear segment.
- `spatial_ode_oracle.py`: separate DOP853 continuous Routh-ODE reference for
  strictly sliding 3D cases, compared at three tolerances.
- `coupled_spatial_experiment.py`: twelve small approaching-contact graphs using
  the energetic point operation; exploratory sequence, not general simultaneity
  or termination acceptance.
- `scalar_friction_probe.cpp`: compiled scalar specialization of the supplied
  normal/tangent update. This is NOT the complete production ContactSolver.
- `production_single_contact_witness.cpp`: optional actual-ContactSolver adapter.
  **Not compiled/run here**; GLM was unavailable. It is not an additional request
  to modify or audit the user's repository.

## Deliberate exclusions

No production code modification; no whole-world event scheduler; no 3D
multi-contact friction law; no general event-termination or engine performance
claim; no modifications to FTFT1–3, FTFT4A, materials or fluid prototypes.

## Evidence integrity

`SHA256.json` lists all distributed files except itself. Original failed
midpoint snapshots remain distinct from the later backward-cone candidate.
No full papers are redistributed; `SOURCES.md` identifies the primary sources.
