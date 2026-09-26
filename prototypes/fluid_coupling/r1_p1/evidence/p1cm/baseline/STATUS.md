# R1-P1 status

## Current checkpoint: P1-C PASS, integrated P1 incomplete

On 2026-09-26, frozen-WY JSGT passed the complete requested uniform family and both affine and nonlinear divergence-free nonuniform families. See `P1C_RESULTS.md` for raw metrics and limitations. This acceptance concerns prescribed-velocity liquid/gas fraction transport only.

All work remains under `prototypes/fluid_coupling/r1_p1/`. Production HEAD remains `48385a78afb2930744f2b87ad6aa55452f18ae18`. No production files, commits, tags or milestone claims changed.

## What this checkpoint does establish

- One original binary WY correction field is frozen across the complete timestep.
- Actual JSGT final flux averaging remains active.
- Predictors, both complete virtual sweep orders, and final fractions stay within the unchanged numerical bounds tolerance.
- Shared-face budgets close, including independently checked nonzero boundary export.
- The JSGT/WY mean identity closes under measured discrete incompressibility.
- All six 24/48/96 geometric error sequences decrease.

## What remains unearned

- Moving solid occupancy and conservative triple-interface transport are not integrated.
- Existing `cutcell.hpp`, `triple_point.hpp`, `coupled_projection.hpp` and `hydrostatic.hpp` are inactive historical components. Their isolated checks do not establish integrated acceptance.
- No pressure system is assembled from active moving cut geometry.
- Consistent liquid momentum transport and pressure/body coupling are absent from this executable.
- P1-A/B have not been rerun through an integrated liquid/body method.
- P1-D/E were not run.
- P2 and production integration remain outside this checkpoint.

The previous continuous-C predictor failure is preserved in `results/continuous_predictor_archive/`; it was resolved by an explicitly authorized method change, not by changing fixture geometry or relaxing assertions. Work stops at the P1-C report for operator review before further integration.
