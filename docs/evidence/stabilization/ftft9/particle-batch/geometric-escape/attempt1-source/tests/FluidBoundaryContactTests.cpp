// FTFT9 production FluidWorld solid-contact ownership and exact-touch witnesses.
// Geometric witnesses isolate solid response; a separate two-particle witness
// checks that configured smoothing cannot invalidate the final wall condition.
#include <algorithm>
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
void SqueezedBox(bool rotated,bool reversed) {
 const glm::quat q=rotated?glm::angleAxis(.713f,glm::normalize(glm::vec3(1,2,-3))):glm::quat(1,0,0,0);
 const glm::vec3 translation=rotated?glm::vec3(.5f,-.2f,1):glm::vec3(0);
 const auto transform=[&](glm::vec3 point){return translation+q*point;};
 PhysicsWorld world;world.Init();
 const auto floorOwner=world.CreateStaticBox(transform({0,-.125f,0}),q,{2,.125f,2},0,0);
 const auto bodyOwner=world.CreateStaticBox(transform({0,.2f,0}),q,{.25f,.25f,.35f},0,0);
 FluidBoxCollider floor{floorOwner,{transform({0,-.125f,0}),q},{transform({0,-.125f,0}),q},{2,.125f,2}};
 FluidBoxCollider body{bodyOwner,{transform({0,.575f,0}),q},{transform({0,.2f,0}),q},{.25f,.25f,.35f}};
 FluidSettings settings;settings.particleRadius=.125f;settings.substeps=1;settings.densityIterations=0;settings.velocitySmoothing=0;
 FluidWorld fluid(settings);fluid.AddParticle(transform({.05f,.125f,.03f}),glm::vec3(0),1);
 UniformGravity gravity(glm::vec3(0));
 bool oldBottom=false,newSide=false,floorRetained=false;
 const FluidVelocityBatchResponse response=[&](std::vector<FluidParticle>& particles,const std::vector<FluidVelocityContact>& contacts,std::vector<glm::vec3>& impulses){
  std::vector<PhysicsWorld::ContactParticle> points;for(const auto& p:particles)points.push_back({p.position,p.velocity,p.mass});
  std::vector<PhysicsWorld::ParticleBoundaryContact> rows;
  for(const auto& c:contacts){const auto normal=glm::inverse(q)*c.normal;
   if(c.owner.id==bodyOwner.id){oldBottom=oldBottom||normal.y<-.9f;newSide=newSide||normal.x>.9f;}
   if(c.owner.id==floorOwner.id)floorRetained=floorRetained||normal.y>.9f;
   rows.push_back({c.particleIndex,c.owner,c.point,c.normal,c.wallVelocity,false});
  }
  world.SolveParticleContacts(points,rows,impulses);
  for(std::size_t i=0;i<particles.size();++i)particles[i].velocity=points[i].velocity;
 };
 const std::vector<FluidBoxCollider> boxes=reversed?std::vector<FluidBoxCollider>{body,floor}:std::vector<FluidBoxCollider>{floor,body};
 fluid.Step(.01f,gravity,boxes,{},{},nullptr,response);
 const auto& particle=fluid.Particles().front();const auto point=glm::inverse(q)*(particle.position-translation);
 const auto stats=fluid.GetDiagnostics();
 for(const auto& box:boxes){const auto local=glm::inverse(box.currentPose.rotation)*(particle.position-box.currentPose.position);
  const auto margin=glm::abs(local)-(box.halfExtents+glm::vec3(settings.particleRadius));
  Check(std::max({margin.x,margin.y,margin.z})>=-2e-6f,"squeezed particle is excluded from every expanded solid");
 }
 Check(std::abs(point.x-.375f)<3e-5f&&point.y>=.125f-2e-6f,"unique nearest feasible box escape is lateral, above floor");
 Check(!oldBottom&&newSide&&floorRetained,"escape removes inactive bottom patch and preserves final side/floor normals");
 Check(stats.geometryEscapeCount>0&&stats.unresolvedGeometryCount==0&&stats.geometryEscapeCandidates>0,"geometric escape and all candidate work are reported");
 Check(stats.maximumGeometryEscapeDistance>0,"endpoint escape displacement is observable");
 Check(fluid.Particles().size()==1&&stats.totalMass==1,"endpoint escape preserves particle identity/count/mass");
 Check(glm::length(particle.velocity)<2e-5f&&stats.kineticEnergy<2e-9f,"geometric endpoint escape does not become kinetic impulse");
 Check(!world.GetParticleContactStats().iterationCapReached,"final active escaped box normals are feasible in actual auxiliary solve");
 std::cout<<"CASE squeezed-box rotated="<<rotated<<" reversed="<<reversed<<" local_final="<<point.x<<','<<point.y<<','<<point.z<<" escapes="<<stats.geometryEscapeCount<<" unresolved="<<stats.unresolvedGeometryCount<<" candidates="<<stats.geometryEscapeCandidates<<" maximum_escape_distance="<<stats.maximumGeometryEscapeDistance<<" old_bottom="<<oldBottom<<" new_side="<<newSide<<" floor="<<floorRetained<<" energy="<<stats.kineticEnergy<<'\n';
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
 for(bool rotated:{false,true})for(bool reversed:{false,true})SqueezedBox(rotated,reversed);
 std::cout<<"SUMMARY cases=12 checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;
}
