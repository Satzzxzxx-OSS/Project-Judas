# Judas FTFT4B event-timing research

Read RESEARCH_REPORT.md first. This is NOT a production implementation prompt.

The package contains executed component references, source-style counterexamples,
and a not-yet-executed current-engine witness. It does not claim a completed
3D engine collision scheduler or performance acceptance.

Run independent checks:

    python code/run_all.py

Dependencies: Python, numpy, mpmath, g++ with C++17. See requirements.txt for the
versions used. Research results deliberately preserve failed candidate evidence.
An executable's successful exit is a reproduction check, NOT an FTFT4B PASS.

No runtime binaries are shipped. code/production_energy_witness.cpp needs actual
Judas/GLM linkage and was NOT run here. It returns failure for measured energy
creation rather than printing an unconditional PASS.

The reference/source_snapshot files are the relevant parts of the earlier
user-provided source export. Check results/source_scope.json before attributing
any finding to the final 422fd27 implementation. Some source files match its
supplied fingerprints; optimized ContactSolver/PhysicsWorld do not.
