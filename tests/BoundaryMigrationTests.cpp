// M51 executes ordinary project policy through the production QuickJS/physics path.
#include "EngineHost.h"
#include "RuntimeWorld.h"
#include "GameSession.h"
#include "Simulation.h"
#include "Project.h"
#include "SceneSerialization.h"
#include "SceneSession.h"
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
namespace fs=std::filesystem;
int checks=0,failures=0;
void Check(bool v,const char* message){++checks;failures+=!v;std::printf("%s %s\n",v?"PASS":"FAIL",message);}
int GravityProof(bool baseline){
 std::string error;Project project;Check(project.Load("projects/boundary_demo/boundary_demo.judasproj",error),"actual project registry loads");Scene authored;Check(LoadSceneFromFile(baseline?"docs/evidence/m51/gravity-followup/planet-before.judas":"projects/boundary_demo/Scenes/planet.judas",authored,error),"actual radial demo parses");
 for(auto& object:authored.Objects()){object.scripts.clear();object.ui.reset();}
 RuntimeWorld world;world.legacyGameplay=false;Check(world.Build(authored,nullptr,error,&project.Settings().classification),"actual gravity content builds without a special simulation path");if(failures){std::puts(error.c_str());return 1;}
 const glm::vec3 flatPoint{6,31.5f,5},radialPoint{15,31.5f,5};
 auto sampled=world.Gravity().Sample(flatPoint);std::printf("DEMO_FLAT_SAMPLE %.8f %.8f %.8f\n",sampled.x,sampled.y,sampled.z);
 if(baseline){Check(glm::length(sampled-glm::vec3(0,-9.81f,0))>.5f,"reproduced original flat-area radial gravity defect");world.Destroy();return failures?1:0;}
 Check(glm::length(sampled-glm::vec3(0,-9.81f,0))<1e-5f,"actual flat-area authored region supplies uniform gravity");
 Check(glm::length(world.Gravity().Sample(radialPoint)+glm::normalize(radialPoint)*9.81f)<1e-5f,"planet gravity away from flat zone remains unchanged");
 auto* motor=world.RuntimeCharacter(10);
 for(auto position:{radialPoint,flatPoint,radialPoint}){motor->Reset(position,{1,0,0,0});auto expected=world.Gravity().Sample(position);world.UpdateCharacters(1.f/60);Check(glm::length(motor->result.gravity-expected)<1e-5f,"real CharacterMotor samples applicable region at current position");}
 // Ordinary dynamic-body gravity uses the same resolver, not the motor result.
 auto body=world.RuntimeBody(40);auto transform=world.RuntimeDefinition(40)->transform;transform.position=flatPoint;world.SetRuntimeTransform(40,transform);world.Physics().SetLinearVelocity(body,{0,0,0});
 PrepareDynamicBodiesForStep(world.DynamicBodies(),world.Gravity(),world.Physics(),1.f/60);world.Physics().Step(1.f/60);
 Check(glm::length(world.Physics().GetLinearVelocity(body)-glm::vec3(0,-9.81f/60,0))<1e-4f,"ordinary rigid body receives the same uniform acceleration");world.Destroy();
 for(auto rotation:{glm::quat(1,0,0,0),glm::angleAxis(.8f,glm::normalize(glm::vec3(1,2,3)))}){
  Scene scene;auto& region=scene.CreateObject("Local uniform region");region.transform.position=rotation*glm::vec3(0,.9f,0);region.transform.rotation=rotation;region.gravity=SceneGravityComponent{};region.gravity->kind=SceneGravityKind::Uniform;region.gravity->magnitude=9.81f;region.gravity->regionRadius=2.5f;
  auto& broad=scene.CreateObject("Broad radial region");broad.transform.position=rotation*glm::vec3(0,-10,0);broad.gravity=SceneGravityComponent{};broad.gravity->kind=SceneGravityKind::Radial;broad.gravity->magnitude=9.81f;broad.gravity->regionRadius=100;
  auto& floor=scene.CreateObject("Physical support");floor.transform.position=rotation*glm::vec3(0,-.5f,0);floor.transform.rotation=rotation;floor.body=SceneBodyComponent{};floor.body->halfExtents={12,.5,12};
  auto& character=scene.CreateObject("Motor");auto id=character.id;character.transform.position=rotation*glm::vec3(-4,.92f,0);character.transform.rotation=rotation;character.characterMotor=CharacterMotorSettings{};
  Check(world.Build(scene,nullptr,error),"ordinary rotated mixed-region runtime builds");if(failures){std::puts(error.c_str());return 1;}auto* moving=world.RuntimeCharacter(id);
  bool sawRadial=false,sawUniform=false,sawRestored=false,independent=false,stable=true;
  for(int frame=0;frame<320;++frame){auto before=moving->position;auto expected=world.Gravity().Sample(before);auto uniform=glm::length(expected-rotation*glm::vec3(0,-9.81f,0))<1e-5f;
   if(!uniform&&!sawUniform)sawRadial=true;
   if(uniform&&sawRadial)sawUniform=true;
   if(!uniform&&sawUniform)sawRestored=true;
   moving->velocity=rotation*glm::vec3(frame<160?3:-3,0,0);world.Physics().Step(1.f/60);world.UpdateCharacters(1.f/60);
   stable&=std::isfinite(glm::dot(moving->position,moving->position))&&glm::length(moving->position-before)<.2f&&glm::length(moving->result.gravity-expected)<1e-5f;
   if(moving->result.supported&&glm::dot(moving->result.supportNormal,-glm::normalize(expected))<.999f)independent=true;
  }
  Check(sawRadial&&sawUniform&&sawRestored,"continuous motion crosses radial/uniform boundary and returns");
  Check(stable,"motor movement remains finite and bounded through transitions");Check(independent,"physical support normal remains distinct from gravity direction");
  Check(glm::length(world.Gravity().Sample(rotation*glm::vec3(0,1,0))-rotation*glm::vec3(0,-9.81f,0))<1e-5f,"uniform gravity uses authored orientation, not global Y");world.Destroy();
 }
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
int main(int argc,char** argv){if(argc>1)return GravityProof(std::string(argv[1])=="--gravity-baseline");std::string error;Project project;
 Check(project.Load("projects/boundary_demo/boundary_demo.judasproj",error)&&!project.Settings().legacyGameplay,"ordinary project explicitly disables historical gameplay");
 ProjectSettings parsed;Check(Project::ParseFromString(Project::SerializeToString(project.Settings()),parsed,error)&&!parsed.legacyGameplay,"execution setting round trips");
 ProjectSettings legacy;Check(Project::ParseFromString("JudasProject 1\nname \"Old\"\nstartup-scene \"Scenes/main.judas\"\nassets-dir \"Assets\"\nscenes-dir \"Scenes\"\nsaves-dir \"Saves\"\n",legacy,error)&&legacy.legacyGameplay,"missing setting preserves historical compatibility");
 EngineHost host;Check(host.Init("M51 boundary migration",320,240,false,error),"normal engine host");if(failures){std::puts(error.c_str());return 1;}
 host.OpenProjectAssets(fs::absolute("projects/boundary_demo").string(),fs::absolute("projects/boundary_demo/Assets").string());
 auto& window=host.GetWindow();window.SetTestInputMode(true);window.Input().SetMap(project.Settings().input,error);
 RuntimeWorld world;GameSession game;
 auto load=[&](const char* name){game.End();world.Destroy();Scene scene;const auto path=project.Resolve(std::string("Scenes/")+name+".judas");
  bool ok=LoadSceneFromFile(path,scene,error);if(!ok){std::puts(error.c_str());return false;}
  world.legacyGameplay=false;world.SetSceneControl(std::make_shared<SceneSession>(project,path));
  ok=world.Build(scene,&host.Resources(),error,&project.Settings().classification)&&game.Begin(world,error);if(!ok){std::puts(error.c_str());return false;}
  world.UpdateScripts(&window.Input(),1.f/60);host.Resources().WaitForAll();return true;};
 auto state=[&](){std::string result;for(auto& s:world.Scripts()->Capture())if(s.entity==10)result=s.json;return result;};
 auto press=[&](const char* key){window.Input().SetPhysical(key,1);world.UpdateScripts(&window.Input(),1.f/60);window.Input().SetPhysical(key,0);window.Input().BeginFrame();};
 auto step=[&](int count){for(int i=0;i<count;++i){window.Input().BeginFixedStep();StepPlayedWorld(game,window,1.f/60);world.UpdateScripts(&window.Input(),1.f/60);window.Input().BeginFrame();}};
 Check(load("flat"),"workshop builds normally");if(failures)return 1;
 Check(!game.UsesLegacyGameplay()&&!game.HasVehicle()&&game.Interactables().empty(),"legacy adapters inactive");
 Check(world.pointerCapture&&world.Scripts()->Diagnostics().empty(),"ordinary JS requests capture and starts without faults");
 // Deliberately place the prop in the look ray; all motion thereafter is forces.
 auto t=world.RuntimeDefinition(60)->transform;t.position={0,1.8f,3};world.SetRuntimeTransform(60,t);
 press("key:G");Check(state().find("\"held\":\"60\"")!=std::string::npos,"input/query/tag acquires ordinary body in JS");
 auto body=world.RuntimeBody(60);auto before=world.Physics().GetTransform(body).position;step(30);
 Check(world.Scripts()->Diagnostics().empty()&&glm::length(world.Physics().GetTransform(body).position-before)>.1f,"holding applies finite physical forces and tensor torque");
 press("key:H");Check(state().find("\"held\":null")!=std::string::npos&&world.Physics().GetLinearVelocity(body).z<-5,"JS mass-scaled throw releases body");
 auto character=world.RuntimeDefinition(10)->transform;character.position={3,1.1f,6};world.SetRuntimeTransform(10,character);
 press("key:G");Check(state().find("\"doorOpen\":true")!=std::string::npos,"query/tag chooses JS hinge policy");step(60);
 auto joint=world.RuntimeJoint(62);JointState jointState;Check(joint.IsValid()&&world.Physics().GetJoint(joint,jointState)&&std::abs(jointState.coordinate)>.2f,"JS door spring moves real M45 hinge");
 press("key:V");Check(state().find("\"third\":true")!=std::string::npos,"camera selection belongs to script");
 const auto start=std::chrono::steady_clock::now();step(120);
 std::printf("M51_SCRIPTED_FIXED_AVERAGE_MS %.6f\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/120);
 Check(world.Scripts()->Diagnostics().empty(),"workshop composition remains fault-free");
 // Even an absent script view must not enable old movement or spawn policy.
 game.End();Scene empty;world.Build(empty,&host.Resources(),error);game.Begin(world,error);auto count=world.ScriptObjects().size();window.Input().SetPhysical("key:Z",1);game.HandleFrameInput(window,false,false,false,false,false);window.Input().SetPhysical("key:Z",0);
 Check(world.ScriptObjects().size()==count,"no implicit C++ spawn without script view");
 Check(load("flight"),"ordinary flight scene builds");if(failures)return 1;
 t=world.RuntimeDefinition(70)->transform;t.position={0,1.8f,3};world.SetRuntimeTransform(70,t);press("key:F");Check(state().find("\"piloting\":true")!=std::string::npos,"JS transfers control via query/tag");
 window.Input().SetPhysical("key:W",1);step(20);window.Input().SetPhysical("key:W",0);
 Check(world.Physics().GetLinearVelocity(world.RuntimeBody(70)).z<-1,"script thrust moves normal rigid body");press("key:X");step(3);
 Check(state().find("\"sas\":true")!=std::string::npos&&world.Scripts()->Diagnostics().empty(),"script attitude controller uses generic inertia");press("key:F");step(2);Check(state().find("\"piloting\":false")!=std::string::npos,"script exits safely to CharacterMotor");
 auto stale=world.RuntimeBody(70);world.DestroyEntity(70,&error);step(1);Check(!world.Physics().IsDynamicBody(stale)&&world.Scripts()->Diagnostics().empty(),"destroyed target handle cannot alias live body");
 Check(load("planet"),"radial scene uses same project script path");step(10);Check(world.Scripts()->Diagnostics().empty(),"radial project controls execute");
 Check(load("pool"),"current copied pool registers normally");step(3);Check(world.Scripts()->Diagnostics().empty(),"JS swimming uses unchanged production fluid queries");
 game.End();world.Destroy();Check(!world.pointerCapture&&!world.Scripts(),"Stop destroys script state and capture intent");host.Shutdown();
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;}
