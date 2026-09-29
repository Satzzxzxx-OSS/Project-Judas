# FTFT4B selected-policy contradiction

Read [RESULTS.md](RESULTS.md). This directory is **not passing FTFT4B evidence**. The approved compression/Poisson-expansion rule creates energy in a closed, exactly touching two-contact fixture with mixed restitution.

## Reproduce

From the repository root, with existing Judas C++17/GLM/CMake dependencies:

```sh
python3 scripts/ftft4b_policy_validation.py --output docs/evidence/ftft4b/policy-replay
```

Choose a fresh output directory. No NumPy/SciPy dependency for this runner or the exact proof. The runner uses Release and `-ffp-contract=off`, builds real engine targets, and returns zero only when the **recorded blocked-state measurements** have completed. The policy witness and exact rational proof intentionally retain physical failure exit code 1. Existing world energy and full temporal checks also retain their failures because no event/impact repair was integrated.

Direct counterexample, after build:

```sh
build/judas_poisson_policy_witness
python3 docs/evidence/ftft4b/implementation/exact_poisson_check.py
```

Both exit 1 for the energy contradiction. The C++ witness consumes actual `PhysicsWorld` shape/mass/inertia and production narrowphase. Its response is the nonsingular two-normal specialization of the selected equations, **not** a completed world-step PLUS implementation. The exact proof uses those represented physical inputs and Fraction arithmetic; it does not use C++ final states as oracle inputs.

## Files

- `initial.json`: clean checkpoint, full starting tracked fingerprints and original final-handoff ZIP hash.
- `ASTRA_FTFT4B.md`, `RESEARCH_REPORT.md`, `REFERENCES.md`, `SHA256.json`: unchanged final handoff documents/manifest.
- `impact_reference/`, `contact_reference/`: supplied prior research source/results, not newly executed production evidence; older production snapshots excluded.
- `validation/`: final executed commands, raw witness traces, exact proof, existing regression logs and tested source fingerprints.
- `policy-first-run.json`, `policy-fixture-first.cpp`: rejected decimal-coordinate setup (not a physical result).
- `exact_poisson_check.py`: independent exact-rational equations and mechanics.
- `final-verification.json`, `SOURCE_SHA256.json`, `git-status.txt`: final protection/fingerprint/status audit.

Stage A's partial motion storage is not a full event ledger integration. Player sweeps still consume old endpoint history. No historical baseline files, protected fluid prototypes, FTFT1–3 implementation/evidence, ContactSolver, robust geometry, or quaternion integrator have been changed. No timing repair is claimed.

Selected paper: Uchida, Sherman & Delp (2015), DOI [10.1098/rspa.2014.0859](https://doi.org/10.1098/rspa.2014.0859), normal equations (2.3)/(2.4), Part II/table 1. Accessible XML used to check these selected equations: [Europe PMC](https://www.ebi.ac.uk/europepmc/webservices/rest/PMC4984984/fullTextXML). No Simbody implementation code was copied. No new model-selection research was performed.
