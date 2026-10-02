# M45 candidate results

Starting HEAD: `fa2c281c399a848033768f253e277742625078eb`. Uncommitted candidate; human visual/physical validation pending.

- One clean Release configure/build: PASS, zero compiler warnings.
- Existing complete production gate: **83 suites PASS**, no failed commands.
- Real async Application integration: **12 cases / 246 assertions PASS**.
- Focused M45: **19 solver + 14 authoring/prefab/persistence + 8 Application/JS checks PASS**. These targets also ran in the final production gate.
- Demo editor Play/Stop: PASS; authored scene identical after Stop.
- Demo standalone: PASS.
- M38 export and moved package from unrelated `/tmp` working directory: PASS; JS validated live joints and motor control in the normal application.
- Exported package: 7,035,806 bytes (6.710 MiB); export 0.019 s (not a universal performance guarantee).
- 100 independent constraints, 120 ordinary physics steps: **0.166682 ms mean whole-step** in the clean final run (focused pre-gate run 0.184256 ms). Sanity measurement only, not a large articulated assembly guarantee.
- Complete automated gate wall time: 663.251 s, including clean build and existing expensive fluid regressions. This is validation wall time, not engine frame cost.
- 37 source/test/script/demo/build fingerprints: verified equal to the tested final snapshot. See `final/SOURCE_SHA256.json`.
- Protected FTFT/P1 and M33–M44 evidence/prototypes: unchanged. No commits, pushes or tags.

## Reproduction and raw evidence

`python3 scripts/m45_validation.py` uses the current production infrastructure and refuses to overwrite existing `final/`. Raw commands, exit codes, timings and compiler output are under `final/`; focused development logs are under `focused/`. The three malformed demo-generation logs remain preserved alongside the corrected application pass. No physical assertion was weakened to correct those fixture syntax defects.

## Accepted candidate limitations

See `docs/JOINTS.md`: iterative stabilization, wrapped hinge limits, unsuitable initially antiparallel hinge frames/extreme errors, no exact articulated-body solution, and transient runtime motor/warm-start settings. Existing kinematic M16 door evidence is retained; the new demo uses an ordinary physical hinge.

## Human checklist

1. Fixed assembly stays connected.
2. Hinge swings only around its authored axis.
3. Limited hinge stops at its limits.
4. Ball/socket moves freely without separating.
5. Slider moves only along its axis.
6. Motor visibly drives its constrained body (G reverses).
7. Oblique hinge behaves sensibly without a world-up assumption.
8. Physical door operates correctly.
9. P spawns an independent prefab assembly; exported standalone matches.

Open `projects/joint_demo/joint_demo.judasproj` in the editor. Human acceptance has not been claimed from automated/headless execution.

## Human follow-up — mouse capture

Operator reported cursor escaping during look. The real desktop Application check reproduced loss of SDL relative mode across refocus: **11 checks / 1 failure** (`focused/capture-visible-before.log`). Window now reapplies requested capture on focus gain, explicitly grabs the window, and reports visible-window backend capture failures. Pause still releases capture.

After the repair, the same real desktop check passed **11/11** (`capture-visible-after.log`), and affected scene-transition coverage passed **23/23** (`capture-transitions.log`). No full production rerun was performed for this narrow Window focus fix. The original complete gate remains the pre-follow-up result; final corrected fingerprints are `CAPTURE_FOLLOWUP_SOURCE_SHA256.json` (38 files).

The first headless capture attempt (`capture-before.log`) showed two failures because hidden/offscreen windows cannot establish actual desktop cursor capture; those physical assertions therefore execute in explicitly visible desktop mode, not through a fake backend. Original failure evidence is preserved.
