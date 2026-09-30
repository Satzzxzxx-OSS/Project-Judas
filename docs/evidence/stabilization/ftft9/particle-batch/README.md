# Contained-particle / rigid-support batch solve

The adapter in `PhysicsWorld::SolveParticleContacts` uses the existing accumulated-impulse `ContactSolver` with restitution zero. Particle point masses and copies of their actual touching rigid support island participate in the same solve. Static boundaries stop island expansion. All geometry comes from the production narrowphase. Touching contacts participate directly. Following explicit operator authorization, an optional upcoming rigid drift interval allows only already-cached support contacts to retain their genuine signed gap using the ordinary solver's existing target construction. New separated pairs remain excluded. No gap is collapsed or pose snapped. This is velocity contact exchange, not fluid pressure projection.

The adapter commits only particle and dynamic-body velocities. It preserves rigid poses, force accumulators, motion ledgers and the ordinary solver warm cache. One-way exterior contacts use the prescribed wall velocity and never feed an impulse to the actual body. Contained contacts return the exact same accumulated row impulse used by the solve. Optional static support impulse is the external impulse **on** the finite island.

The auxiliary solve first executes the ordinary ten iterations. Additional ten-iteration blocks, with a final four-iteration block, are allowed up to 64 iterations when the maximum closing violation of the existing row target exceeds `1e-5 * max(1, initial point-speed magnitude)`. Iterations, residual, scale and cap are observable. No final velocity clamp is applied. This convergence refinement does not change ordinary rigid contact iteration counts.

## Development evidence

- `development1`: test wiring defect: omitted `PhysicsWorld::Init`; exit -11 before contact solving. This is not a physical failure. Source and raw output are preserved.
- `development2`: corrected initialization; 33 checks, one failure. The ten-iteration auxiliary solve left a dynamic two-body support chain particle speed 0.00263166 m/s, exceeding the predeclared 0.002 m/s gate. Direct static support and all mass/torque/momentum checks passed.
- `development3`: authorized bounded auxiliary refinement; 35 checks, zero failures. The same support-chain assertion passed at 20 iterations with closing residual 9.95865e-6 m/s and particle speed 9.83477e-6 m/s. Direct support needed ten iterations. Neither reached the cap.

- `development4`: explicit cached/new signed-gap checks. Uncached separated pair correctly contributes no support; cached pair contributes four matched rows. The hostile 100 kg particle / 1 kg supported body reaches the 64-iteration cap and fails the 0.0001 m/s converged-speed check (body -1.10535 m/s versus permitted gap-closing speed -0.059999 m/s). This finite-iteration stress failure is preserved and is not physical acceptance. The prior 35 checks still pass.

These focused results do not establish settled container hydrostatics. Integrated container evidence is tracked separately; changing support geometry between the fluid and ordinary rigid steps remains explicitly under investigation.

## Current disposition

The explicit cached-support repair is now wired through the upcoming rigid interval. The stiff-case finite-iteration failure was repaired using an algebraic preconditioner of the same normal operator; see [METHOD.md](METHOD.md). The resumed unchanged failure remains preserved. Current evidence is `../containers/development8`: 52 focused checks and 8 integrated cases / 7,014 checks pass. This supersedes the earlier unresolved disposition without rewriting historical output.
