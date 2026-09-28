// FTFT4B-1: observation only. Links the ordinary engine; no alternate solver.
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "PhysicsWorld.h"
#include "RigidBody.h"
namespace {
void Vec(const glm::dvec3& v){std::cout<<'['<<v.x<<','<<v.y<<','<<v.z<<']';}
void Quat(const glm::quat&q){std::cout<<'['<<q.w<<','<<q.x<<','<<q.y<<','<<q.z<<']';}
struct State {std::array<glm::dvec3,3> p,v,w;double linear=0,rotation=0;glm::dvec3 momentum{0};};
State Read(PhysicsWorld&world,const std::array<BodyHandle,3>&b){
 const double mass[]={10,.125,10};State s;
 for(int i=0;i<3;++i){s.p[i]=world.GetTransform(b[i]).position;s.v[i]=world.GetLinearVelocity(b[i]);s.w[i]=world.GetAngularVelocity(b[i]);
  s.linear+=.5*mass[i]*glm::dot(s.v[i],s.v[i]);s.rotation+=.5*(.4*mass[i]*.25)*glm::dot(s.w[i],s.w[i]);s.momentum+=mass[i]*s.v[i];}
 return s;
}
void Print(const State&s){
 std::cout<<"{\"linear_energy\":"<<s.linear<<",\"rotational_energy\":"<<s.rotation<<",\"total_energy\":"<<s.linear+s.rotation<<",\"momentum\":";Vec(s.momentum);
 std::cout<<",\"bodies\":[";for(int i=0;i<3;++i){if(i)std::cout<<',';std::cout<<"{\"id\":"<<i<<",\"position\":";Vec(s.p[i]);std::cout<<",\"velocity\":";Vec(s.v[i]);std::cout<<",\"angular_velocity\":";Vec(s.w[i]);std::cout<<'}';}
 std::cout<<"],\"pair_separations\":["<<glm::length(s.p[1]-s.p[0])-1<<','<<glm::length(s.p[2]-s.p[1])-1<<"]}";
}
int Energy(){
 int failures=0,caseId=0;std::array<int,3> order{0,1,2};
 do {for(const glm::vec3 axis:{glm::vec3(1,0,0),glm::vec3(0,1,0),glm::vec3(0,0,-1)})for(int boosted:{0,1}){
  PhysicsWorld world;if(!world.Init())return 2;std::array<BodyHandle,3>b;
  const float masses[]={10,.125f,10},speed[]={1,-3,4};const glm::vec3 boost=boosted?glm::vec3(2,-1,3):glm::vec3(0);
  for(int i:order){b[i]=world.CreateDynamicSphere(float(i)*axis,.5f,masses[i],0,1);world.SetLinearVelocity(b[i],speed[i]*axis+boost);}
  const auto before=Read(world,b);const float dt=1.f/60;world.Step(dt);const auto after=Read(world,b);
  const double delta=after.linear+after.rotation-before.linear-before.rotation,res=glm::length(after.momentum-before.momentum);
  const bool badEnergy=delta>1e-4,badMomentum=res>1e-4;failures+=badEnergy||badMomentum;
  std::cout<<"{\"kind\":\"energy\",\"case\":"<<caseId++<<",\"creation_order\":["<<order[0]<<','<<order[1]<<','<<order[2]<<"],\"axis\":";Vec(axis);std::cout<<",\"boost\":";Vec(boost);
  std::cout<<",\"dt\":"<<dt<<",\"before\":";Print(before);std::cout<<",\"after\":";Print(after);
  std::cout<<",\"delta_energy\":"<<delta<<",\"momentum_residual\":"<<res<<",\"energy_failure\":"<<badEnergy<<",\"momentum_failure\":"<<badMomentum<<",\"contact_count\":"<<world.LastStepContactCount()<<",\"detection_penetrations\":[";
  bool comma=false;for(const auto&c:world.LastStepContacts()){if(comma)std::cout<<',';comma=true;std::cout<<c.penetration;}std::cout<<"]}\n";
 }}while(std::next_permutation(order.begin(),order.end()));
 return failures?1:0; // A measured physical failure remains a failing executable.
}
int Extended(){
 for(float e:{0.f,.5f}){
  PhysicsWorld w;w.Init();w.CreateStaticBox({0,-.5f,0},{10,.5f,10},0,0);auto b=w.CreateDynamicSphere({0,.501f,0},.5f,1,0,e);w.SetLinearVelocity(b,{0,-1,0});
  const double gap=double(w.GetTransform(b).position.y)-.5;const float h=1.f/60;
  for(int step=1;step<=120;++step){w.Step(h);std::cout<<"{\"kind\":\"extended_temporal\",\"e\":"<<e<<",\"step\":"<<step<<",\"dt\":"<<h<<",\"initial_gap\":"<<gap<<",\"gap\":"<<double(w.GetTransform(b).position.y)-.5<<",\"velocity\":";Vec(w.GetLinearVelocity(b));std::cout<<",\"angular_velocity\":";Vec(w.GetAngularVelocity(b));std::cout<<",\"contacts\":"<<w.LastStepContactCount()<<"}\n";}
 }
 return 0; // Observations only; physical assertions are independently evaluated by runner.
}
int Angular(const char*path){
 std::ifstream in(path);if(!in)return 2;int id,n;float h;
 while(in>>id){RigidBody state;state.inverseMass=1;state.linearVelocity={.25f,-.5f,.75f};
  in>>state.orientation.w>>state.orientation.x>>state.orientation.y>>state.orientation.z>>state.angularVelocity.x>>state.angularVelocity.y>>state.angularVelocity.z>>h>>n;
  if(!in||n<1||n>32)return 2;
  std::vector<float> parts(n);
  for(float&dt:parts)in>>dt;
  if(!in)return 2;
  const auto initial=state;auto single=state,split=state;IntegrateRigidBodyPosition(single,h);for(float dt:parts)IntegrateRigidBodyPosition(split,dt);
  std::cout<<"{\"kind\":\"angular_partition\",\"case\":"<<id<<",\"q0\":";Quat(initial.orientation);std::cout<<",\"omega\":";Vec(initial.angularVelocity);std::cout<<",\"h\":"<<h<<",\"parts\":[";
  for(int i=0;i<n;++i){if(i)std::cout<<',';std::cout<<parts[i];}std::cout<<"],\"single\":";Quat(single.orientation);std::cout<<",\"split\":";Quat(split.orientation);std::cout<<",\"omega_after_single\":";Vec(single.angularVelocity);std::cout<<",\"omega_after_split\":";Vec(split.angularVelocity);std::cout<<"}\n";
 }
 return 0;
}
}
int main(int argc,char**argv){std::cout<<std::setprecision(17)<<std::boolalpha;if(argc<2)return 2;
 const std::string mode=argv[1];if(mode=="energy")return Energy();if(mode=="extended")return Extended();if(mode=="angular"&&argc==3)return Angular(argv[2]);return 2;}
