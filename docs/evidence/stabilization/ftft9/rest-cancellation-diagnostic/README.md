# Final bounded settling diagnosis: pressure reconstruction cancellation

Passive diagnostics only; no fluid physics or acceptance threshold changes.
At each PBF density iteration, record the actual represented position change
`double(position_after) - double(position_before)`, then sum in double over
that substep. Compare this with the existing float reconstruction
`position - unconstrained_position - solid_corrections`, and divide the
difference by the substep duration.

`pressureReconstructionSamples` counts particle/substep observations over the
last executed fluid step; maximum and RMS residual fields use m/s. Existing
authoritative velocity reconstruction remains unchanged. Counters reset on
an executed step and on Clear, like existing geometry diagnostics.

The observations are to be folded into the single planned current body family;
no second player smoke or separate acceptance run is required. A residue much
smaller than the measured PBF agitation disproves the cancellation hypothesis
and does not authorize a new damping/iteration/pressure model.

## Observed result

The single combined body family completed all eleven cases (2,640 measured
ordinary rows, 1,320 executed fluid rows). The diagnostic samples are from
the measured post-release interval, not from its preceding warmup. Held
frames are excluded rather than counting the same fluid observations twice.

Maximum cancellation-velocity residue across the complete family was
1.67638e-6 m/s; maximum per-step RMS residue was 3.67076e-8 m/s. In neutral
.25 m, they were 2.61934e-9 and 6.21543e-11 m/s, compared with preparation
particle mean speed .0727231 m/s. In zero gravity both residuals were exactly
zero while its preparation mean speed was .0972153 m/s.

The measured cancellation hypothesis is disproved as an explanation for the
material agitation in this continuing pool operation. The original velocity
formula is retained. The strict .05 m/s pool-rest requirement remains failed
for .25 m cases, zero gravity and the small-body case; no threshold is changed.
After two bounded investigations there is no demonstrated ordinary coding
error justifying a third autonomous fluid-model change. All raw per-case
values and source CSV fingerprints are in combined-family-analysis.json.
