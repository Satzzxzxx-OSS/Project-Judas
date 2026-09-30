# Auxiliary normal-acceleration safety

This is a bounded FTFT9 solver safeguard, not closure of unresolved coarse-particle opposing-wall geometry. Existing 52 checks plus 25 added checks pass (77 total). The ordinary rigid ContactSolver source and numerical contact law are unchanged.

## Before

`before.log` records one finite-mass particle between two prescribed walls: lower normal +Y and velocity zero, upper normal proportional to (tilt,-1,0) and velocity -Y. Exact opposing normals are inconsistent and the original bounded sequential solver reports its cap. At tilt 1e-6, optional normal-PCG instead finds an algebraically feasible lateral velocity 1,000,000 m/s, KE 4e12 J, and reports zero closing residual. The original quadratic decrease and an actual-residual-only guard would accept it. Prescribed boundaries can supply arbitrarily large work, so this is not a closed-system conservation counterexample. It demonstrates why acceleration must not be used blindly in this coarse geometric configuration.

## Changes

Optional acceleration is eligible only for active rows whose infinite-mass endpoints have exactly zero normal point velocity. Finite two-way bodies remain unknowns and eligible; stationary support with the already-cached signed-gap target remains eligible. Moving prescribed rows are still solved by the same bounded 64-iteration PGS; no row is excluded or collision response replaced. `normalAccelerationBoundaryDeclines` records each declined proposal. This limits the optional accelerator's mathematical domain; it is not a body-mass threshold.

Within that domain a proposed impulse must retain the frozen tangent's Coulomb normal load. After replaying the tentative impulses, all actual velocities and impulses must be finite, normal impulses nonnegative, tangents inside the existing Coulomb cone, and actual maximum closing residual must improve or remain within a 32-float-epsilon replay budget. Otherwise auxiliary constraint records and body velocities are restored exactly to the pre-proposal state, and bounded PGS continues. `normalAccelerationRejections` exposes such rejections.

## Results and unresolved limit

`after.log` records 77 checks with no failures. The original stiff 100:1 cached-support witness still accelerates and converges (residual 1.55e-6, static load and momentum checks preserved). Exact and five nearly opposing prescribed-wall fixtures decline optional acceleration, remain finite, match an independent recurrence for the original 64 sequential projections, and honestly report residual about 1 m/s and the iteration cap. The largest test KE is 5.57694 J at tilt .01. Their normal constraints are NOT declared physically satisfied. General treatment of mutually conflicting coarse particle contacts is still a model/geometry limitation.

The guard changes auxiliary iteration sequencing only. It does not change friction, restitution, geometry, the ordinary rigid iteration budget, authoritative poses or the main contact cache. No engine-wide or integrated-body acceptance is claimed by this focused run.
