// Execute the shipped project scripts with the real input, motor, queries and solver.
#include "EngineHost.h"
#include "RuntimeWorld.h"
#include "GameSession.h"
#include "Simulation.h"
#include "Project.h"
#include "SceneSerialization.h"
#include "SceneSession.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <filesystem>
int checks=0,failures=0;
void Check(bool ok,const char* label){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",label);}
double Number(const std::string& s,const char* name){auto p=s.find(std::string("\"")+name+"\":");return p==std::string::npos?-999:std::strtod(s.c_str()+p+std::strlen(name)+3,nullptr);}
int main(){std::string error;Project project;Check(project.Load("projects/shooter_game/shooter_game.judasproj",error),"ordinary registered shooter project");
 EngineHost host;Check(host.Init("M52 game check",640,360,false,error),"production engine host");if(failures){std::puts(error.c_str());return 1;}host.Audio().Init(error,true);
 host.OpenProjectAssets(std::filesystem::absolute("projects/shooter_game").string(),std::filesystem::absolute("projects/shooter_game/Assets").string());
 auto& window=host.GetWindow();window.SetTestInputMode(true);Check(window.Input().SetMap(project.Settings().input,error),"project logical controls load");
 RuntimeWorld world;GameSession game;std::shared_ptr<SceneSession> session;
 auto load=[&](){game.End();world.Destroy();Scene scene;if(!LoadSceneFromFile(project.StartupScenePath(),scene,error))return false;
  world.legacyGameplay=false;session=std::make_shared<SceneSession>(project,project.StartupScenePath());world.SetSceneControl(session);
  if(!world.Build(scene,&host.Resources(),error,&project.Settings().classification,&project.Settings().navigation)||!game.Begin(world,error))return false;
  world.UpdateScripts(&window.Input(),1.f/60);host.Resources().WaitForAll();world.UpdateScripts(&window.Input(),1.f/60);return true;};
 auto state=[&](EntityId id=10){for(const auto& x:world.Scripts()->Capture())if(x.entity==id)return x.json;return std::string{};};
 auto step=[&](int count){for(int i=0;i<count;++i){window.Input().BeginFixedStep();if(!world.UI().Paused())StepPlayedWorld(game,window,1.f/60);world.UpdateUIScripts(&window.Input(),1.f/60);world.UpdateScripts(&window.Input(),1.f/60);world.PresentationScripts(&window.Input(),1.f/60,1);window.Input().BeginFrame();}};
 auto press=[&](const char* physical){window.Input().SetPhysical(physical,1);step(1);window.Input().SetPhysical(physical,0);step(1);};
 auto aim=[&](float x,float z){auto t=world.RuntimeDefinition(10)->transform;t.position={x,.92f,z};world.SetRuntimeTransform(10,t);world.RuntimeCharacter(10)->velocity={0,0,0};step(3);};
 Check(load(),"actual project scene and scripts build");if(failures){std::puts(error.c_str());return 1;}step(20);
 Check(!game.UsesLegacyGameplay()&&world.RuntimeCharacter(10)&&world.pointerCapture,"JS motor and capture; no built-in gameplay");
 Check(world.Scripts()->Diagnostics().empty(),"all real game scripts start without errors");Check(world.QueryEntities(1).size()==12,"twelve independently tagged plates");
 const auto initial=world.RuntimeDefinition(10)->transform.position;window.Input().SetPhysical("key:W",1);step(12);window.Input().SetPhysical("key:W",0);step(12);
 Check(world.RuntimeDefinition(10)->transform.position.z<initial.z-.2f,"logical movement drives CharacterMotor");
 press("key:Space");Check(world.RuntimeCharacter(10)->velocity.y>1,"launch is project JS");step(75);aim(-2,8);
 auto idBefore=world.RuntimeDefinition(10)->id;press("key:V");Check(state().find("\"third\":true")!=std::string::npos&&world.RuntimeDefinition(10)->id==idBefore,"third person preserves same controlled entity");
 Check(world.view&&world.view->pose.position.z>world.RuntimeDefinition(10)->transform.position.z+2,"third-person view uses separate camera boom");
 press("key:V");Check(state().find("\"third\":false")!=std::string::npos,"return to first person");
 const auto normalBefore=world.Physics().GetTransform(world.RuntimeBody(103)).rotation;
 press("mouse:Left");std::printf("SHOT_STATE %s\n",state().c_str());
 Check(Number(state(),"shots")==1&&Number(state(),"score")==100&&Number(state(),"hits")==1,"first-person real ray hit scores once");
 Check(state().find("\"lastHit\":\"103\"")!=std::string::npos,"reticle ray hits intended physical plate, not shooter");
 Check(state(103).find("\"ready\":false")!=std::string::npos,"target enters cooling state independently");
 JointState js;world.Physics().GetJoint(world.RuntimeJoint(104),js);float maxAngle=std::abs(js.coordinate);
 for(int i=0;i<100;++i){step(1);world.Physics().GetJoint(world.RuntimeJoint(104),js);maxAngle=std::max(maxAngle,std::abs(js.coordinate));}
 std::printf("TARGET maximum_angle=%.6f settled=%.6f\n",maxAngle,js.coordinate);
 Check(maxAngle>.12f,"shot impulse physically swings hinge");Check(maxAngle<1.3f,"authored hinge limits hold");
 Check(std::abs(js.coordinate)<.12f&&state(103).find("\"ready\":true")!=std::string::npos,"spring/damping physically returns and rearms target");
 Check(std::abs(glm::dot(normalBefore,world.Physics().GetTransform(world.RuntimeBody(103)).rotation))>.99f,"plate returns near rest without scripted pose reset");
 Check(state(100).find("\"hitOnce\":false")!=std::string::npos,"other targets retain independent state");
 press("key:V");window.Input().AddDelta("mouse:dx",-23);window.Input().AddDelta("mouse:dy",15);world.UpdateScripts(&window.Input(),1.f/60);window.Input().BeginFrame();press("mouse:Left");Check(Number(state(),"score")==200&&Number(state(),"unique")==1,"third-person reticle and player-origin query hit same target; repeat rearm scores");step(20);
 auto cover=world.RuntimeDefinition(30)->transform;cover.position={-2.5f,1.6f,7};world.SetRuntimeTransform(30,cover);
 press("mouse:Left");Check(Number(state(),"score")==200&&state().find("\"lastHit\":\"30\"")!=std::string::npos,"third-person player-origin ray cannot shoot through cover visible beside the camera");step(20);
 aim(10,8);press("mouse:Left");Check(Number(state(),"score")==200&&Number(state(),"shots")==4,"non-target wall/miss does not score");step(20);
 // Position-only test setup aims at the ordinary untagged crate; game still uses queries/impulses.
 auto crate=world.RuntimeDefinition(30)->transform;crate.position={3,1.62f,3};world.SetRuntimeTransform(30,crate);aim(3,8);press("mouse:Left");
 Check(Number(state(),"score")==200&&world.Physics().GetLinearVelocity(world.RuntimeBody(30)).z<-.05f,"non-target dynamic body receives real impulse without score");
 auto hud=world.UI().Find("range_ui");Check(world.UI().Element(hud,"score")->text.find("200")!=std::string::npos,"authored HUD reflects JS score");
 press("key:Escape");Check(world.UI().Paused()&&!world.pointerCapture,"pause owns modal UI and releases requested pointer");auto pausedShots=Number(state(),"shots");auto pausedPos=world.RuntimeDefinition(10)->transform.position;
 window.Input().SetPhysical("key:W",1);press("mouse:Left");window.Input().SetPhysical("key:W",0);Check(Number(state(),"shots")==pausedShots&&glm::length(world.RuntimeDefinition(10)->transform.position-pausedPos)<1e-6f,"pause blocks gameplay movement/fire");
 // Resume through the real authored UI pointer route (no fake onUI invocation).
 world.UI().Layout(640,360);auto layout=world.UI().LayoutOf(hud,"resume");auto point=layout->rect.position+layout->rect.size*.5f;
 window.Input().SetPhysical("mouse:Left",1);world.UI().Input(window.Input(),point,true,640,360);window.Input().BeginFrame();window.Input().SetPhysical("mouse:Left",0);world.UI().Input(window.Input(),point,true,640,360);world.DispatchUIEvents(&window.Input(),1.f/60);
 Check(!world.UI().Paused()&&world.pointerCapture,"authored resume button calls JS and restores capture");
 step(2);Check(Number(state(),"shots")==pausedShots,"resume click does not fire through menu");
 press("key:R");Check(session->Pending(),"restart queues normal scene reload");auto stale=world.RuntimeBody(103);Check(load(),"round reconstructs through ordinary scene load");step(10);
 Check(Number(state(),"score")==0&&Number(state(),"shots")==0&&state(103).find("\"ready\":true")!=std::string::npos,"reload resets JS score/target state and rebuilds handles");
 (void)stale;
 for(int row=0;row<3;++row)for(int col=0;col<4;++col){aim(float(-6+col*4),float(-3-row*7+(row==2?-1:0)+5));press("mouse:Left");step(20);}
 Check(Number(state(),"unique")==12&&Number(state(),"score")==1800,"complete real query/impulse round hits all independently authored targets");
 Check(world.UI().Element(world.UI().Find("range_ui"),"message")->text.find("RANGE CLEARED")!=std::string::npos,"round completion is visible in authored HUD");
 const auto begin=std::chrono::steady_clock::now();step(600);std::printf("M52_FIXED_AVERAGE_MS %.6f bodies=%zu targets=12\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/600,world.Physics().AliveBodies().size());
 Check(world.Scripts()->Diagnostics().empty(),"game remains fault-free during repeated authoritative steps");game.End();world.EndScripts();world.Destroy();Check(!world.Scripts(),"Stop clears VM, bodies and target state");host.Shutdown();
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
