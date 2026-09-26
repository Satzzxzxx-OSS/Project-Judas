# Judas P1-PF Astra handoff

1. ASTRA_TASK.md is the complete implementation assignment.
2. EVIDENCE_RECHECK.md distinguishes original and newly executed reference results.
3. reference/RESEARCH_NOTE.md gives the derivation, independent oracles and limits.
4. SOURCES.md identifies primary papers and numerical-library documentation.
5. SHA256.json fingerprints the package files; it does not fingerprint the user's repo.

Run reference checks in a COPY of reference/, since scripts write JSON results:

    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python projection_checks.py
    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python adapter_reference_checks.py

Dependencies: NumPy and SciPy. The adapter loads only fixed-grid functions from
compatibility_checks.py; do not run that companion's unrelated geometry main.

The original research archive is retained unchanged under evidence/original/.

This is a frozen/resolved-geometry component assignment. It is not a full moving
solid/free-surface timestep or a claim of P1-D/E completion.
