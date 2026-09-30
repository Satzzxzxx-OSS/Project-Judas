// FTFT9 production FluidWorld solid-contact ownership and exact-touch witnesses.
// No pressure/PBF correction, smoothing or forces obscure collision response.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include "FluidWorld.h"
#include "UniformGravity.h"
namespace {
int checks=0,failures=0;
void Check(bool yes,const char* label){++checks;if(!yes){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
FluidBoxCollider Box(glm::vec3 center,glm::vec3 half){const BodyTransform pose{center,glm::quat(1,0,0,0)};return {{},pose,pose,half};}
void Run(const char* name,const std::vector<FluidBoxCollider>& boxes,glm::vec3 position,glm::vec3 velocity,float dt,glm::vec3 expectedPosition,glm::vec3 expectedVelocity){
 FluidSettings settings;settings.particleRadius=.125f;settings.substeps=1;settings.densityIterations=0;settings.velocitySmoothing=0;
 FluidWorld fluid(settings);fluid.AddParticle(position,velocity,1);UniformGravity gravity(glm::vec3(0));
 fluid.Step(dt,gravity,boxes);const auto& p=fluid.Particles().front();
 std::cout<<std::setprecision(17)<<"CASE "<<name<<" dt="<<dt<<" initial_position="<<position.x<<','<<position.y<<','<<position.z<<" initial_velocity="<<velocity.x<<','<<velocity.y<<','<<velocity.z<<" final_position="<<p.position.x<<','<<p.position.y<<','<<p.position.z<<" final_velocity="<<p.velocity.x<<','<<p.velocity.y<<','<<p.velocity.z<<" expected_position="<<expectedPosition.x<<','<<expectedPosition.y<<','<<expectedPosition.z<<" expected_velocity="<<expectedVelocity.x<<','<<expectedVelocity.y<<','<<expectedVelocity.z<<" energy_initial="<<.5*glm::dot(velocity,velocity)<<" energy_final="<<.5*glm::dot(p.velocity,p.velocity)<<'\n';
 Check(glm::length(p.position-expectedPosition)<2e-6f,name);
 Check(glm::length(p.velocity-expectedVelocity)<2e-6f,name);
 Check(glm::dot(p.velocity,p.velocity)<=glm::dot(velocity,velocity)+1e-5f,name);
}
}
int main(){
 const auto floor=Box({0,-.125f,0},{2,.125f,2});
 const auto wall=Box({-.125f,0,0},{.125f,2,2});
 Run("two-wall-floor-then-wall",{floor,wall},{.25f,.25f,0},{-1,-1,0},.25f,{.125f,.125f,0},{0,0,0});
 Run("two-wall-wall-then-floor",{wall,floor},{.25f,.25f,0},{-1,-1,0},.25f,{.125f,.125f,0},{0,0,0});
 // Radius/solid margin are exactly representable. Starting on the surface and
 // crossing the entire thin wall must be caught at t=0, not omitted because
 // the slab entry equals the initial interval's zero lower bound.
 Run("touching-fast-inward",{wall},{.125f,0,0},{-100,0,0},.01f,{.125f,0,0},{0,0,0});
 Run("touching-fast-outward",{wall},{.125f,0,0},{100,0,0},.01f,{1.125f,0,0},{100,0,0});
 Run("touching-tangent",{wall},{.125f,0,0},{0,10,0},.01f,{.125f,.1f,0},{0,10,0});
 Run("positive-gap-fast-inward",{wall},{.126f,0,0},{-100,0,0},.01f,{.125f,0,0},{0,0,0});
 std::cout<<"SUMMARY cases=6 checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;
}
