// Read-only first-90-step diagnostic. Same inputs as committed walk.txt during
// this interval (W begins at30); actual GameSession/StepPlayedWorld, no altered
// scene or physics. Additional post-step sweep is observation only.
#include <iomanip>
#include <iostream>
#include "GameSession.h"
#include "RuntimeWorld.h"
#include "SceneSerialization.h"
#include "Simulation.h"
#include "SimulationTiming.h"
#include "Window.h"
SceneObjectId Id(const RuntimeWorld& world,BodyHandle h){
 auto id=world.EntityIdOfBody(h);if(id!=kInvalidSceneObjectId)return id;
 for(const auto&b:world.StaticBodies())if(b.handle.id==h.id)return b.id;
 for(const auto&b:world.Terrains())if(b.handle.id==h.id)return b.id;
 return kInvalidSceneObjectId;
}
int main(){
 Scene scene;std::string error;RuntimeWorld world;GameSession session;Window window;window.SetTestInputMode(true);
 if(!LoadSceneFromFile("assets/scenes/terrain.judas",scene,error)||!world.Build(scene,nullptr,error)||!session.Begin(world,error)){std::cerr<<error;return 1;}
 std::cout<<std::setprecision(17);
 for(int step=0;step<90;++step){
  if(step==30)window.SetTestActionState(Action::MoveForward,true);
  session.HandleFrameInput(window,false,false,false,false,false);
  StepPlayedWorld(session,window,SimulationTiming::kFixedTimestep);
  const auto&p=session.Player();const auto x=p.GetPosition(),v=p.GetVelocity();
  const auto forward=world.Physics().SweepPlayerShape(x,p.GetOrientation(),v*SimulationTiming::kFixedTimestep);
  std::cout<<"{\"step\":"<<step<<",\"player\":["<<x.x<<','<<x.y<<','<<x.z<<"],\"velocity\":["<<v.x<<','<<v.y<<','<<v.z<<"],\"grounded\":"<<p.IsGrounded()<<",\"support_scene_id\":"<<Id(world,p.GetSupportBodyHandle())<<",\"post_step_forward_hit\":"<<forward.hit<<",\"post_step_forward_scene_id\":"<<Id(world,forward.hitBody)<<",\"objects\":[";
  bool comma=false;for(const auto&b:world.DynamicBodies()){if(comma)std::cout<<',';comma=true;const auto q=b.GetPosition();const auto u=world.Physics().GetLinearVelocity(b.Handle());std::cout<<"{\"id\":"<<Id(world,b.Handle())<<",\"position\":["<<q.x<<','<<q.y<<','<<q.z<<"],\"velocity\":["<<u.x<<','<<u.y<<','<<u.z<<"]}";}std::cout<<"]}\n";
 }
}
