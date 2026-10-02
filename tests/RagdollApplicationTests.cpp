#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
int checks=0,failures=0;
void Check(bool ok,const char* text){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",text);}
int main(int argc,char** argv){
 namespace fs=std::filesystem;fs::path out=argc==3?argv[2]:"build/m48-application";fs::create_directories(out);
 auto fixture=out/"project";fs::remove_all(fixture);fs::copy("projects/ragdoll_demo",fixture,fs::copy_options::recursive);
 fs::rename(fixture/"Assets/scripts/animation.js",fixture/"Assets/scripts/demo.js");
 std::ofstream(fixture/"Assets/scripts/demo.js.judasmeta")<<"JudasAssetMeta 1\nid \"46464646464646464646464646464604\"\ntype script\nsource \"\"\n";
 std::ofstream(fixture/"Assets/scripts/animation.js")<<R"JS(
import Demo from './demo.js';import {world,scenes,session} from 'judas';
export default class extends Demo {
 update(){super.update();if(this.state.checked||!this.a.info.ready)return;
 const assert=(v,m)=>{if(!v)throw Error(m);};
 if(session.get('ragdollReload')){assert(!world.entity('10').ragdoll.active&&!world.entity('11').ragdoll.active,'fresh scene has no articulation');this.state.checked=true;session.set('animationChecks',2);return;}
 this.frames=(this.frames||0)+1;
 const r=world.entity('10').ragdoll;
 if(this.frames===1){this.a.seek(.5);r.enter();assert(r.active,'activation');
   this.stale=r.body('Tip');assert(this.stale&&this.stale.valid,'safe mapped body');this.stale.applyImpulse({x:3,y:1,z:0});
   const root=this.spawn();root.ragdoll.enter();assert(root.ragdoll.active&&root.ragdoll.body('Root').id!==r.body('Root').id,'independent spawned ragdoll');root.destroy();assert(!root.valid,'hierarchy teardown');
 }
 if(this.frames===12){r.leave(.2);assert(!r.active&&!this.stale.valid,'released body handle');let rejected=false;try{this.stale.applyImpulse({x:1,y:0,z:0});}catch(e){rejected=true;}assert(rejected,'stale physics fails safely');}
 if(this.frames===30){assert(!r.active,'return complete');r.enabled=false;let rejected=false;try{r.enter();}catch(e){rejected=true;}assert(rejected,'disabled rejected');r.enabled=true;world.entity('11').ragdoll.enter();session.set('ragdollReload',true);scenes.reload();}
 }

})JS";
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[](EngineHost& host){std::string error;host.Audio().Init(error,true);};
 int frames=0,checksDone=0;
 control.afterFrame=[&](EngineHost&,RuntimeWorld& world,InteractivePlay& play){
  ++frames;
  if(world.Scripts()){auto faults=world.Scripts()->Diagnostics();if(!faults.empty()){for(auto& f:faults)std::printf("FAULT %s\n",f.message.c_str());Check(false,"JS animation callbacks have no faults");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return;}
   auto states=world.Scripts()->Capture();bool done=!states.empty()&&states[0].json.find("\"checked\":true")!=std::string::npos;
   if(done&&world.SceneControl()->Get("animationChecks")=="2"&&checksDone==0){checksDone=frames;Check(true,"real JS ragdoll enter/return/impulse, prefab and stale handles");Check(world.RuntimeAnimation(10)&&world.RuntimeAnimation(10)->asset,"scene reload reconstructs fresh animation state");SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;SDL_PushEvent(&key);}
  }
  if(checksDone&&frames==checksDone+1){auto doc=world.UI().Find("game_ui");Check(play.IsPaused()&&doc&&world.UI().Element(doc,"pause")->visible,"demo authored pause menu opens");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
  if(frames>240){Check(false,"ragdoll app completed within deterministic frame guard");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
 };
 std::string path=(fixture/"ragdoll_demo.judasproj").string();char name[]="judas";char* args[]={name,path.data()};Application app;
 Check(app.Run(2,args,&control)==0&&checksDone>0,"normal Application ragdoll startup/reload/shutdown");
 std::printf("SUMMARY %d checks %d failures frames=%d\n",checks,failures,frames);return failures?1:0;
}
