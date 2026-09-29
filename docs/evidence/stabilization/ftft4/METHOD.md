# FTFT4 stabilization impact policy

**CLOSED with documented approximations.** The implementation and executed
acceptance evidence are summarized in [RESULTS.md](RESULTS.md).
Starting committed baseline: `c43af4c0b65c597bff7fd03bed55173f5c483139`.

## Selected mechanism

1. Apply external force/torque velocity updates once per fixed step. Keep
   the existing accumulated normal/tangent impulse solver and warm starts
   for persistent support, stacks, slopes and sustained friction.
2. Use preserved signed-separation geometry to search for new separated
   impacts during remaining step time. Evaluate motion from anchored
   segments; advance the affected island to the event, solve, and refresh
   changed trajectories and broadphase candidates. Consume all physical
   time. An event elsewhere must not change an unaffected body's endpoint.
3. One isolated material closing contact retains authored restitution and
   Coulomb friction. Mechanically connected multiple/induced/persistent or
   mixed-restitution impact contacts use **inelastic normal response**.
   This deliberately sacrifices coupled bounce fidelity for stability.
4. Measure dynamic kinetic energy before/after an event, including spin.
   Account for prescribed moving-boundary work separately. The candidate
   tolerance is `64 * float epsilon * max(1 J, pre-impact energy,
   abs(boundary work))`; it is an engineering tolerance, not a rounding
   proof. On failure, restore velocities and retry with restitution and
   impact friction both zero; report `impactSafetyFallback`.
5. Retain the existing 0.5 m/s restitution threshold. After more than 16
   impact events for a body/island in one step, capture connected contacts
   inelastically, consume remaining time and report `impactEventCapFallback`.
   The fallback does not authorize freezing in midair or dropping time.
6. Conservative advancement is limited to 64 iterations per pair query.
   Rotating grazing features that exhaust that bound use deterministic,
   pair-local temporal sampling of the remaining anchored trajectory:
   at least 16 intervals, travel no greater than 1/16 of the smaller
   primitive radius/half-extent, and combined angular increment no greater
   than 1/256 radian, subject to a 65,536-sample safety cap. Actual zero-margin
   geometry alone permits an impulse; a detected crossing is bisected.
   `impactSamplingFallbacks`, `impactSamplingTests`,
   `impactSamplingResolutionCaps`, and `impactSamplingMaxInterval` expose
   this approximation. Proximity features without a certified positive
   advancement gap (including terrain's strict zero-margin equality case)
   use the same fallback and count `impactUncertifiedAdvances`; the proximity
   result itself never authorizes an impulse.
   A contact interval shorter than the sample interval
   may be missed. This is finite-resolution CCD, not a completeness proof
   for arbitrarily thin or grazing rotating features. It does not freeze a
   body across a positive gap or subdivide unrelated bodies.
7. Keep ordered motion segments for dynamic/player sweeps and bounds.
   The ledger is transient and excluded from scene persistence. ResetBody
   and door resets remain discontinuous pose changes, not swept trajectories.

## Source responsibilities

- `src/PhysicsWorld.cpp`: `ResolveInitialImpacts`, `ImpactTime`,
  `AdvanceImpacts`, the ordinary step, bounds and player-query integration.
  Event times are cached only for the current step and unchanged pair
  trajectories; changed velocities invalidate incident searches and query
  the actual broadphase for new candidates. Geometric/contact storage is
  reused. Initial impact rebuilding uses the existing ContactSolver API;
  its numerical expressions and persistent solver remain unchanged.
  These are candidate paths under test; a function name is not acceptance.
- `src/RigidMotion.h`: fixed-anchor drift observations and new segments
  after actual velocity changes. The original quaternion arithmetic remains.
- `src/ImpactSolver.{h,cpp}`: event classification, reuse of ContactSolver,
  shared contact impulses, budget measurement and restored-state retry.
- `src/ContactSolver.{h,cpp}`: existing persistent accumulation, warm starts
  and Coulomb friction. FTFT4A predicates, signed gaps and anchors stay intact.
- `scripts/ftft4_closure_validation.py`: current-engine witnesses,
  regressions and paired performance entry points with fresh output paths.

No PLUS/Poisson rounds, fixture names, mass exclusions, hidden damping,
fast-math or analytic contact-force substitutions are part of this policy.
The [rejected policy and exact contradiction](../../ftft4b/implementation/RESULTS.md)
and [old engine failure baseline](../../ftft4b/baseline/RESULTS.md) remain
historical evidence, including their raw nonzero witness exits.

## Closure gates — passed

- Positive-gap restitution-zero impact reaches contact; restitution-0.5
  impact uses the physical event time and remaining drift.
- Induced chain collision is discovered after trajectory changes.
- The original three-sphere energy witness and transformed/order variants
  satisfy energy and momentum budgets; simultaneous inelastic response is
  allowed. Report actual safety/event-cap fallback counts.
- Preserve geometry, enclosure/tree oracle, stack/slope/platform/friction,
  player sweep and lifecycle regressions plus FTFT1–3/prototype protections.
- Seven interleaved current/c43 measurements: 1,500-crate median whole
  physics step **≤15 ms** and ratio **≤1.10**. Include cache preparation;
  report construction, first/all/steady steps and teardown. Report the
  exhaustive correctness oracle separately from production physics.
- Repeat rotating, compound, exact-touch and moving-support workloads.
  Small/mixed allocation-pass gains are not a broad speedup. The older
  approximately 4 ms crate result belongs to less-robust geometry.

Existing float input/range limits, approximate terrain geometry and
unsupported extreme cases are unchanged unless new evidence explicitly
establishes otherwise. No claim of exact general rigid-body material
physics or live origin rebasing is added.
