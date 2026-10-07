#include "PhysicsWorld.h"
#include "ScriptSystem.h"
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {unsigned checks=0;void require(bool value,const char* label){++checks;if(!value)throw std::runtime_error(label);}void distance(const PhysicsCastHit& h,float d){require(h.hit,"expected hit");require(std::abs(h.distance-d)<2e-4f,"independent distance");}}
int main(){try{
 auto owned=std::make_unique<PhysicsWorld>();auto& w=*owned;require(w.Init(),"init");
 auto target=w.CreateQueryCapsule(.3f,.6f,{{0,0,0},{1,0,0,0}});
 for(float y:{0.f,.4f,-.4f}){auto h=w.Raycast({-2,y,0},{1,0,0},4);distance(h,1.7f);require(h.body.id==target.id&&glm::length(h.normal-glm::vec3(-1,0,0))<1e-4f,"capsule cylinder normal");}
 distance(w.Raycast({0,2,0},{0,-1,0},4),1.1f);
 auto inside=w.Raycast({.2f,.4f,0},{1,0,0},1);require(inside.hit&&inside.initialOverlap&&inside.distance==0,"inside overlap contract");
 require(!w.Raycast({-2,.91f,0},{1,0,0},4).hit,"hemisphere miss");
 distance(w.Raycast({-2,0,.3f},{1,0,0},4),2.f);
 distance(w.SphereCast({-2,.4f,0},.2f,{1,0,0},4),1.5f);
 distance(w.CapsuleCast({{-2,.2f,0},{1,0,0,0}},.2f,.4f,{1,0,0},4),1.5f);
 auto box=w.BoxCast({{-2,.2f,0},{1,0,0,0}},{.2f,.2f,.2f},{1,0,0},4);distance(box,1.5f);require(glm::length(box.point-glm::vec3(-.3f,box.point.y,0))<1e-3f,"box/capsule target witness");
 auto closest=w.ClosestPoint({-2,.4f,0},3);require(closest.hit&&!closest.contains&&std::abs(closest.distance-1.7f)<1e-4f,"closest capsule cylinder");
 auto contained=w.ClosestPoint({.2f,.4f,0},1);require(contained.hit&&contained.contains&&std::abs(contained.distance-.1f)<1e-4f,"closest interior surface");
 PhysicsQueryFilter ignore;ignore.ignoredBodies.push_back(target);require(!w.Raycast({-2,0,0},{1,0,0},4,ignore).hit,"ignored capsule");
 require(w.SetBodyEnabled(target,false)&&!w.Raycast({-2,0,0},{1,0,0},4).hit,"disabled capsule");w.SetBodyEnabled(target,true);
 auto q=glm::angleAxis(.7f,glm::normalize(glm::vec3(1,2,3)));glm::vec3 p(4,-2,8);w.ResetBody(target,p,q);
 auto rotated=w.Raycast(p+q*glm::vec3(-2,.4f,0),q*glm::vec3(1,0,0),4);distance(rotated,1.7f);require(glm::length(rotated.normal-q*glm::vec3(-1,0,0))<1e-4f,"arbitrary rotation normal");
 PhysicsQueryFilter layers;w.SetCollisionFilter(target,3,kAllCategories);layers.includeLayers=CategoryMask(1)<<2;require(!w.Raycast(p+q*glm::vec3(-2,.4f,0),q*glm::vec3(1,0,0),4,layers).hit,"layer exclusion applies to capsule");layers.includeLayers=CategoryMask(1)<<3;require(w.Raycast(p+q*glm::vec3(-2,.4f,0),q*glm::vec3(1,0,0),4,layers).hit,"layer inclusion applies to capsule");
 w.DestroyBody(target);require(!w.Raycast(p+q*glm::vec3(-2,.4f,0),q*glm::vec3(1,0,0),4).hit,"destroyed capsule");
 auto sphere=w.CreateQueryCapsule(.3f,0,{{0,0,0},{1,0,0,0}});distance(w.Raycast({-2,0,0},{1,0,0},4),1.7f);require(sphere.id!=target.id,"slot reuse generation");PhysicsQueryFilter stale;stale.ignoredBodies={target};require(w.Raycast({-2,0,0},{1,0,0},4,stale).hit,"stale ignored generation does not hide reused body");auto nearer=w.CreateStaticBox({-1,0,0},{.1f,.1f,.1f},.5f,0);auto nearest=w.Raycast({-2,0,0},{1,0,0},4);distance(nearest,.9f);require(nearest.body.id==nearer.id,"nearest hit beats farther capsule");
 std::cout<<"Post-consumer capsule regressions: "<<checks<<" checks PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
