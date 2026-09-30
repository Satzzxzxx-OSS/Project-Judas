#pragma once
#include <memory>
#include <vector>
#include "PhysicsWorld.h"
class RuntimeWorld;
class PlayerController;
struct PlayerFluidSample;
struct FluidBodyCouplingMeasurement {
    BodyHandle body;
    float fraction = 0, displacedVolume = 0, containedMass = 0;
    glm::vec3 buoyancyForce{0}, dragForce{0}, containedImpulse{0};
};
struct ProductionFluidMeasurements {
    bool executed = false;
    unsigned long long executedSteps = 0;
    float lastFluidDeltaTime = 0;
    double particleMilliseconds = 0, totalMilliseconds = 0;
    glm::vec3 exteriorReactionNotApplied{0};
    glm::vec3 containedStaticSupportImpulse{0}; // external impulse on coupled rigid island
    std::vector<FluidBodyCouplingMeasurement> bodies;
};
// Production approximation only. No research pressure solver is integrated.
// Owns transient cadence, geometry-keyed quadrature and particle-field caches.
class ProductionFluidCoupling {
public:
    ProductionFluidCoupling();
    ~ProductionFluidCoupling();
    void Reset();
    void Advance(RuntimeWorld& world, float rigidDeltaTime);
    PlayerFluidSample SamplePlayer(const RuntimeWorld& world, const PlayerController& player) const;
    const ProductionFluidMeasurements& Measurements() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
