#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "CollisionShapes.h"
#include "FluidWorld.h"

class GravityField;

// Deterministic shape-local volume quadrature, constructed once per shape.
// Boxes use equal Cartesian subvolumes; spheres use antipodal equal-volume
// radial/angular samples. Compound boxes are partitioned into a nonoverlapping
// union before sampling, so overlapping child boxes do not double their volume.
struct FluidVolumeSample {
    glm::vec3 localPosition{0};
    glm::vec3 localHalfExtents{0};
    float localRadius = 0; // isotropic sample footprint (sphere quadrature)
    float volume = 0;
};
struct FluidVolumeQuadrature {
    std::vector<FluidVolumeSample> samples;
    float totalVolume = 0;
    float boundRadius = 0;
};
FluidVolumeQuadrature MakeBoxFluidVolume(const glm::vec3& halfExtents, unsigned samplesPerAxis = 4);
FluidVolumeQuadrature MakeSphereFluidVolume(float radius, unsigned radialLayers = 3, unsigned antipodalPairs = 24);
FluidVolumeQuadrature MakeCompoundFluidVolume(const std::vector<CompoundBox>& boxes, unsigned samplesPerAxis = 4);

struct FluidFieldSample {
    float fraction = 0;
    float density = 0; // rest density of liquid, not multiplied by fraction
    glm::vec3 velocity{0};
    glm::vec3 acceleration{0};
};

// Approximate local liquid columns reconstructed from particle volume. Each
// particle contributes a vertical interval of length cbrt(m/rho). Intersecting
// intervals from nearby lateral positions form an occupied column, with gaps
// retained between disconnected intervals. This extrapolates through a body's
// particle-exclusion hole; it is not an SPH pressure/reaction model. The reach
// is geometric (the queried body's bound plus particle spacing), independent
// of mass/density/name. Shoreline/overhang detail below that reach is approximate.
// Direction is supplied by the caller; zero-gravity callers use a body-local
// reference axis, never a privileged world-up gravity direction.
class FluidHydrostaticField {
public:
    void Build(const std::vector<FluidParticle>& particles, float restDensity,
               const std::vector<bool>* excludedParticles = nullptr);
    FluidFieldSample Query(const glm::vec3& point, const glm::vec3& up,
                           float sampleHalfHeight, float extrapolationRadius) const;
    float RestDensity() const { return m_density; }
    float MaximumParticleSpacing() const { return m_maxSpacing; }
    std::size_t ParticleCount() const { return m_particles.size(); }
private:
    struct Particle { glm::vec3 position, velocity, acceleration; float spacing, volume; };
    struct Key {
        std::int64_t x, y, z;
        bool operator==(const Key& b) const { return x == b.x && y == b.y && z == b.z; }
    };
    struct Hash { std::size_t operator()(const Key& k) const; };
    Key Cell(const glm::vec3& point) const;
    float m_density = 0, m_maxSpacing = 0, m_cellSize = 1;
    glm::vec3 m_min{0}, m_max{0};
    std::vector<Particle> m_particles;
    std::unordered_map<Key, std::vector<std::size_t>, Hash> m_cells;
};

struct FluidHydrostaticResult {
    float totalVolume = 0, submergedVolume = 0, fraction = 0, density = 0;
    glm::vec3 submergedCentroid{0};
    glm::vec3 fluidVelocity{0}, fluidAcceleration{0};
    glm::vec3 buoyancyForce{0}, buoyancyTorque{0}; // torque about pose.position
};
// No body mass enters this calculation. Apply the returned force at COM and
// returned torque once, or (for uniform effective gravity) force at centroid.
// Exterior particle collision must not add a second hydrostatic reaction.
FluidHydrostaticResult EvaluateFluidHydrostatics(const FluidHydrostaticField& field,
                                                const FluidVolumeQuadrature& volume,
                                                const BodyTransform& pose,
                                                const GravityField& gravity);
