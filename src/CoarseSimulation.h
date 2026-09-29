#pragma once

class RuntimeWorld;

// Milestone 29: the reduced-fidelity simulation path.
//
// Advances every Active, Coarse entity by one fixed step from its retained
// EntityPhysicalState, with no PhysicsWorld body:
//
//   Settled  — nothing moves. An entity demoted while at rest (below the
//              settle thresholds) is assumed to remain supported by the
//              static geometry it rested on; it costs nothing per step.
//   Inertial — free flight. The same symplectic Euler update the rigid-body
//              integrator uses (velocity, then position; quaternion
//              integrated from angular velocity), driven by the same
//              acceleration sources a live body receives: the scene's local
//              gravity contexts and pairwise Newtonian gravity with the
//              other celestial participants. Static point-mass sources belong
//              only to Celestial-mode vehicles, which require Full fidelity.
//              No contacts, no friction, no drag. Mutual celestial forces
//              wake a Settled coarse celestial into inertial motion.
//
// This is real reduced physics, not an animation: the state it evolves is
// exactly the state reconstruction needs, so promoting back to Full hands
// the live body the coarse pose and velocities with no discontinuity
// beyond what the missing contact response would have produced. That
// missing response is the documented limit: an Inertial coarse entity
// passes through solid geometry. A policy that lets an entity go coarse
// while heading into terrain is choosing to accept that; the M29 demo
// keeps its inertial coarse entities in free space.
//
// Systems that cannot be reduced (fluid, atmosphere, combustion, the
// player, pilotable vehicles, compound bodies) keep their entities at Full
// by capability (RuntimeWorld::EntityRequiresFull); this file never sees
// them.
// Call before PhysicsWorld::Step, once per fixed step. Each pair involving a
// Coarse celestial is evaluated from start-of-step poses exactly once; Full
// bodies receive accumulated force and Coarse records its reciprocal velocity
// kick. Full/Full pair forces remain owned by CelestialGravity::ApplyForces.
void ApplyCoarseCelestialForces(RuntimeWorld& world, float fixedDeltaTime);

// Local acceleration and pose drift, after the force preparation above.
void StepCoarseEntities(RuntimeWorld& world, float fixedDeltaTime);
