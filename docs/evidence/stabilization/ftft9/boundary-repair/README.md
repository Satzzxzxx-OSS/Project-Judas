# FTFT9 boundary repair — current development

This continuation implements the operator's hard physical-solid exclusion / soft
particle-clearance rule. It does not replace the PBF model or integrate any
research prototype. Historical rejected expanded-skin escape and gradient work
is preserved separately.

The first gate is deliberately limited to the unchanged closing-gap witness,
unchanged `half_025` and existing fluid/rigid coupling suite. Run from the repository
root:

```
python3 docs/evidence/stabilization/ftft9/boundary-repair/run_first_gate.py
```

The runner records exact commands, raw exits, wall times and source hashes. The
small witness retains its original physical configuration. Its existing physical
exclusion assertion is supplemented by passive finite-state/particle-count
observations; those observations do not modify the simulation.

## Executed boundary gate

The [fourth-gate record](fourth-gate/results.json) passed all three required
checks against unchanged source during execution:

- Closing-gap witness: one particle retained, final position and velocity zero,
  outside both actual solids, finite state.
- `half_025`: 731 checks / 480 steps, zero failures; independently observed
  immersion 0.5790364583. No retuning.
- Existing fluid/rigid coupling suite: PASS, 28 assertions in seven sections.

Earlier first/second/third-gate failures remain alongside that record. The repair
preserves legacy preferred-clearance arithmetic where feasible, clips conflicting
soft corrections against actual solids, and uses the resolved rigid motion path.
Physical recovery is checked separately from preferred skin clearance; geometric
correction is excluded from reconstructed particle velocity. See the
[hard-solid/soft-skin mechanism](../hard-solid-soft-skin/METHOD.md) and
[resolved-motion integration](../resolved-boundary-motion/phase-split/IMPLEMENTATION.md).

This is acceptance of the demonstrated boundary blocker fix, **not a complete
FTFT9 pass**. Subsequent sampler/player changes and the final integrated gate
have separate evidence. No commit or push is authorized by this continuation.
