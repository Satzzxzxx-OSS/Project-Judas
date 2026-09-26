# R1-P1 isolated fluid prototype

Transport checkpoint: **P1-C-M fixed-grid mass-consistent component-momentum transport**. This checkpoint also includes the separate P1-PF frozen-geometry component described below. One executable runs the unchanged 25-case frozen-WY JSGT fraction gate with momentum attached, followed by the new momentum validation family.

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


## P1-PF component checkpoint

The P1-PF checkpoint adds a separate optional target for simultaneous
multi-cell fluid pressure and rigid-body linear/angular velocity projection on
frozen resolved geometry. Read `P1PF_METHOD.md` and `P1PF_RESULTS.md`.

```sh
bash prototypes/fluid_coupling/r1_p1/run_p1pf.sh
```

Run that command from the repository root. It preserves/reruns the 55 transport
checks and executes 42 projection cases, five negative controls and six adapters
using actual compiled transport. LAPACK is required only for the new target.
No body geometry is advanced; this is not integrated P1-D/E. Source/evidence lives
under `evidence/p1pf/`; the original checkpoint/baseline remains preserved.

The recorded results and run metadata describe the tested state before this Git
checkpoint. Their HEAD/status/action fields are preserved execution provenance.
Checkpoint preparation changes only this README, STATUS and EVIDENCE_INDEX; tested
source and numerical evidence remain unchanged.
