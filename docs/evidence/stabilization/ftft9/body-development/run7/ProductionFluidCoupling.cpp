#include "ProductionFluidCoupling.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <glm/gtc/matrix_inverse.hpp>
#include "FluidHydrostatics.h"
#include "PlayerController.h"
#include "RuntimeWorld.h"
namespace {
using Clock = std::chrono::steady_clock;
double Milliseconds(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
bool SamePose(const BodyTransform& a, const BodyTransform& b) { return a.position==b.position && a.rotation==b.rotation; }
bool Inside(const glm::vec3& point, const BodyTransform& pose, const std::vector<SceneFluidCavity>& cavities) {
    const glm::vec3 local=glm::inverse(pose.rotation)*(point-pose.position);
    for(const auto& c:cavities) if(glm::all(glm::lessThanEqual(glm::abs(local-c.localCenter),c.halfExtents))) return true;
    return false;
}
glm::mat3 CrossMatrix(const glm::vec3& r) {
    return glm::mat3(0,r.z,-r.y,-r.z,0,r.x,r.y,-r.x,0);
}
}
struct ProductionFluidCoupling::Impl {
    struct Body {
        BodyHandle handle;
        FluidVolumeQuadrature volume;
        std::vector<SceneFluidCavity> cavities;
        FluidHydrostaticField exterior;
    };
    unsigned entityVersion=std::numeric_limits<unsigned>::max();
    double pending=0;
    bool first=true;
    FluidHydrostaticField field;
    FluidVolumeQuadrature playerVolume=MakeBoxFluidVolume(glm::vec3(PlayerController::CapsuleRadius(),
        PlayerController::CapsuleHalfHeight()+PlayerController::CapsuleRadius(),PlayerController::CapsuleRadius()));
    std::vector<Body> bodies;
    std::unordered_map<unsigned,BodyTransform> previousFluidPoses;
    ProductionFluidMeasurements measured;
    void Rebuild(RuntimeWorld& world) {
        if(entityVersion==world.EntityVersion()) return;
        bodies.clear();
        for(const EntityRecord& e:world.Entities()) {
            if(e.lifecycle!=EntityLifecycle::Active || e.fidelity!=SimulationFidelity::Full || !e.definition.body) continue;
            const auto& definition=*e.definition.body;
            const auto handle=world.DynamicBodies()[e.slot].Handle();
            if(!world.Physics().IsDynamicBody(handle)) continue;
            FluidVolumeQuadrature volume;
            if(definition.shape==SceneShape::Box) volume=MakeBoxFluidVolume(definition.halfExtents);
            else if(definition.shape==SceneShape::Sphere) volume=MakeSphereFluidVolume(definition.radius);
            else if(definition.shape==SceneShape::Compound) volume=MakeCompoundFluidVolume(definition.compoundBoxes);
            else continue;
            bodies.push_back({handle,std::move(volume),definition.fluidCavities,{}});
        }
        entityVersion=world.EntityVersion();
    }
};
ProductionFluidCoupling::ProductionFluidCoupling():m_impl(std::make_unique<Impl>()) {}
ProductionFluidCoupling::~ProductionFluidCoupling()=default;
void ProductionFluidCoupling::Reset() { m_impl=std::make_unique<Impl>(); }
const ProductionFluidMeasurements& ProductionFluidCoupling::Measurements() const { return m_impl->measured; }
void ProductionFluidCoupling::Advance(RuntimeWorld& world,float dt) {
    auto& s=*m_impl;const auto started=Clock::now();auto& p=world.Physics();auto& fluid=world.Fluid();
    s.Rebuild(world);s.measured.executed=false;s.measured.particleMilliseconds=0;s.measured.exteriorReactionNotApplied=glm::vec3(0);s.measured.containedStaticSupportImpulse=glm::vec3(0);
    s.measured.contactBatches=0;s.measured.contactIterations=0;s.measured.normalMatrixProducts=0;
    s.measured.normalAccelerations=0;s.measured.contactIterationCaps=0;s.measured.normalBreakdowns=0;
    s.measured.maximumContactRelativeResidual=0;
    s.measured.bodies.assign(s.bodies.size(),{});
    for(std::size_t i=0;i<s.bodies.size();++i)s.measured.bodies[i].body=s.bodies[i].handle;
    s.pending+=dt;
    const double period=1.0/world.FluidSettingsUsed().updateRateHz;
    if(s.first || s.pending+std::numeric_limits<float>::epsilon()*period>=period) {
        const float fluidDt=static_cast<float>(s.pending);s.pending=0;s.first=false;
        std::vector<FluidBoxCollider> boxes;std::vector<FluidSphereCollider> spheres;std::vector<FluidTerrainCollider> terrains;
        std::unordered_map<unsigned,BodyTransform> nextPoses, startPoses;
        for(const auto handle:p.AliveBodies()) {
            Shape shape;BodyTransform now;if(!p.GetBodyShape(handle,shape,now))continue;
            BodyTransform end=now,begin=now;
            if(p.IsDynamicBody(handle)) {
                // One-step endpoint predictor from authoritative, already
                // gravity-kicked velocity. It is only a fluid boundary, never
                // writes rigid pose. The 30 Hz liquid uses its own pose history.
                end.position+=p.GetLinearVelocity(handle)*dt;
                const glm::vec3 omega=p.GetAngularVelocity(handle);
                end.rotation=glm::normalize(now.rotation+.5f*dt*(glm::quat(0,omega)*now.rotation));
                auto found=s.previousFluidPoses.find(handle.id);
                if(found!=s.previousFluidPoses.end())begin=found->second;
                // ResetBody synchronizes both authoritative pose endpoints.
                // Such teleports are not interpolated as liquid trajectories.
                if(SamePose(p.GetPreviousTransform(handle),now) &&
                   found!=s.previousFluidPoses.end() && !SamePose(found->second,now))begin=now;
            }
            nextPoses.emplace(handle.id,end);startPoses.emplace(handle.id,begin);
            if(shape.type==ShapeType::Sphere)spheres.push_back({handle,begin,end,shape.radius});
            else if(shape.type==ShapeType::Box)boxes.push_back({handle,begin,end,shape.halfExtents});
            else if(shape.type==ShapeType::CompoundBoxes)for(const auto& b:shape.boxes)
                boxes.push_back({handle,{begin.position+begin.rotation*b.localCenter,begin.rotation},
                    {end.position+end.rotation*b.localCenter,end.rotation},b.halfExtents});
            else if(shape.type==ShapeType::Terrain)terrains.push_back({handle,begin,end,shape.terrain.get()});
        }
        std::vector<std::vector<bool>> insideBefore(s.bodies.size());
        for(std::size_t i=0;i<s.bodies.size();++i)if(!s.bodies[i].cavities.empty()) {
            const auto pose=startPoses.at(s.bodies[i].handle.id);auto& mask=insideBefore[i];mask.reserve(fluid.Particles().size());
            for(const auto& particle:fluid.Particles())mask.push_back(Inside(particle.position,pose,s.bodies[i].cavities));
        }
        std::vector<FluidContactImpulse> reactions;
        const auto particleStart=Clock::now();
        const FluidVelocityBatchResponse response=[&](std::vector<FluidParticle>& particles,
            const std::vector<FluidVelocityContact>& contacts,std::vector<glm::vec3>& impulses) {
            std::vector<PhysicsWorld::ContactParticle> points;points.reserve(particles.size());
            for(const auto& particle:particles)points.push_back({particle.position,particle.velocity,particle.mass});
            std::vector<PhysicsWorld::ParticleBoundaryContact> rows;rows.reserve(contacts.size());
            std::vector<std::size_t> owners;owners.reserve(contacts.size());
            for(const auto& c:contacts) {
                std::size_t owner=s.bodies.size();
                for(std::size_t i=0;i<s.bodies.size();++i) {
                    const auto& body=s.bodies[i];if(body.handle.id!=c.owner.id||body.cavities.empty())continue;
                    const auto& begin=startPoses.at(body.handle.id);const auto& end=nextPoses.at(body.handle.id);
                    const BodyTransform pose{glm::mix(begin.position,end.position,c.stepFraction),
                        glm::normalize(glm::slerp(begin.rotation,end.rotation,c.stepFraction))};
                    if(insideBefore[i][c.particleIndex] || Inside(c.particlePosition,pose,body.cavities))owner=i;
                    break;
                }
                owners.push_back(owner);
                rows.push_back({c.particleIndex,c.owner,c.point,c.normal,c.wallVelocity,owner<s.bodies.size()});
            }
            glm::vec3 supportImpulse(0);
            p.SolveParticleContacts(points,rows,impulses,&supportImpulse,dt);
            const auto contactStats=p.GetParticleContactStats();
            ++s.measured.contactBatches;s.measured.contactIterations+=contactStats.iterations;
            s.measured.normalMatrixProducts+=contactStats.normalMatrixProducts;
            s.measured.normalAccelerations+=contactStats.normalAccelerations;
            s.measured.contactIterationCaps+=contactStats.iterationCapReached?1:0;
            s.measured.normalBreakdowns+=contactStats.normalAccelerationBreakdowns;
            s.measured.maximumContactRelativeResidual=std::max(s.measured.maximumContactRelativeResidual,
                contactStats.maximumClosingSpeed/contactStats.speedScale);
            s.measured.containedStaticSupportImpulse+=supportImpulse;
            for(std::size_t i=0;i<particles.size();++i)particles[i].velocity=points[i].velocity;
            for(std::size_t row=0;row<rows.size();++row)if(owners[row]<s.bodies.size())
                s.measured.bodies[owners[row]].containedImpulse-=impulses[row];
        };
        fluid.Step(fluidDt,world.Gravity(),boxes,spheres,terrains,&reactions,response);

        s.measured.particleMilliseconds=Milliseconds(particleStart);
        s.measured.executed=true;++s.measured.executedSteps;s.measured.lastFluidDeltaTime=fluidDt;
        // Reactions were applied exactly once inside the velocity contact.
        // The remaining dynamic-boundary exchange is deliberately one-way.
        for(const auto& reaction:reactions)if(p.IsDynamicBody(reaction.owner))
            s.measured.exteriorReactionNotApplied+=reaction.impulse;
        for(const auto& body:s.measured.bodies)s.measured.exteriorReactionNotApplied-=body.containedImpulse;
        s.previousFluidPoses.swap(nextPoses);
        s.field.Build(fluid.Particles(),world.FluidSettingsUsed().restDensity);
    }
    for(std::size_t i=0;i<s.bodies.size();++i) {
        auto& body=s.bodies[i];const auto pose=p.GetTransform(body.handle);const FluidHydrostaticField* field=&s.field;
        if(!body.cavities.empty()) {
            std::vector<bool> excluded;excluded.reserve(fluid.Particles().size());
            for(const auto& particle:fluid.Particles()) {
                // The particle field is stamped at the most recent liquid endpoint.
                // On executed frames rigid integration still follows this call;
                // testing against its pre-step pose would reclassify contained
                // liquid as exterior and apply a second hydrostatic force.
                const auto found=s.previousFluidPoses.find(body.handle.id);
                const BodyTransform fluidPose=found==s.previousFluidPoses.end()?pose:found->second;
                const bool contained=Inside(particle.position,fluidPose,body.cavities);excluded.push_back(contained);
                if(contained)s.measured.bodies[i].containedMass+=particle.mass;
            }
            body.exterior.Build(fluid.Particles(),world.FluidSettingsUsed().restDensity,&excluded);field=&body.exterior;
        }
        const auto hydro=EvaluateFluidHydrostatics(*field,body.volume,pose,world.Gravity());
        auto& diagnostic=s.measured.bodies[i];diagnostic.fraction=hydro.fraction;diagnostic.displacedVolume=hydro.submergedVolume;
        diagnostic.buoyancyForce=hydro.buoyancyForce;
        p.ApplyForce(body.handle,hydro.buoyancyForce);p.ApplyTorque(body.handle,hydro.buoyancyTorque);
        if(hydro.fraction>0) {
            const float mass=p.GetMass(body.handle);const glm::mat3 inverseInertia=glm::inverse(p.GetInertiaWorld(body.handle));
            const glm::vec3 offset=hydro.submergedCentroid-pose.position;
            const glm::vec3 liquidVelocity=hydro.fluidVelocity+hydro.fluidAcceleration*static_cast<float>(s.pending);
            // Drag follows the known gravity/hydrostatic kick. In particular,
            // neutral hydrostatics must not damp gravity and then cancel it a
            // second time. The point effective mass includes rotational inertia.
            const glm::vec3 relative=p.GetLinearVelocity(body.handle)+hydro.buoyancyForce*(dt/mass)+
                glm::cross(p.GetAngularVelocity(body.handle)+inverseInertia*hydro.buoyancyTorque*dt,offset)-liquidVelocity;
            const glm::mat3 cross=CrossMatrix(offset);
            const glm::mat3 inverseEffectiveMass=glm::mat3(1.0f/mass)-cross*inverseInertia*cross;
            const float fraction=-std::expm1(-world.FluidSettingsUsed().hydrostaticDragRate*hydro.fraction*dt);
            const glm::vec3 impulse=-fraction*(glm::inverse(inverseEffectiveMass)*relative);
            diagnostic.dragForce=impulse/dt;
            p.ApplyForce(body.handle,diagnostic.dragForce);p.ApplyTorque(body.handle,glm::cross(offset,diagnostic.dragForce));
        }
    }
    s.measured.totalMilliseconds=Milliseconds(started);
}
PlayerFluidSample ProductionFluidCoupling::SamplePlayer(const RuntimeWorld& world,const PlayerController& player) const {
    const auto& s=*m_impl;const auto sample=EvaluateFluidHydrostatics(s.field,s.playerVolume,
        {player.GetPosition(),player.GetOrientation()},world.Gravity());
    return {sample.fraction,sample.density,
        sample.fluidVelocity+sample.fluidAcceleration*static_cast<float>(s.pending),sample.fluidAcceleration};
}
