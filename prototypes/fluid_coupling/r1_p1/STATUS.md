# R1 prototype status

P1-C/P1-C-M transport is preserved. This checkpoint also records the separately validated P1-PF frozen-geometry pressure/body component. `P1CM_RESULTS.md` and `evidence/p1cm/current/RUN_METADATA.json` record the executed current result.

Production reference: `48385a78afb2930744f2b87ad6aa55452f18ae18`. All implementation and evidence remain under `prototypes/fluid_coupling/r1_p1/`. The authorized Git checkpoint adds this isolated prototype; production files are unchanged. No milestone tag is part of the checkpoint.

The accepted P1-C sources were fingerprinted before editing and rerun with all 25 non-timing result rows identical. They are preserved under `evidence/p1cm/baseline/`. The current fraction algorithm exposes stored geometric transfers to momentum; new periodic index support is opt-in and does not change the legacy fixtures.

The compiled mass/momentum path adds positive-density fine mass, staggered restriction including physical boundary half-duals, per-segment upwind momentum, frozen source velocity and conserved branch combination. It does not solve pressure or evolve an advector from output momentum.

## Still incomplete/outside this checkpoint

Moving-solid occupancy, conservative triple-interface transport, cut-cell pressure/body coupling, buoyancy and swimming are absent. Historical cut-cell, triple-point and projection headers remain inactive. P1-D/E were not run. Integrated P1 and P2 remain unearned by this work.


## P1-PF checkpoint

An isolated optional frozen/resolved-geometry projection component passed 42
projection cases, five negative controls, six actual-transport adapters and nine
malformed-input checks. Its pre-checkpoint validation ran at HEAD
`0d73f47206cc34674bb9fbbce2cb438f38ce9f22`; the original 55 transport runs also
passed with unchanged numerical evidence. See `P1PF_RESULTS.md`
for actual executed counts and `P1PF_METHOD.md` for the exact scope. It includes
simultaneous body velocity coupling but never advances body geometry. The older
`coupled_projection.hpp` remains inactive. This operator-authorized Git checkpoint
records P1-PF only; the completed P1-C/P1-C-M checkpoint remains intact. No
production integration or milestone tag is part of this checkpoint.
