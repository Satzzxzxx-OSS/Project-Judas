// Same fixed-step game input under classic versus comma process numeric locale.
#include "EngineHost.h"
#include "RuntimeWorld.h"
#include "GameSession.h"
#include "Simulation.h"
#include "Project.h"
#include "SceneSession.h"
#include "SceneSerialization.h"
#include <filesystem>
#include <locale>
#include <vector>
#include <cstdio>
struct Comma:std::numpunct<char>{char do_decimal_point()const override{return ',';}};
int main(){std::string error;EngineHost host;if(!host.Init("M58 numeric invariance",640,360,false,error))return 1;host.Audio().Init(error,true);std::vector<std::vector<float>> results;std::vector<std::string> fingerprints;auto old=std::locale();for(int pass=0;pass<2;++pass){std::locale::global(pass?std::locale(std::locale::classic(),new Comma):std::locale::classic());Project p;if(!p.Load("projects/shooter_game/shooter_game.judasproj",error)){std::fprintf(stderr,"project: %s\n",error.c_str());return 2;}host.OpenProjectAssets(p.RootDir(),p.AssetsDir());Scene scene;if(!LoadSceneFromFile(p.StartupScenePath(),scene,error))return 3;auto control=std::make_shared<SceneSession>(p,p.StartupScenePath());RuntimeWorld world;world.legacyGameplay=false;world.SetSceneControl(control);if(!world.Build(scene,&host.Resources(),error,&p.Settings().classification,&p.Settings().navigation)){std::fprintf(stderr,"%s\n",error.c_str());return 4;}host.Resources().WaitForAll();auto& input=host.GetWindow().Input();host.GetWindow().SetTestInputMode(true);input.SetMap(p.Settings().input,error);input.SetPhysical("key:W",1);GameSession game;if(!game.Begin(world,error))return 5;for(int frame=0;frame<60;++frame){input.BeginFixedStep();world.UpdateUIScripts(&input,1.f/60);world.UpdateScripts(&input,1.f/60);StepPlayedWorld(game,host.GetWindow(),1.f/60);input.BeginFrame();}std::vector<float> state;for(auto h:world.Physics().AliveBodies()){auto t=world.Physics().GetTransform(h);auto v=world.Physics().GetLinearVelocity(h);for(auto x:{t.position.x,t.position.y,t.position.z,t.rotation.w,t.rotation.x,t.rotation.y,t.rotation.z,v.x,v.y,v.z})state.push_back(x);}for(auto& o:scene.Objects())if(auto* motor=world.RuntimeCharacter(o.id)){state.push_back(motor->position.x);state.push_back(motor->position.y);state.push_back(motor->position.z);state.push_back(motor->velocity.x);state.push_back(motor->velocity.y);state.push_back(motor->velocity.z);}results.push_back(state);fingerprints.push_back(world.BaselineFingerprint());game.End();world.Destroy();world.SetSceneControl(nullptr);control.reset();}
 std::locale::global(old);host.Shutdown();bool ok=!results[0].empty()&&results[0]==results[1]&&fingerprints[0]==fingerprints[1];std::printf("%s identical 60-step inputs preserve %zu state values and authored fingerprint under comma numeric locale\n",ok?"PASS":"FAIL",results[0].size());return ok?0:1;}
