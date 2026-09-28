#include <cstdio>
#include <cstring>
#include <vector>
#include "ContactSolver.h"
#include "Narrowphase.h"
#include "PhysicsWorld.h"
namespace {
int checks=0,failures=0;
void Check(bool ok,const char* message){++checks;if(!ok){++failures;std::printf("FAIL %s\n",message);}}
bool Same(const Aabb&a,const Aabb&b){for(int k=0;k<3;++k)if(std::memcmp(&a.min[k],&b.min[k],4)||std::memcmp(&a.max[k],&b.max[k],4))return false;return true;}
void Frames(){
 ContactSolver solver;std::size_t retained=0;
 for(int step=0;step<100;++step){
  solver.Clear();
  // Fresh allocation deliberately changes body addresses across solve lifetimes.
  std::vector<RigidBody> bodies(65);
  bodies[0].inverseMass=0;
  Contact c;c.hit=true;c.normal={0,1,0};c.point={0,0,0};
  for(int i=1;i<65;++i){bodies[i].inverseMass=1;bodies[i].position={float(i),.5f,0};
   solver.AddContact(bodies[i],bodies[0],c,.5f,0);
   solver.AddContact(bodies[i],bodies[0],c,.5f,0);}
  solver.Prepare(1.f/60);solver.SolveVelocities();solver.SolvePositions();
  Check(solver.FrameNodeRequests()==65,"one current frame per distinct body");
  if(step==0){Check(solver.FrameAllocations()>0,"initial capacity growth recorded");retained=solver.FrameStorageBytes();}
  else {Check(solver.FrameAllocations()==0,"pool-backed map requires no new upstream allocation after warmup");Check(solver.FrameAllocatedBytes()==0,"zero steady frame bytes");Check(solver.FrameStorageBytes()==retained,"retained capacity stable through address churn");}
  solver.Clear(); // no live consumer when this vector is destroyed
 }
 // A small solve after a large one must not retain logical keys/frames.
 solver.Clear();RigidBody a,b;a.inverseMass=1;Contact c;c.hit=true;c.normal={0,1,0};
 solver.AddContact(a,b,c,.5f,0);solver.Prepare(.01f);Check(solver.FrameNodeRequests()==2,"old keys do not survive Clear");
}
void Bounds(){
 PreparedShapeBounds storage;
 const Shape compound=Shape::Compound({{{-.4f,.1f,.2f},{.3f,.4f,.2f}},{{.5f,-.2f,0},{.2f,.1f,.4f}}});
 std::size_t cap=0;
 for(int n=0;n<100;++n){
  const auto q=glm::angleAxis(float(n)*.003f,glm::normalize(glm::vec3(1,2,3)));ContactPreparedOrientation o(q);
  PrepareShapeBounds(compound,o,storage);if(!n)cap=storage.children.capacity();
  Check(storage.children.capacity()==cap,"rotating compound retains child capacity");
  const glm::vec3 p(float(n),-13,137);Check(Same(ShapeAabb(storage,p),ShapeAabb(compound,p,q)),"reused bounds match uncached arithmetic bits");
 }
 for(const Shape s:{Shape::Sphere(.4f),Shape::Box({.3f,.7f,.1f}),compound}){
  const glm::quat q(1,0,0,0);ContactPreparedOrientation o(q);PrepareShapeBounds(s,o,storage);
  Check(Same(ShapeAabb(storage,{0,0,0}),ShapeAabb(s,{0,0,0},q)),"shape-type rebuild clears old content correctly");
 }
}
void World(){
 PhysicsWorld w;w.Init();auto floor=w.CreateStaticBox({0,-.5f,0},{100,.5f,100},.5f,0);
 std::vector<BodyHandle> bodies;
 const auto shape=Shape::Compound({{{-.2f,0,0},{.3f,.35f,.3f}},{{.4f,.1f,0},{.2f,.2f,.2f}}});
 for(int i=0;i<24;++i)bodies.push_back(w.CreateDynamicCompoundBoxes({float(i)*2,.1f,0},shape.boxes,3,.5f,0));
 std::size_t retained=0;
 for(int step=0;step<80;++step){
  // Same population/storage requirements, genuine rotation and static motion.
  for(int i=0;i<24;++i)w.ResetBody(bodies[i],{float(i)*2,.1f,0},glm::angleAxis(.01f*float(step),glm::vec3(0,1,0)));
  w.ResetBody(floor,{0,-.5f,0},glm::angleAxis(.0001f*float(step),glm::vec3(0,1,0)));
  w.Step(1.f/60);const auto s=w.LastStepStats();Check(s.collidingPairs==24,"real compound workload retains contacts");
  if(step==0)retained=s.geometryCacheBytes;
  else{Check(s.geometryCacheAllocations==0,"no steady world frame/compound cache heap requests");Check(s.geometryCacheAllocatedBytes==0,"no steady world cache bytes allocated");Check(s.geometryCacheBytes==retained,"world cache retained bytes bounded");}
 }
 const auto stale=bodies[0];w.DestroyBody(stale);bodies[0]=w.CreateDynamicBox({0,.1f,0},{.2f,.4f,.3f},2,.5f,0);
 w.Step(1.f/60);PhysicsWorld::GeometryCacheState old;Check(!w.GetBodyGeometryCacheState(stale,old),"generation reuse cannot revive old body");
 // Force body-vector growth outside solve; old frame addresses must never be read.
 for(int i=0;i<512;++i)w.CreateStaticSphere({float(i),100,0},.1f,.5f,0);
 w.Step(1.f/60);Check(w.LastStepStats().geometryUnresolved==0,"body growth leaves no stale frame consumer");
}
}
int main(){Frames();Bounds();World();std::printf("Storage lifetime: %d checks, %d failures\n",checks,failures);return failures?1:0;}
