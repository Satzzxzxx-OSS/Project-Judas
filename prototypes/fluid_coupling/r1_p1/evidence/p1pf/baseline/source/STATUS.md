# R1 prototype status

P1-C is preserved; the current task is P1-C-M fixed-grid mass/momentum transport. `P1CM_RESULTS.md` and `evidence/p1cm/current/RUN_METADATA.json` record the executed current result.

Production reference: `48385a78afb2930744f2b87ad6aa55452f18ae18`. All implementation and evidence remain under `prototypes/fluid_coupling/r1_p1/`. The authorized Git checkpoint adds this isolated prototype; production files are unchanged. No milestone tag is part of the checkpoint.

The accepted P1-C sources were fingerprinted before editing and rerun with all 25 non-timing result rows identical. They are preserved under `evidence/p1cm/baseline/`. The current fraction algorithm exposes stored geometric transfers to momentum; new periodic index support is opt-in and does not change the legacy fixtures.

The compiled mass/momentum path adds positive-density fine mass, staggered restriction including physical boundary half-duals, per-segment upwind momentum, frozen source velocity and conserved branch combination. It does not solve pressure or evolve an advector from output momentum.

## Still incomplete/outside this checkpoint

Moving-solid occupancy, conservative triple-interface transport, cut-cell pressure/body coupling, buoyancy and swimming are absent. Historical cut-cell, triple-point and projection headers remain inactive. P1-D/E were not run. Integrated P1 and P2 remain unearned by this work.
