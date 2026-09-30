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
    std::vector<CompoundBox> solidBoxes; // exact union pieces used for geometric column occlusion
    float solidSphereRadius = 0;
};
struct FluidSolidQuery {
    const FluidVolumeQuadrature& volume;
    BodyTransform pose;
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

// Approximate liquid columns at max particle spacing, in an explicit tangent/
// up basis. Vertical particle intervals form a locally filled envelope WITHIN
// each column (internal air pockets below this resolution are unresolved); column
// occupancy fractions are averaged by positive area/spatial-kernel weights.
// Nearby lower liquid columns vote dry at an elevated sample. A supplied solid
// excludes geometrically occluded column footprints so excluded interior
// particles do not masquerade as a lower free surface. There is no SPH reaction.
// The lattice is anchored at the first retained particle, so its reference
// follows universe rotations/translations; changing particle order can change
// the finite-resolution approximation. Shorelines below the reach are smeared.
// Bulk velocity/acceleration use a separate continuous volume-weighted spatial
// kernel Vp*(1-distanceSquared/reachSquared)^2 within the same unoccluded
// exterior columns; displaced-volume/contact shadows do not define bulk motion.
// There is no velocity classifier or temporal filter.
// Direction/tangent are supplied by the caller; zero-gravity callers use a
// body-local reference, never a privileged world-up gravity direction.
class FluidHydrostaticField {
public:
    void Build(const std::vector<FluidParticle>& particles, float restDensity,
               const std::vector<bool>* excludedParticles = nullptr);
    FluidFieldSample Query(const glm::vec3& point, const glm::vec3& up,
                           float sampleHalfHeight, float extrapolationRadius,
                           const glm::vec3& tangentAxis,
                           const FluidSolidQuery* solid = nullptr) const;
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
