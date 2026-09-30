// Supporting differential oracle: compile this SAME driver against the
// accepted pre-FTFT9 PlayerController.cpp and current PlayerController.cpp.
// Emits authoritative per-step states in exact hexadecimal float notation.
#include <iomanip>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "PhysicsWorld.h"
#include "PlayerController.h"
#include "UniformGravity.h"
#include "Window.h"
int main(){std::cout<<std::hexfloat;
for(int fixture=0;fixture<3;++fixture){PhysicsWorld physics;if(!physics.Init())return 2;const glm::quat q=fixture==2?glm::angleAxis(.63f,glm::normalize(glm::vec3(1,2,-1))):glm::quat(1,0,0,0);
const BodyHandle floor=fixture==1?physics.CreateDynamicBox(q*glm::vec3(0,-.5f,0),{20,.5f,20},500,0,0):physics.CreateStaticBox(q*glm::vec3(0,-.5f,0),{20,.5f,20},0,0);
physics.ResetBody(floor,q*glm::vec3(0,-.5f,0),q);if(fixture==1){physics.SetLinearVelocity(floor,{.3f,.1f,-.2f});physics.SetAngularVelocity(floor,{0,.15f,0});}
PlayerController player(q*glm::vec3(0,.92f,0),15);if(!player.Spawn(physics))return 2;player.FixedUpdateAttached(q*glm::vec3(0,.92f,0),q);Window window;window.SetTestInputMode(true);UniformGravity gravity(q*glm::vec3(0,-9.81f,0));
for(int step=0;step<720;++step){window.SetTestActionState(Action::MoveForward,step<240||step>=500);window.SetTestActionState(Action::StrafeRight,step>=120&&step<500);if(step==40||step==390)window.RequestTestJump();if(step%37==0)window.QueueTestMouseDelta(3,-1);player.UpdateFrameInput(window);physics.Step(1.f/60.f);player.FixedUpdate(window,physics,gravity,1.f/60.f);const auto p=player.GetPosition(),v=player.GetVelocity();const auto r=player.GetOrientation();std::cout<<fixture<<','<<step<<','<<p.x<<','<<p.y<<','<<p.z<<','<<v.x<<','<<v.y<<','<<v.z<<','<<r.w<<','<<r.x<<','<<r.y<<','<<r.z<<','<<player.IsGrounded()<<','<<player.GetSupportBodyHandle().id<<'\n';}
player.Destroy(physics);physics.Shutdown();}return 0;}
