# R1-P1 isolated fluid prototype

Current checkpoint: **P1-C-M fixed-grid mass-consistent component-momentum transport**. One executable runs the unchanged 25-case frozen-WY JSGT fraction gate with momentum attached, followed by the new momentum validation family.

Run from any directory:

```sh
bash /home/conner/Documents/GitHub/Project-Judas/prototypes/fluid_coupling/r1_p1/run_p1cm.sh
```

The script builds locally, runs the compiled validations, checks fraction regression against the preserved baseline, and records logs, source snapshots and hashes under `evidence/p1cm/current/`. No external Python packages are needed for the C++ validation or report generation.

- `P1CM_METHOD.md`: mass/momentum equations and code mapping.
- `P1CM_RESULTS.md`: current executed results and limitations.
- `METHOD.md`, `P1C_METHOD.md`, `P1C_RESULTS.md`: preserved fraction-method description and acceptance history.
- `evidence/p1cm/baseline/`: accepted sources, prior results, and the pre-edit rerun.
- `evidence/p1cm/handoff/`: supplied evidence, preserved unchanged.
- `evidence/p1cm/reference_rerun/`: independently rerun supporting Python fixed-grid checks.

Pressure, moving solids, cut cells, triple-line integration, P1-D/E and production integration remain excluded. P1-C-M does not complete integrated P1. This Git checkpoint covers P1-C/P1-C-M only; it is not a milestone tag or production integration. See `EVIDENCE_INDEX.md` for dependencies, archived failures and evidence provenance.
