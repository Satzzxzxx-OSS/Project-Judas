// Read-only diagnostic isolation of two exact authored classic-scene pairs.
// This does not replace the application regression or alter a physics law.
#include <cmath>
#include <iomanip>
#include <iostream>
#include "PhysicsWorld.h"
#include "Narrowphase.h"
#include "RadicalGravity.h"
int main(){
 std::cout<<std::setprecision(17);
 for(bool planet:{true,false}){
  PhysicsWorld w;w.Init();
  auto support=planet?w.CreateStaticSphere(glm::vec3(0),20,.8f,.1f):w.CreateStaticBox(glm::vec3(0,17,27.5f),glm::vec3(6,1,20),.8f,.1f);
  auto h=w.CreateDynamicBox(planet?glm::vec3(14.666667f,14.666667f,-7.3333335f):glm::vec3(-3,21,18),glm::vec3(.5f),5,.6f,.15f);
  RadicalGravity radial(glm::vec3(0),9.81f);
  for(int step=0;step<70;++step){
   const auto before=w.GetTransform(h);
   const glm::vec3 g=planet?radial.Sample(before.position):glm::vec3(0,-9.81f,0);
   constexpr float dt=1.f/60;
   w.ApplyLinearAcceleration(h,g,dt);
   Shape sa,sb;BodyTransform pa,pb;w.GetBodyShape(support,sa,pa);w.GetBodyShape(h,sb,pb);
   RigidBody a,b;a.position=pa.position;a.orientation=pa.rotation;b.position=pb.position;b.orientation=pb.rotation;
   const float margin=(glm::length(w.GetLinearVelocity(h))+glm::length(w.GetAngularVelocity(h))*ShapeBoundingRadius(sb))*dt;
   const auto m=ComputeContacts(sa,a,sb,b,margin);
   w.Step(dt);const auto after=w.GetTransform(h);const auto v=w.GetLinearVelocity(h);const auto omega=w.GetAngularVelocity(h);
   std::cout<<"{\"case\":\""<<(planet?"planet_cube":"plank_cube")<<"\",\"step\":"<<step<<",\"before\":["<<before.position.x<<','<<before.position.y<<','<<before.position.z<<"],\"q\":["<<before.rotation.w<<','<<before.rotation.x<<','<<before.rotation.y<<','<<before.rotation.z<<"],\"margin\":"<<margin<<",\"contacts\":[";
   for(int i=0;i<m.count;++i){if(i)std::cout<<',';const auto&c=m.points[i];std::cout<<"{\"point\":["<<c.point.x<<','<<c.point.y<<','<<c.point.z<<"],\"normal\":["<<c.normal.x<<','<<c.normal.y<<','<<c.normal.z<<"],\"gap\":";
#ifdef FTFT4_BASELINE
    std::cout<<-double(c.penetration);
#else
    std::cout<<c.signedSeparation;
#endif
    std::cout<<'}';}
   std::cout<<"],\"after\":["<<after.position.x<<','<<after.position.y<<','<<after.position.z<<"],\"velocity\":["<<v.x<<','<<v.y<<','<<v.z<<"],\"angular\":["<<omega.x<<','<<omega.y<<','<<omega.z<<"],\"solver_gaps\":[";
   for(std::size_t i=0;i<w.LastStepContacts().size();++i){if(i)std::cout<<',';std::cout<<-double(w.LastStepContacts()[i].penetration);}
   std::cout<<"]}\n";
  }
 }
}
