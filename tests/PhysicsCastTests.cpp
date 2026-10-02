#include "PhysicsWorld.h"
#include "RadialTerrain.h"
#include <glm/gtc/quaternion.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
int checks=0,failures=0;
void Check(bool ok,const char* name){++checks;if(!ok)++failures;std::printf("%s %s\n",ok?"PASS":"FAIL",name);}
bool Near(float a,float b){return std::abs(a-b)<3e-5f;}
int main(){
 PhysicsWorld world;world.Init();auto far=world.CreateStaticSphere({8,0,0},1,0,0);
 auto box=world.CreateStaticBox({4,0,0},{1,1,1},0,0);auto before=world.GetTransform(box);
 auto hit=world.Raycast({0,0,0},{5,0,0},10);Check(hit.hit&&hit.body.id==box.id&&Near(hit.distance,3)&&Near(hit.fraction,.3f),"nearest ray / normalized direction / fraction");
 Check(glm::length(hit.point-glm::vec3(3,0,0))<3e-5f&&glm::length(hit.normal-glm::vec3(-1,0,0))<3e-5f,"ray surface point and outward normal");
 Check(!world.Raycast({0,4,0},{1,0,0},10).hit,"ray miss");
 Check(!world.Raycast({0,0,0},{1,0,0},2.9f).hit,"maximum distance clips hit");
 PhysicsQueryFilter filter;filter.ignoredBodies={box};hit=world.Raycast({0,0,0},{1,0,0},10,filter);
 Check(hit.hit&&hit.body.id==far.id&&Near(hit.distance,7),"ignored generation-aware body");
 world.SetCollisionFilter(box,2,0);world.SetBodyTags(box,CategoryBit(3));filter={};filter.includeLayers=CategoryBit(2);filter.requiredTags=CategoryBit(3);
 Check(world.Raycast({0,0,0},{1,0,0},10,filter).body.id==box.id,"layer and required tag filter independent of physical mask");
 filter.excludedTags=CategoryBit(3);Check(!world.Raycast({0,0,0},{1,0,0},10,filter).hit,"excluded tag");filter={};filter.excludeLayers=CategoryBit(2);
 Check(world.Raycast({0,0,0},{1,0,0},10,filter).body.id==far.id,"excluded layer");
 world.SetBodyEnabled(box,false);Check(world.Raycast({0,0,0},{1,0,0},10).body.id==far.id,"disabled collider omitted");world.SetBodyEnabled(box,true);
 world.SetBodySensor(box,true);Check(world.Raycast({0,0,0},{1,0,0},10).body.id==far.id,"sensor opt-in");filter={};filter.includeSensors=true;
 Check(world.Raycast({0,0,0},{1,0,0},10,filter).body.id==box.id,"sensor explicitly queried");world.SetBodySensor(box,false);
 hit=world.SphereCast({0,0,0},.5f,{1,0,0},10);Check(hit.body.id==box.id&&Near(hit.distance,2.5f)&&Near(hit.point.x,3),"sphere cast surface witness");
 hit=world.CapsuleCast({{0,0,0},{1,0,0,0}},.25f,.75f,{1,0,0},10);Check(hit.body.id==box.id&&Near(hit.distance,2.75f),"independent capsule dimensions (no player shape needed)");
 hit=world.BoxCast({{0,0,0},{1,0,0,0}},{.5f,.5f,.5f},{1,0,0},10);Check(hit.body.id==box.id&&Near(hit.distance,2.5f)&&Near(hit.point.x,3),"box cast robust SAT and target surface witness");
 hit=world.BoxCast({{0,0,0},{1,0,0,0}},{.5f,.5f,.5f},{1,0,0},10,filter);Check(Near(hit.distance,2.5f),"box filtering shared");
 auto rotated=world.CreateStaticBox({0,0,-5},glm::angleAxis(glm::radians(45.f),glm::vec3(0,1,0)),{1,1,1},0,0);
 hit=world.Raycast({0,0,0},{0,0,-1},10);Check(hit.body.id==rotated.id&&Near(hit.distance,5-std::sqrt(2.f)),"rotated box analytic ray");
 hit=world.SphereCast({0,0,0},.5f,{0,0,-1},10);Check(hit.body.id==rotated.id&&Near(hit.distance,5-std::sqrt(2.f)-.5f),"sphere against rotated box corner");
 auto after=world.GetTransform(box);Check(before.position==after.position&&before.rotation==after.rotation&&world.GetLinearVelocity(box)==glm::vec3(0),"queries leave pose and velocity unchanged");
 auto inside=world.Raycast({4,0,0},{1,0,0},0);Check(inside.hit&&inside.initialOverlap&&inside.distance==0,"zero distance initial overlap");
 auto thin=world.CreateStaticBox({20,0,0},{.0001f,1,1},0,0);hit=world.Raycast({10,0,0},{1,0,0},10000);Check(hit.body.id==thin.id&&Near(hit.distance,9.9999f),"long ray cannot skip thin box");
 world.DestroyBody(box);auto replacement=world.CreateStaticSphere({4,0,0},.5f,0,0);filter={};filter.ignoredBodies={box};
 Check(replacement.id!=box.id&&world.Raycast({0,0,0},{1,0,0},10,filter).body.id==replacement.id,"stale ignore cannot suppress reused slot");
 bool invalid=false;try{world.Raycast({0,0,0},{0,0,0},10);}catch(const std::invalid_argument&){invalid=true;}Check(invalid,"invalid direction rejected");
 auto terrain=std::make_shared<RadialTerrain>(2,[](const glm::vec3&){return 0.f;},0);
 auto ground=world.CreateStaticTerrain({0,10,0},{1,0,0,0},terrain,0,0);
 hit=world.Raycast({0,15,0},{0,-1,0},6);Check(hit.body.id==ground.id&&Near(hit.distance,3)&&Near(hit.point.y,12)&&Near(hit.normal.y,1),"real radial terrain ray / independent flat elevation oracle");
 hit=world.SphereCast({0,15,0},.25f,{0,-1,0},6);Check(hit.body.id==ground.id&&Near(hit.distance,2.75f),"radial terrain sphere cast");
 hit=world.CapsuleCast({{0,15,0},{1,0,0,0}},.25f,.5f,{0,-1,0},6);Check(hit.body.id==ground.id&&Near(hit.distance,2.25f),"radial terrain capsule samples");
 world.Shutdown();world.Init();for(int x=0;x<32;++x)for(int z=0;z<32;++z)world.CreateStaticBox({float(x*4),0,float(z*4)},{.5f,.5f,.5f},0,0);
 PhysicsCastStats stats;auto start=std::chrono::steady_clock::now();unsigned total=0;
 for(int i=0;i<10000;++i){world.Raycast({-2,0,0},{1,0,0},130,{},&stats);total+=stats.primitivesTested;}
 double rays=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();start=std::chrono::steady_clock::now();
 for(int i=0;i<10000;++i)world.SphereCast({-2,0,0},.25f,{1,0,0},130);
 double spheres=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 Check(stats.broadphaseCandidates==32&&total==320000,"real broadphase limits 1024-body world to 32 ray candidates");
 std::printf("PERF 10000 rays %.6f ms; 10000 sphere casts %.6f ms; candidates %u / 1024\n",rays,spheres,stats.broadphaseCandidates);
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
