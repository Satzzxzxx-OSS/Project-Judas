#include <cmath>
#include <iomanip>
#include <iostream>
#include "PhysicsWorld.h"
int main(){
 for(float tilt:{0.f,1e-6f,1e-5f,.0001f,.001f,.01f}){
  PhysicsWorld w;w.Init();
  const auto owner=w.CreateStaticBox({0,0,0},{1,1,1},0,0);
  std::vector<PhysicsWorld::ContactParticle> p{{{0,0,0},{0,-.1635f,0},8}};
  std::vector<PhysicsWorld::ParticleBoundaryContact> rows{
    {0,owner,{0,0,0},{0,1,0},{0,0,0},false},
    {0,owner,{0,0,0},glm::normalize(glm::vec3(tilt,-1,0)),{0,-1,0},false}};
  std::vector<glm::vec3> impulses;w.SolveParticleContacts(p,rows,impulses);
  const auto s=w.GetParticleContactStats();
  std::cout<<std::setprecision(17)<<"tilt="<<tilt<<" vx="<<p[0].velocity.x<<" vy="<<p[0].velocity.y<<" KE="<<4.0*glm::dot(glm::dvec3(p[0].velocity),glm::dvec3(p[0].velocity))<<" residual="<<s.maximumClosingSpeed<<" iterations="<<s.iterations<<" acceleration="<<s.normalAccelerations<<" breakdowns="<<s.normalAccelerationBreakdowns<<" cap="<<s.iterationCapReached<<'\n';
 }
}
