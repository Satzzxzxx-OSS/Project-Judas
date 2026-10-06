#include "PhysicsWorld.h"
#include <glm/gtc/quaternion.hpp>
#include <chrono>
#include <cstdio>
int checks=0,failures=0;void Check(bool pass,const char* label){++checks;failures+=!pass;std::printf("%s %s\n",pass?"PASS":"FAIL",label);}
int main(){
 for(auto type:{JointType::Fixed,JointType::Hinge,JointType::Ball,JointType::Slider}){
  PhysicsWorld w;w.Init();auto b=w.CreateDynamicBox({0,0,0},{.5f,.5f,.5f},1,0,0);JointSettings s;s.type=type;s.bodyA=b;
  auto h=w.CreateJoint(s);Check(h.IsValid(),"joint creation");
  w.SetLinearVelocity(b,{1,2,3});w.SetAngularVelocity(b,{2,3,4});
  for(int i=0;i<120;++i)w.Step(1.f/60);
  auto pose=w.GetTransform(b);auto v=w.GetAngularVelocity(b);
  if(type==JointType::Fixed)Check(glm::length(pose.position)<.0001f&&glm::length(v)<.0001f,"fixed relative pose");
  if(type==JointType::Hinge)Check(glm::length(pose.position)<.0001f&&std::abs(v.x)>1&&std::abs(v.y)+std::abs(v.z)<.001f,"hinge only local X rotation");
  if(type==JointType::Ball)Check(glm::length(pose.position)<.0001f&&glm::length(v)>1,"ball anchors / free rotation");
  if(type==JointType::Slider)Check(std::abs(pose.position.x)>1&&std::abs(pose.position.y)+std::abs(pose.position.z)<.0001f&&glm::length(v)<.0001f,"slider local axis only");
 }
 PhysicsWorld w;w.Init();auto b=w.CreateDynamicSphere({0,0,0},.3,1,0,0);JointSettings s;s.bodyA=b;s.type=JointType::Slider;s.limits=true;s.lower=-.5;s.upper=.5;s.motor=true;s.speed=-3;s.maxForce=2;
 auto h=w.CreateJoint(s);float peak=0;for(int i=0;i<180;++i){w.Step(1.f/60);JointState state;w.GetJoint(h,state);peak=std::max(peak,std::abs(state.motorImpulse));}
 Check(std::abs(w.GetTransform(b).position.x)<=.501f,"slider limit / bounded motor");Check(peak<=2.f/60+1e-6f&&peak>0,"motor impulse force bound");
 s.motor=false;s.limits=false;s.spring=true;s.rest=0;s.stiffness=20;s.damping=4;w.SetJoint(h,s);w.ResetBody(b,{1,0,0},{1,0,0,0});for(int i=0;i<240;++i)w.Step(1.f/60);Check(glm::length(w.GetTransform(b).position)<.01f&&glm::length(w.GetLinearVelocity(b))<.02f,"implicit spring/damping settles");
 w.SetBodyEnabled(b,false);JointState state;Check(w.GetJoint(h,state)&&!state.active,"disabled body deactivates joint");w.SetBodyEnabled(b,true);Check(w.GetJoint(h,state)&&state.active,"reenabled body activates joint");w.DestroyBody(b);auto fresh=w.CreateDynamicSphere({0,0,0},.3,1,0,0);Check(fresh.id!=b.id&&!w.GetJoint(h,state),"destroy and body slot reuse cannot revive joint");
 w.Shutdown();w.Init();auto a=w.CreateDynamicSphere({0,0,0},.2,1,0,0),c=w.CreateDynamicSphere({2,0,0},.2,3,0,0);s={};s.bodyA=a;s.bodyB=c;s.anchorA={1,0,0};s.anchorB={-1,0,0};auto pair=w.CreateJoint(s);w.SetLinearVelocity(a,{4,0,0});for(int i=0;i<60;++i)w.Step(1.f/60);
 Check(std::abs(w.GetLinearVelocity(a).x-1)<.001f&&std::abs(w.GetLinearVelocity(c).x-1)<.001f,"unequal-mass dynamic pair conserves linear impulse");Check(std::abs(w.GetTransform(c).position.x-w.GetTransform(a).position.x-2)<.001f,"fixed dynamic pair stays connected");w.DestroyJoint(pair);
 auto rotation=glm::angleAxis(.8f,glm::normalize(glm::vec3(1,2,3)));w.ResetBody(a,{0,0,0},rotation);s={};s.bodyA=a;s.type=JointType::Slider;s.frameB=rotation;h=w.CreateJoint(s);w.SetLinearVelocity(a,rotation*glm::vec3(1,2,3));for(int i=0;i<60;++i)w.Step(1.f/60);auto local=glm::inverse(rotation)*w.GetTransform(a).position;Check(std::abs(local.x-1)<.001f&&std::abs(local.y)+std::abs(local.z)<.001f,"arbitrary rotated joint covariance");
 w.Shutdown();w.Init();b=w.CreateDynamicBox({0,0,0},{.5f,.5f,.5f},1,0,0);s={};s.type=JointType::Hinge;s.bodyA=b;s.limits=true;s.lower=-.4f;s.upper=.4f;s.motor=true;s.speed=2;s.maxForce=3;h=w.CreateJoint(s);peak=0;
 for(int i=0;i<120;++i){w.Step(1.f/60);w.GetJoint(h,state);peak=std::max(peak,std::abs(state.motorImpulse));}
 Check(std::abs(state.coordinate)<=.401f&&peak<=3.f/60+1e-6f,"hinge angular limits and bounded torque");
 w.Shutdown();w.Init();auto floor=w.CreateStaticBox({0,-.5f,0},{10,.5f,10},.5f,0);(void)floor;
 a=w.CreateDynamicSphere({0,2,0},.3f,1,.5f,0);c=w.CreateDynamicSphere({1,2,0},.3f,1,.5f,0);s={};s.bodyA=a;s.bodyB=c;s.anchorA={.5f,0,0};s.anchorB={-.5f,0,0};w.CreateJoint(s);w.SetLinearVelocity(a,{0,-3,0});w.SetLinearVelocity(c,{0,-3,0});
 for(int i=0;i<60;++i)w.Step(1.f/60);
 Check(w.GetTransform(a).position.y>=.299f&&w.GetTransform(c).position.y>=.299f&&std::abs(glm::length(w.GetTransform(a).position-w.GetTransform(c).position)-1)<.005f,"joint assembly interacts with actual impact/contact path");
 w.Shutdown();w.Init();for(int i=0;i<100;++i){auto body=w.CreateDynamicSphere({float(i*2),0,0},.2,1,0,0);s={};s.bodyA=body;s.anchorB={float(i*2),0,0};w.CreateJoint(s);}auto start=std::chrono::steady_clock::now();for(int i=0;i<120;++i)w.Step(1.f/60);std::printf("PERF 100 joints mean whole-step %.6f ms\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/120);
 // D-01 regression: a slider must stay stable at any travel distance. A small, low-inertia
 // body held by a world slider, under gravity, with zero initial error, at several separations
 // from the world anchor along the axis (previously diverged beyond ~1-2 m).
 for(float sep:{0.f,1.f,2.f,3.f,6.f,12.f}){
  w.Shutdown();w.Init();b=w.CreateDynamicBox({sep,2,0},{.24f,.04f,.1f},75,0,0);s={};s.type=JointType::Slider;s.bodyA=b;s.anchorB={0,2,0};w.CreateJoint(s);
  float deviation=0;for(int i=0;i<240;++i){w.ApplyLinearAcceleration(b,{0,-9.81f,0},1.f/60);w.Step(1.f/60);auto p=w.GetTransform(b).position;deviation=std::max(deviation,std::abs(p.y-2)+std::abs(p.z));}
  char label[96];std::snprintf(label,sizeof label,"long-travel slider stable at %.0f m separation (deviation %.4f m)",sep,deviation);
  Check(deviation<.01f&&glm::length(w.GetLinearVelocity(b))<.05f&&glm::length(w.GetAngularVelocity(b))<.05f,label);
 }
 // Same with a moving body travelling along the axis through several metres.
 w.Shutdown();w.Init();b=w.CreateDynamicBox({0,2,0},{.24f,.04f,.1f},75,0,0);s={};s.type=JointType::Slider;s.bodyA=b;s.anchorB={0,2,0};w.CreateJoint(s);w.SetLinearVelocity(b,{6,0,0});
 {float deviation=0;for(int i=0;i<120;++i){w.ApplyLinearAcceleration(b,{0,-9.81f,0},1.f/60);w.Step(1.f/60);auto p=w.GetTransform(b).position;deviation=std::max(deviation,std::abs(p.y-2)+std::abs(p.z));}
  Check(deviation<.01f&&std::abs(w.GetTransform(b).position.x-12)<.05f&&std::abs(w.GetLinearVelocity(b).x-6)<.01f,"slider carries a moving body 12 m along its axis");}
 // Dynamic pair on a long slider: equivalent rows must still conserve linear momentum.
 w.Shutdown();w.Init();a=w.CreateDynamicBox({0,0,0},{.24f,.04f,.1f},2,0,0);c=w.CreateDynamicBox({3,0,0},{.24f,.04f,.1f},3,0,0);s={};s.type=JointType::Slider;s.bodyA=a;s.bodyB=c;w.CreateJoint(s);
 w.SetLinearVelocity(a,{0,2,0});for(int i=0;i<120;++i)w.Step(1.f/60);
 {auto momentum=2.f*w.GetLinearVelocity(a)+3.f*w.GetLinearVelocity(c);auto rel=glm::inverse(w.GetTransform(a).rotation)*(w.GetTransform(c).position-w.GetTransform(a).position);
  Check(glm::length(momentum-glm::vec3(0,4,0))<.01f&&std::abs(rel.y)+std::abs(rel.z)<.01f&&glm::length(w.GetLinearVelocity(a))<10&&glm::length(w.GetAngularVelocity(a))<10,"long dynamic slider pair conserves momentum, stays on axis and bounded");}
 // D-02 regression: enabling a limited joint that is already past its limit must stop and
 // correct it gently, not rebound it near-elastically.
 for(float violation:{.067f,.3f}){
  w.Shutdown();w.Init();b=w.CreateDynamicBox({0,2,0},{.24f,.04f,.1f},75,0,0);s={};s.type=JointType::Slider;s.bodyA=b;s.anchorB={-violation,2,0};s.limits=true;s.lower=0;s.upper=8;s.enabled=false;
  // coordinate = B anchor relative to A along A's X = -violation (past lower=0); A moves +X (deeper).
  h=w.CreateJoint(s);w.SetLinearVelocity(b,{6,0,0});s.enabled=true;w.SetJoint(h,s);
  // Before the fix the rebound was 0.2*violation/dt (0.80 and 3.6 m/s) and persisted.
  float rebound=0;JointState js;bool recovered=false;for(int i=0;i<90;++i){w.Step(1.f/60);rebound=std::max(rebound,-w.GetLinearVelocity(b).x);w.GetJoint(h,js);recovered|=js.coordinate>=-.01f;}
  char label[112];std::snprintf(label,sizeof label,"violated limit engaged at %.3f m: rebound %.3f m/s, recovered %d",violation,rebound,int(recovered));
  Check(rebound<=.51f&&recovered&&js.coordinate<1.f,label);
 }
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
