# Supported-container contact-phase witness

The actual Scene → RuntimeWorld → StepPlayedWorld fixtures were run with the bounded auxiliary contact solve. Eight cases, 3,124 ordinary steps (including preparation), 7,014 checks: one remaining failure. The original open zero-gravity modes remain available using `--open-zero-g` or `--raw-initial`; their previous outputs are preserved. The default zero-gravity acceptance tank now has an ordinary resolved top wall because an open zero-gravity cup with outward liquid velocity has no physical retention guarantee. No production cavity classifier changed for this fixture.

Four supported load/retention cases passed. Both sealed zero-gravity impulse cases passed retention and momentum accounting: residuals 0.000135458 and 0.0000391599 kg·m/s; maximum energy 1.04970 and 1.21683 J includes measured prepared energy plus 1 J input. Prescribed impulse work is computed in the actual laboratory frame, `J·v + |J|²/(2M)`; the relative co-moving increment remains 1 J.

The 0.15 m particle-spacing free-fall case retains all liquid and passes relative displacement, but the tank endpoint differs by 0.46179 m from the unforced-body ballistic oracle. The initial fluid is not settled relative to the tank: RMS 0.29405 m/s and total kinetic energy 21.4495 J remain after preparation. The 0.20 m case passes.

## Exact support-geometry evidence

For the supported 200 kg tank at spacing 0.15 m, the floor top and tank bottom are both -0.1 m at body position y=0; body y therefore measures the axis-aligned face gap independently of the narrowphase. In `prepared/supported-s0.150000-m200.csv`:

| Step | Pre-step gap (m) | Auxiliary rigid rows | External batch support impulse y | Mean liquid vy |
|---:|---:|---:|---:|---:|
| 350 | 0.000136827060487 | 0 | 0 | -0.461345564 |
| 352 | 0.000000007006558 | 3 | 424.761535645 | 0.001903176 |
| 354 | 0.000000009313226 | 1 | 46.422035217 | -0.205091904 |
| 356 | -0.000000004251611 | 3 | 296.870666504 | 0.000374901 |

No iteration cap occurred in these rows, and the velocity residuals are below the declared convergence criterion. This establishes support geometry/phase mismatch, not insufficient convergence: the zero-margin fluid batch drops the real persistent floor constraint while the following ordinary rigid step retains/recovers floor support. The production repair must preserve genuine signed gaps and ordinary impact handling for new contacts.

Supported full-world momentum columns include external ordinary PhysicsWorld floor reactions that the fixture does not measure separately. They must not be described as unexplained momentum creation. The CSV additionally records the external support impulse from the auxiliary batch alone.
