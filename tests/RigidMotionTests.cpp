#include "RigidMotion.h"
#include "PhysicsWorld.h"
#include <iostream>
#include <cmath>
#include <cstring>
int main() {
 int checks=0;
 auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("motion check "+std::to_string(checks));};
 for(float speed : {0.f,.1f,10.f,100.f,1000.f})for(float h : {1.f/240,1.f/60,.2f}) {
  RigidBody b;b.inverseMass=1;b.position={1,2,3};b.linearVelocity={2,-3,1};
  b.orientation=glm::normalize(glm::quat(.7f,.2f,-.3f,.4f));b.angularVelocity=glm::normalize(glm::vec3(1,2,3))*speed;
  auto old=b;IntegrateRigidBodyPosition(old,h);RigidMotion m;m.Begin(42,b,h);
  for(int i=0;i<19;++i)(void)m.Evaluate(double(h)*i/19);
  auto last=m.Evaluate(h);check(last.position==old.position);check(last.orientation==old.orientation);
  check(m.Segments().size()==1 && m.Segments()[0].owner==42);
 }
 RigidBody immovable; immovable.linearVelocity={1,2,3};immovable.angularVelocity={0,0,10};
 RigidMotion frozen;frozen.Begin(99,immovable,1);
 check(frozen.Evaluate(1).position==immovable.position);check(frozen.Evaluate(1).orientation==immovable.orientation);
 RigidBody b;b.inverseMass=1;b.linearVelocity={2,0,0};RigidMotion m;m.Begin(7,b,1);
 b.linearVelocity={-2,0,0};m.ChangeVelocity(b,.5);
 check(m.Evaluate(1).position==glm::vec3(0));check(m.Evaluate(.5).position==glm::vec3(1,0,0));
 check(m.Segments().size()==2 && m.Segments()[0].endPosition==m.Segments()[1].position);
 for(bool elsewhere : {false,true}) {
  PhysicsWorld w;w.Init();auto b=w.CreateDynamicSphere({100,100,100},.5,1,0,0);
  w.SetAngularVelocity(b,{0,0,100});w.SetLinearVelocity(b,{1,2,3});
  if(elsewhere){w.CreateStaticBox({0,-.5,0},{10,.5,10},0,1);auto x=w.CreateDynamicSphere({0,.501,0},.5,1,0,1);w.SetLinearVelocity(x,{0,-1,0});}
  RigidBody oracle;oracle.inverseMass=1;oracle.position={100,100,100};oracle.linearVelocity={1,2,3};oracle.angularVelocity={0,0,100};
  IntegrateRigidBodyPosition(oracle,1.f/60);w.Step(1.f/60);
  check(w.GetTransform(b).position==oracle.position);check(w.GetTransform(b).rotation==oracle.orientation);
 }
 std::cout<<"Rigid motion checks "<<checks<<" PASS (event restart/query integration remains Stage B/F)\n";
}
