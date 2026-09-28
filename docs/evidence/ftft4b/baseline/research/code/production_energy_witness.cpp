// NOT executed in this research environment: requires Judas/GLM.
// Read-only behavioral diagnostic when linked against the actual engine.
// Exits nonzero if the physical no-energy-creation witness fails.
#include "PhysicsWorld.h"
#include <cmath>
#include <iomanip>
#include <iostream>
int main(){
 PhysicsWorld world; if(!world.Init()) return 2;
 const float m[3]={10.0f,0.125f,10.0f};
 const float vx[3]={1.0f,-3.0f,4.0f};
 BodyHandle b[3];
 for(int i=0;i<3;++i){
  b[i]=world.CreateDynamicSphere(glm::vec3(float(i),0,0),0.5f,m[i],0.0f,1.0f);
  world.SetLinearVelocity(b[i],glm::vec3(vx[i],0,0));
 }
 const auto energy=[&](){ double e=0;for(int i=0;i<3;++i){auto v=world.GetLinearVelocity(b[i]);auto w=world.GetAngularVelocity(b[i]);
  e+=0.5*double(m[i])*double(glm::dot(v,v));
  // Solid-sphere I=2/5*m*r^2, r=.5.
  e+=0.5*(0.4*double(m[i])*0.25)*double(glm::dot(w,w));}return e;};
 const auto momentum=[&](){glm::dvec3 p(0);for(int i=0;i<3;++i)p+=double(m[i])*glm::dvec3(world.GetLinearVelocity(b[i]));return p;};
 const double e0=energy();const auto p0=momentum();world.Step(1.0f/60.0f);const double e1=energy();const auto p1=momentum();
 std::cout<<std::setprecision(17)<<"energy_before="<<e0<<" energy_after="<<e1<<" delta="<<e1-e0
          <<" momentum_residual="<<glm::length(p1-p0)<<" contacts="<<world.LastStepContactCount()<<"\n";
 for(int i=0;i<3;++i)std::cout<<"v["<<i<<"]="<<world.GetLinearVelocity(b[i]).x<<"\n";
 const bool failed=e1>e0+1e-4 || glm::length(p1-p0)>1e-4;
 std::cout<<(failed?"FAIL physical impulse budget\n":"PASS measured impulse budget\n");return failed?1:0;
}
