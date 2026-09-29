// FTFT6: the ordinary standalone Application loop opens the editor's project.
// Hooks observe/inject a frame clock; IO, resource pumping, simulation and GL
// rendering are the production paths. No alternative simulation is implemented.
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "Project.h"
#include "RuntimeWorld.h"
#include "SceneFingerprint.h"
#include "SceneSerialization.h"

namespace {
int checks=0,failures=0;
void Check(bool good,const std::string& name){++checks;if(!good)++failures;std::printf("FTFT6 CHECK %s %s\n",name.c_str(),good?"PASS":"FAIL");}
std::string Read(const std::string& path){std::ifstream s(path,std::ios::binary);return {std::istreambuf_iterator<char>(s),{}};}
void Quit(){SDL_Event e{};e.type=SDL_QUIT;Check(SDL_PushEvent(&e)==1,"runtime_quit_event");}
}
int main(int argc,char**argv){
 const std::string projectPath=argc>1?argv[1]:"projects/tiny_game/tiny_game.judasproj";
 Project project;Scene authored;std::string error,fingerprint;
 Check(project.Load(projectPath,error),"project_parse");
 Check(LoadSceneFromFile(project.StartupScenePath(),authored,error),"authored_startup_scene_parse");
 Check(ComputeSceneFingerprint(authored,fingerprint,error),"independent_authored_fingerprint");
 if(failures)return 1;
 const std::string sceneBytes=Read(project.StartupScenePath());
 std::set<std::string> assets;
 for(const auto& o:authored.Objects())if(o.render){if(!o.render->meshAsset.empty())assets.insert(o.render->meshAsset);if(!o.render->textureAsset.empty())assets.insert(o.render->textureAsset);}
 // This run explicitly tests fresh authored startup, not saved delta application.
 // FTFT1 and the editor workflow test valid/incompatible persisted deltas.
 unsetenv("JUDAS_TEST_SCRIPT");unsetenv("JUDAS_RESOURCE_MODE");setenv("JUDAS_WORLD_STATE","none",1);
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.0f/60.0f;};
 int frames=0;bool observed=false;std::vector<double> frameMs;std::chrono::steady_clock::time_point frameStart;
 control.hostReady=[&](EngineHost& h){Check(!h.Resources().BlockingMode()&&h.Resources().Jobs()==&h.Jobs()&&h.Resources().GetRenderer()==&h.GetRenderer(),"runtime_normal_async_path");};
 control.worldReady=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){
  Check(w.BaselineFingerprint()==fingerprint,"runtime_matches_authored_startup_baseline");
  Check(w.Settings().name==authored.Settings().name,"runtime_startup_scene_setting");
  Check(w.CelestialParticipants().empty()&&w.PointMassSources().empty()&&w.Terrains().empty()&&!w.HasFluid()&&!w.GetAtmosphere(),"ordinary_project_no_planetary_or_fluid_setup");
  for(const auto& o:authored.Objects())if(o.body&&o.body->motion==SceneBodyMotion::Dynamic){
   EntityPhysicalState s;const bool found=w.GetEntityState(o.id,s);
   Check(found&&s.position==o.transform.position&&s.linearVelocity==o.body->initialLinearVelocity,"runtime_initial_body_"+std::to_string(o.id));
  }
 };
 control.beforeFrame=[&](EngineHost&,RuntimeWorld&,InteractivePlay&){frameStart=std::chrono::steady_clock::now();};
 control.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& p){
  ++frames;frameMs.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-frameStart).count());
  bool ready=true;for(const auto& id:assets)ready&=h.Resources().StateOf(id)==ResourceState::Ready;
  if(!observed&&p.FixedStepsSinceReset()>=30&&ready){
   Check(p.FixedStepsSinceReset()>=30&&!p.IsPaused(),"runtime_ordinary_fixed_steps");
   Check(h.GetRenderer().Stats().drawCalls>0&&h.GetRenderer().Stats().triangles>0,"runtime_actual_render_submission");
   Check(w.BaselineFingerprint()==fingerprint,"runtime_baseline_stays_authored");
   Check(Read(project.StartupScenePath())==sceneBytes,"runtime_scene_file_unchanged");
   Check(ready,"runtime_project_assets_ready");
   for(const auto& o:authored.Objects())if(o.render&&!o.render->meshAsset.empty()){
    MeshData data;const auto mesh=h.Resources().TryGetMesh(o.render->meshAsset);
    Check(mesh.IsValid()&&h.GetRenderer().ReadMeshForDiagnostics(mesh,data)&&!data.vertices.empty(),"runtime_real_gpu_mesh_"+std::to_string(o.id));
   }
   std::printf("FTFT6 RUNTIME frames=%d steps=%zu fingerprint=%s\n",frames,p.FixedStepsSinceReset(),fingerprint.c_str());observed=true;Quit();
  }else if(frames==2000){Check(false,"runtime_progress_watchdog");Quit();}
 };
 std::string program="judas";std::string argument=projectPath;char* av[]={program.data(),argument.data()};
 Application app;const int result=app.Run(2,av,&control);
 Check(result==0&&observed,"runtime_application_completed");
 Check(Read(project.StartupScenePath())==sceneBytes,"runtime_shutdown_scene_unchanged");
 for(std::size_t i=0;i<frameMs.size();++i)std::printf("FTFT6 FRAME %zu %.9f\n",i,frameMs[i]);
 std::printf("FTFT6 standalone: %d checks, %d failures\n",checks,failures);
 return failures?1:0;
}
