# Resolved rigid motion / production liquid phase split

This is an implementation snapshot, not passing validation evidence. `before/`
preserves pre-edit sources; `implemented/` preserves the first written candidate.
The parent runs the unchanged three-case integration gate after shared source freeze.

1. Existing rigid gravity velocity preparation remains once per ordinary tick.
2. `PrepareRigidStep` observes the latest liquid field and applies hydrostatic
   and drag forces before the ordinary `PhysicsWorld::Step`. Drag extrapolates
   the held liquid velocity by `pending + dt`; it does not apply gravity again.
   The first tick has no fabricated initial liquid acceleration or hydrostatic field.
3. `AdvanceResolvedLiquid` copies that completed rigid step's generation-checked
   motion segments, with their original anchors and physical-time offsets.
   The endpoint predictor is removed. Shape geometry and cavity identification
   sample the same copied path.
4. The existing liquid cadence and substep count are preserved. First advancement
   consumes one rigid tick; subsequent default 30-Hz updates consume two 60-Hz
   ticks. Slower valid authored rates retain only their finite cadence window;
   history is cleared after each executed liquid update and dead generations
   are pruned. Cached capacity and used frames/segments are observable counters.
5. Resets break the affected body's cached path. No interpolated old-to-new
   teleport trajectory is introduced. Compound children use parent paths and
   their explicit shape-local centre offsets.
6. Existing finite-mass cavity velocity exchange runs after resolved rigid motion,
   reusing cached support and the approved next-interval `dt` gap-closing rule.
   This is an operator-split approximation: it does not rewind poses or replay
   gravity/impact integration. Contained impulse and support impulse remain
   measured once; exterior particle reaction remains deliberately unapplied.
7. Each liquid substep samples its actual rigid path endpoints. Conservative
   swept bounds include intermediate segment endpoints and compound rotational
   reach. Collision between these samples remains the existing finite temporal
   approximation; a conservative candidate bound is not an exact path collision
   proof. Anchored angular evaluation calls the existing position integrator.
8. Player sampling follows liquid advancement and extrapolates the held field by
   its remaining `pending` age. Tick timing aggregates preparation and advancement
   costs, including path copies, geometry preparation and field rebuilding.

No kernels, solver laws, restitution, force integration or fixture assertions
were changed by this phase-split work.
