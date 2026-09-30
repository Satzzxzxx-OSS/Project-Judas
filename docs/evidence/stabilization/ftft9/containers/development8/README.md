# Executed container and batch acceptance

`run_metadata.json` records exact commands, exit codes, wall times and source/binary SHA-256 values. The linked executable uses the actual compiled engine; binaries are excluded from evidence. Source snapshots are preserved under `source/`.

- Auxiliary contact checks: **52/52 PASS**, exit 0.
- Actual serialized Scene → RuntimeWorld → GameSession → StepPlayedWorld fixtures: **8 cases, 3,124 steps including preparation, 7,014 checks, zero failures**, exit 0.
- Four supported tank load cases: relative error at most **6.527e-6**, all initial finite liquid retained. Supported system momentum includes ordinary external floor reactions; those totals are not unexplained momentum residuals. Auxiliary support reaction is reported independently.
- Two supported-release common-free-fall cases: liquid/tank relative drift **0.160910 / 0.056296 m** in two seconds, body ballistic error **0.076779 / 0.000004896 m**; whole-system momentum residual **0.13110 / 0.006207 kg·m/s** including actual gravity impulse. These remain approximate 30 Hz liquid / 60 Hz rigid trajectories.
- Two sealed zero-gravity impulse tanks: all liquid retained; momentum residual **8.823e-5 / 3.714e-5 kg·m/s**. Maximum kinetic energy **1.048407 / 1.212502 J** includes measured preparation energy plus the external impulse work.
- Maximum reported median executed particle step among these fixtures: **5.041 ms**; timings are measurements of this run, not a paired performance claim.

Raw open-zero-gravity cup spill and over-compressed startup diagnostics are preserved in previous development directories and remain reproducible via `--open-zero-g` / `--raw-initial`. An open cup with outward liquid velocity in zero gravity does not have a 99% retention guarantee. Acceptance uses an ordinary resolved top wall for the zero-gravity tank; no production object-name or fixture classifier is introduced. Supported and free-fall cups remain open.

Last-batch observations show no PGS caps and at most 20 PGS iterations. Some redundant-manifold normal accelerations decline safely and ordinary PGS completes; see the logged matrix-product and breakdown columns. These columns report the last auxiliary batch in the fluid step, not an aggregate over every internal fluid substep.
