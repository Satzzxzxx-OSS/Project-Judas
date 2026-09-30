# Auxiliary contained-particle contact solve

This is a finite-mass inelastic contact adapter, not a fluid-pressure projection. Ordinary `PhysicsWorld::Step`, `ContactSolver`, impact timing, materials and rigid warm-cache data are unchanged.

## Shared constraints

Each contained particle is a temporary point mass with zero inverse rotational inertia. Contacted dynamic bodies and their connected physical support island are copied; static supports terminate graph expansion. The same prepared contact normal and local anchors produce particle impulse, rigid reaction and torque. One-way exterior contacts use their prescribed boundary velocity and do not alter the real owner. Only final velocities are committed. Original poses, force accumulators and the ordinary warm cache are preserved.

A true touching rigid row uses current robust geometry. Following explicit operator authorization, a genuinely separated row may participate only if the main solver already recognizes its body generations, primitive pair, nearest unused local anchor and normal as persistent support. Its signed gap is preserved and `ContactSolver::Prepare(remainingRigidDt)` constructs the existing target. No new separated pair is admitted. The existing fast-closing target branch is unchanged; this adapter does not claim a new universal impact law.

## Normal algebraic acceleration

The ordinary auxiliary accumulated-impulse solve runs ten iterations first. Its existing rows define normal Jacobian `J`, block inverse mass/inertia `M^-1`, normal target `b`, and accumulated normal impulse `lambda >= 0`. Normal acceleration uses exactly:

```
A = J M^-1 J^T
r = b - J v
A_active delta = r_active
```

The active set contains positive accumulated normal impulses or currently violating inactive rows. `A` is applied without a dense matrix: accumulate signed linear/angular contact impulses on participating bodies; multiply by their actual inverse masses/world inertias; project the resulting point velocities back onto the same normals. Prepared lever arms and inertias are used directly. Double precision Jacobi-preconditioned conjugate gradients is limited to the active row count, with residual target `1e-10 * max(1, initial point-speed magnitude)`. No diagonal regularization or artificial stiffness is introduced. Zero/nonfinite curvature aborts this optional acceleration safely.

The candidate step fraction is the largest value at most one that preserves nonnegative accumulated normal impulses. It must strictly decrease the frozen-tangent quadratic: `alpha*r^T*delta - alpha²*delta^T*A*delta/2 > 0`. Otherwise it is discarded. This impulse-cone constraint is the existing contact law; there is no velocity clamp.

To apply a successful candidate, temporary body velocities are restored to their original values and the SAME ContactSolver rows are rebuilt with the candidate total normal impulses and current tangent impulses as warm values. The original solver resumes its Coulomb friction iterations. Thus only final actually applied row impulses enter particle/body/support diagnostics; tentative iterations are never counted as extra external reaction.

At most 64 total auxiliary PGS iterations are executed in ten-iteration blocks plus a final four. Convergence means excess closing beyond the existing row target is at most `1e-5 * max(1, initial point-speed magnitude)`. Matrix products, accepted accelerations, declined curvature cases, iteration count and remaining residual are observable. These are finite numerical budgets, not claims of exact complementarity for every possible condition.

## Executed evidence

- Historical 100:1 particle/body supported contact failed at 64 plain PGS iterations; source and raw failure remain in `development4` and the resumed unchanged run `../containers/development7/batch.log`.
- With the algebraic acceleration, the unchanged signed-gap fixture converges in 20 PGS iterations and four matrix products. Independent constrained two-mass mechanics predicts its impulse and velocity. Kinetic energy drops from 200.0018 J to approximately 0.181795 J.
- `../containers/development8/batch.log`: 52 checks pass, including same-impulse momentum/torque, fixed-support energy, force-accumulator preservation, unchanged poses, stale-handle rejection, uncached-gap exclusion and the stiff independent budget oracle.
- All eight actual scene/session/container fixtures also pass in development8. Redundant manifold rows sometimes cause optional PCG to decline; ordinary PGS still converges without reaching its cap in recorded last-batch observations.

The energy tests apply to their closed/static-boundary fixtures. Quadratic descent alone is not a proof of full fluid energy conservation, especially with moving prescribed boundaries or the PBF volume model.

## Subsequent bounded safeguard

The original quadratic-only eligibility proved insufficient for nearly opposed
moving prescribed walls: an arbitrarily large lateral velocity could satisfy the
algebraic rows. Optional acceleration now requires homogeneous prescribed normal
motion. All moving prescribed rows still participate in the unchanged bounded
sequential solve. Within the eligible domain, the actual candidate must be finite,
retain nonnegative normal impulses and the friction cone, and not worsen the
prepared-row closing residual; failure restores exact prior velocities and rows.

The subsequent focused run preserves all 52 previous checks and passes 25 added
checks, including exact/five tilted conflicting-wall witnesses. Those witnesses
remain at the iteration cap rather than being called physically solved. The
100:1 finite-body support witness still converges. Full details, source hashes
and unchanged raw failures are in [acceleration-safety](acceleration-safety/README.md).
