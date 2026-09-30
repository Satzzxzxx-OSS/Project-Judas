// FTFT9 direct production-path contained-particle impulse budget witness.
#include <cmath>
#include <iomanip>
#include <iostream>
#include "Scene.h"
#include "RuntimeWorld.h"
#include "GameSession.h"
#include "Simulation.h"
#include "ProductionFluidCoupling.h"
#include "Window.h"
int main() {
 Scene scene;scene.Settings().name="contained transfer budget";scene.Settings().fluidHydrostaticDragRate=0;
 auto& observer=scene.CreateObject("observer");observer.playerStart=ScenePlayerStartComponent{};observer.transform.position={10,10,10};
 auto& gravity=scene.CreateObject("zero acceleration");gravity.gravity=SceneGravityComponent{};gravity.gravity->kind=SceneGravityKind::Uniform;gravity.gravity->magnitude=0;gravity.gravity->regionShape=SceneRegionShape::Box;gravity.gravity->regionHalfExtents=glm::vec3(100);
 auto& wall=scene.CreateObject("boundary");const auto id=wall.id;wall.body=SceneBodyComponent{};wall.body->motion=SceneBodyMotion::Dynamic;wall.body->mass=1;wall.body->halfExtents={.05f,1,1};wall.body->friction=0;wall.body->restitution=0;wall.body->fluidCavities.push_back({{-.55f,0,0},{.5f,.5f,.5f}});wall.transform.position={.55f,0,0};
 auto& volume=scene.CreateObject("liquid");volume.fluidVolume=SceneFluidVolumeComponent{};volume.fluidVolume->countX=1;volume.fluidVolume->countY=1;volume.fluidVolume->countZ=1;volume.fluidVolume->spacing=std::cbrt(.1f);volume.transform.position={.47f,0,0};
 RuntimeWorld world;GameSession session;Window window;window.SetTestInputMode(true);std::string error;
 if(!world.Build(scene,nullptr,error)||!session.Begin(world,error)){std::cerr<<error<<'\n';return 2;}
 // Public runtime API sets the prescribed initial fluid velocity. There is no
 // alternative integration/contact path: the ordinary StepPlayedWorld runs.
 world.Fluid().Clear();world.Fluid().AddParticle({.47f,0,0},{1,0,0},100);
 const auto handle=world.DynamicBodies()[world.FindEntity(id)->slot].Handle();
 auto budget=[&](){const auto fd=world.Fluid().GetDiagnostics();const auto v=world.Physics().GetLinearVelocity(handle),w=world.Physics().GetAngularVelocity(handle);const double ke=fd.kineticEnergy+.5*world.Physics().GetMass(handle)*glm::dot(v,v)+.5*glm::dot(w,world.Physics().GetInertiaWorld(handle)*w);return std::pair<double,glm::vec3>{ke,fd.totalMomentum+world.Physics().GetMass(handle)*v};};
 const auto before=budget();StepPlayedWorld(session,window,1.f/60);const auto after=budget();
 const auto v=world.Physics().GetLinearVelocity(handle),w=world.Physics().GetAngularVelocity(handle);const auto& particle=world.Fluid().Particles().front();const auto& measurement=world.FluidCoupling().Measurements();
 std::cout<<std::setprecision(17)<<"particle_mass=100 body_mass=1 dt="<<1.f/60<<" initial_energy="<<before.first<<" final_energy="<<after.first<<" energy_change="<<after.first-before.first<<" initial_momentum="<<before.second.x<<","<<before.second.y<<","<<before.second.z<<" final_momentum="<<after.second.x<<","<<after.second.y<<","<<after.second.z<<" momentum_residual="<<glm::length(after.second-before.second)<<" body_v="<<v.x<<","<<v.y<<","<<v.z<<" body_w="<<w.x<<","<<w.y<<","<<w.z<<" particle_v="<<particle.velocity.x<<","<<particle.velocity.y<<","<<particle.velocity.z<<" contained_impulse="<<measurement.bodies.front().containedImpulse.x<<","<<measurement.bodies.front().containedImpulse.y<<","<<measurement.bodies.front().containedImpulse.z<<'\n';
 const bool energy=after.first<=before.first+1e-3;const bool momentum=glm::length(after.second-before.second)<=1e-3;
 std::cout<<"CHECK no_unexplained_energy_creation="<<energy<<" momentum_closed="<<momentum<<'\n';return energy&&momentum?0:1;
}
