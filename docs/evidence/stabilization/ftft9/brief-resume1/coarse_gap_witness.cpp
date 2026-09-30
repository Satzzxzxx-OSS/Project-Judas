// Diagnostic of current production FluidWorld solid projection; no new law.
#include <iostream>
#include <iomanip>
#include "FluidWorld.h"
#include "GravityField.h"
struct Zero : GravityField { glm::vec3 Sample(const glm::vec3&) const override { return glm::vec3(0); } };
int main() {
    FluidSettings settings;
    settings.particleRadius=.072f;
    settings.smoothingRadius=.42f;
    settings.maxDensityCorrection=.10f;
    FluidWorld fluid(settings);
    const glm::vec3 initial(0,.075f,0);
    fluid.AddParticle(initial,glm::vec3(0),1.f);
    const BodyTransform floor{{0,-.1f,0},{1,0,0,0}};
    const BodyTransform bodyBefore{{0,.48f,0},{1,0,0,0}};
    const BodyTransform bodyAfter{{0,.30f,0},{1,0,0,0}};
    const std::vector<FluidBoxCollider> boxes={{{1},floor,floor,{1.1f,.1f,1.1f}},{{2},bodyBefore,bodyAfter,{.3f,.3f,.3f}}};
    fluid.Step(1.f/60,Zero{},boxes);
    const auto& particle=fluid.Particles().front();
    const bool floorInterior=particle.position.y<0 && particle.position.y>-.2f && std::abs(particle.position.x)<1.1f && std::abs(particle.position.z)<1.1f;
    const bool bodyInterior=glm::all(glm::lessThan(glm::abs(particle.position-bodyAfter.position),glm::vec3(.3f)));
    std::cout<<std::setprecision(17)<<"initial="<<initial.x<<','<<initial.y<<','<<initial.z<<" radius="<<settings.particleRadius
      <<" final="<<particle.position.x<<','<<particle.position.y<<','<<particle.position.z
      <<" velocity="<<particle.velocity.x<<','<<particle.velocity.y<<','<<particle.velocity.z
      <<" floor_interior="<<floorInterior<<" body_interior="<<bodyInterior<<" particles="<<fluid.Particles().size()<<'\n';
    return floorInterior||bodyInterior ? 1 : 0;
}
