#include "EngineHost.h"
#include "RuntimeWorld.h"
#include "GameSession.h"
#include "Simulation.h"
#include "Project.h"
#include "SceneSerialization.h"
#include "WorldState.h"
#include "ProductionFluidCoupling.h"
#include <filesystem>
#include <cstdio>
int checks=0,failures=0;
void Check(bool value,const char* label){++checks;failures+=!value;std::printf("%s %s\n",value?"PASS":"FAIL",label);}
int main(int argc,char** argv){namespace fs=std::filesystem;auto out=argc==3?fs::path(argv[2]):fs::path("build/m49-application");fs::create_directories(out);
 std::string error;EngineHost host;Check(host.Init("M49 focused application",320,240,false,error),"real EngineHost");if(failures)return 1;
 host.OpenProjectAssets(fs::absolute("projects/character_demo").string(),fs::absolute("projects/character_demo/Assets").string());
 Project project;Check(project.Load("projects/character_demo/character_demo.judasproj",error),"ordinary current registered project");
 auto& window=host.GetWindow();window.SetTestInputMode(true);window.Input().SetMap(project.Settings().input,error);
 RuntimeWorld world;GameSession game;
 for(const char* name:{"flat","planet","pool"}){
  if(argc==4&&std::string(argv[3])=="--pool-only"&&std::string(name)!="pool")continue;
  Scene scene;Check(LoadSceneFromFile(std::string("projects/character_demo/Scenes/")+name+".judas",scene,error),"current scene load");if(failures){std::puts(error.c_str());return 1;}
  Check(world.Build(scene,&host.Resources(),error)&&game.Begin(world,error),"ordinary world / game lifecycle");if(failures){std::puts(error.c_str());return 1;}
  EntityId controlled=0;for(const auto& o:scene.Objects())if(o.characterMotor&&!o.ui&&std::string(name)=="pool")controlled=o.id;
  if(!controlled)controlled=10;
  world.UpdateScripts(&window.Input(),1.f/60);host.Resources().WaitForAll();
  Check(world.Scripts()&&world.Scripts()->Diagnostics().empty()&&world.view.has_value(),"JS project starts motor and separate camera");
  auto* motor=world.RuntimeCharacter(controlled);auto before=motor->position;
  window.Input().SetPhysical("key:W",1);
  for(int i=0;i<60;++i){window.Input().BeginFixedStep();StepPlayedWorld(game,window,1.f/60);world.UpdateScripts(&window.Input(),1.f/60);}
  window.Input().SetPhysical("key:W",0);
  Check(world.Scripts()->Diagnostics().empty()&&glm::length(motor->position-before)>1,"real fixed path uses JS intent, not C++ player locomotion");
  if(std::string(name)!="pool"){
   Check(world.RuntimeCharacter(11)&&world.RuntimeCharacter(11)!=motor&&glm::length(world.RuntimeCharacter(11)->position-before)>1,"independent scripted second motor");
   auto support=world.RuntimeBody(41);Check(world.Physics().IsDynamicBody(support)&&glm::length(world.Physics().GetLinearVelocity(support))>.1f,"authored M45 translating support is physical");
   auto spinning=world.RuntimeBody(42);Check(glm::length(world.Physics().GetAngularVelocity(spinning))>.1f,"authored M45 rotating support is physical");
   window.Input().SetPhysical("key:Space",1);window.Input().BeginFixedStep();StepPlayedWorld(game,window,1.f/60);window.Input().SetPhysical("key:Space",0);
   Check(!motor->result.supported&&glm::dot(motor->velocity,motor->result.up)>3,"launch is project JS velocity intent through fixed logical press");
   SceneTransform placement;placement.position=motor->position+motor->orientation*glm::vec3(2,1,0);auto spawned=world.SpawnPrefab("49494949494949494949494949494903",placement,error);
   Check(spawned&&world.RuntimeCharacter(spawned),"current project prefab motor spawn");world.FixedScripts(&window.Input(),1.f/60);world.UpdateCharacters(1.f/60);
   Check(world.DestroyHierarchy(spawned,error)&&!world.RuntimeCharacter(spawned),"spawn/destruction retires safe motor access");
  }else {
   auto t=world.RuntimeDefinition(controlled)->transform;t.position={-2,1.0f,0};world.SetRuntimeTransform(controlled,t);
   window.Input().SetPhysical("key:Space",1);float start=world.RuntimeCharacter(controlled)->position.y;
   for(int i=0;i<30;++i){window.Input().BeginFixedStep();StepPlayedWorld(game,window,1.f/60);}
   window.Input().SetPhysical("key:Space",0);
   auto sample=world.FluidCoupling().SampleField(world.RuntimeCharacter(controlled)->position,{0,1,0},.9,.3,{1,0,0});
   std::printf("SWIM immersion=%g vertical_change=%g\n",sample.fraction,world.RuntimeCharacter(controlled)->position.y-start);
   Check(world.Scripts()->Diagnostics().empty()&&sample.fraction>.2f&&world.RuntimeCharacter(controlled)->position.y>start,"JS buoyancy/drag/propulsion uses existing fluid query, motor has no swimming mode");
  }
  auto saved=CaptureWorldState(world);RuntimeWorld restored;Check(restored.Build(scene,&host.Resources(),error)&&ApplyWorldState(restored,saved,error),"existing persistence restores motor entity motion");
  auto* restoredMotor=restored.RuntimeCharacter(controlled);Check(restoredMotor&&glm::length(restoredMotor->position-motor->position)<1e-5f&&glm::length(restoredMotor->velocity-motor->velocity)<1e-5f,"restored motor position and velocity coherent");restored.Destroy();
  game.ResetToAuthoredState();Check(!world.view&&world.RuntimeCharacter(controlled)->velocity==glm::vec3(0),"reset clears motor and camera runtime state");
  game.End();world.Destroy();Check(!world.RuntimeCharacter(controlled)&&!world.view,"Stop releases world-owned motors and view");
 }
 host.Shutdown();std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
