// FTFT9 production FluidWorld solid-contact ownership and exact-touch witnesses.
// Geometric witnesses isolate solid response; a separate two-particle witness
// checks that configured smoothing cannot invalidate the final wall condition.
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
void SmoothingWall(float smoothing) {
 FluidSettings settings;settings.particleRadius=.018f;settings.substeps=1;
 settings.densityIterations=0;settings.velocitySmoothing=smoothing;
 const float dt=1.f/60.f;const glm::vec3 acceleration(0,-9.81f,0);
 FluidWorld fluid(settings);fluid.AddParticle({0,settings.particleRadius,0},{0,0,0},1);
 fluid.AddParticle({0,.06f,0},{0,0,0},1);
 UniformGravity gravity(acceleration);
 const auto floor=Box({0,-.125f,0},{2,.125f,2});
 fluid.Step(dt,gravity,{floor});const auto& lower=fluid.Particles()[0];const auto& upper=fluid.Particles()[1];
 const float normalSpeed=lower.velocity.y;
 std::cout<<std::setprecision(17)<<"CASE final-wall-after-smoothing coefficient="<<smoothing<<" dt="<<dt
  <<" lower_initial_y="<<settings.particleRadius<<" upper_initial_y=0.06 gravity_y="<<acceleration.y
  <<" lower_final_y="<<lower.position.y<<" upper_final_y="<<upper.position.y
  <<" lower_normal_velocity="<<normalSpeed<<" upper_normal_velocity="<<upper.velocity.y
  <<" actual_kinetic_energy="<<fluid.GetDiagnostics().kineticEnergy
  <<" unimpeded_gravity_velocity="<<acceleration.y*dt<<'\n';
 // A non-penetrating velocity condition must still hold at the END of the
 // ordinary fluid step. Smoothing is internal transport, not authorization
 // for the final velocity to point into a stationary impenetrable floor.
 Check(lower.position.y>=settings.particleRadius-2e-6f,"smoothing floor final position remains outside solid");
 Check(normalSpeed>=-2e-6f,"smoothing preserves final solid normal velocity condition");
 const double gravityOnlyEnergy=glm::dot(glm::dvec3(acceleration*dt),glm::dvec3(acceleration*dt));
 Check(fluid.GetDiagnostics().kineticEnergy<=gravityOnlyEnergy+1e-6,"smoothing/contact does not exceed unimpeded gravity kinetic energy");
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
 SmoothingWall(0);SmoothingWall(.15f);
 std::cout<<"SUMMARY cases=8 checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;
}
