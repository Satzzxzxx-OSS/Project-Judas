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
 namespace fs=std::filesystem;fs::path out=argc==3?argv[2]:"build/m46-application";fs::create_directories(out);
 auto fixture=out/"project";fs::remove_all(fixture);fs::copy("projects/animation_demo",fixture,fs::copy_options::recursive);
 fs::rename(fixture/"Assets/scripts/animation.js",fixture/"Assets/scripts/demo.js");
 std::ofstream(fixture/"Assets/scripts/demo.js.judasmeta")<<"JudasAssetMeta 1\nid \"46464646464646464646464646464604\"\ntype script\nsource \"\"\n";
 std::ofstream(fixture/"Assets/scripts/animation.js")<<R"JS(
import Demo from './demo.js';import {world,scenes,session} from 'judas';
export default class extends Demo {
 update(){super.update();if(this.state.checked||!this.a.info.ready)return;
 const assert=(value,message)=>{if(!value)throw Error(message);};
 assert(this.a.clips.length===2&&this.a.clips[0].name==='Wave','named clips');
 this.a.play('Stretch');this.a.loop=false;this.a.speed=2;this.a.seek(.5);this.a.pause();assert(this.a.time===.5&&!this.a.playing,'seek/pause');
 this.a.resume();assert(this.a.playing,'resume');this.a.stop();assert(this.a.time===0&&!this.a.playing,'stop/reset');
 this.a.play('Wave');this.a.loop=true;this.a.speed=1;this.a.seek(.25);
 assert(this.b.info.clip==='Stretch'&&this.b.speed===1,'independent instance');
 let rejected=false;try{this.a.play('missing');}catch(e){rejected=true;}assert(rejected,'unknown clip rejected');
 const root=this.spawn();assert(root.animation&&root.animation.info.ready,'prefab animation');const stale=root.animation;root.destroy();
 rejected=false;try{stale.info;}catch(e){rejected=true;}assert(rejected,'stale animation fails safely');
 this.state.checked=true;session.set('animationChecks',(session.get('animationChecks')||0)+1);
 if(!session.get('animationReload')){session.set('animationReload',true);scenes.reload();}
 }
})JS";
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[](EngineHost& host){std::string error;host.Audio().Init(error,true);};
 int frames=0,checksDone=0;
 control.afterFrame=[&](EngineHost&,RuntimeWorld& world,InteractivePlay& play){
  ++frames;
  if(world.Scripts()){auto faults=world.Scripts()->Diagnostics();if(!faults.empty()){for(auto& f:faults)std::printf("FAULT %s\n",f.message.c_str());Check(false,"JS animation callbacks have no faults");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return;}
   auto states=world.Scripts()->Capture();bool done=!states.empty()&&states[0].json.find("\"checked\":true")!=std::string::npos;
   if(done&&world.SceneControl()->Get("animationChecks")=="2"&&checksDone==0){checksDone=frames;Check(true,"real JS playback controls, prefab and stale handles");Check(world.RuntimeAnimation(10)&&world.RuntimeAnimation(10)->asset,"scene reload reconstructs fresh animation state");SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;SDL_PushEvent(&key);}
  }
  if(checksDone&&frames==checksDone+1){auto doc=world.UI().Find("game_ui");Check(play.IsPaused()&&doc&&world.UI().Element(doc,"pause")->visible,"demo authored pause menu opens");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
  if(frames>240){Check(false,"animation app completed within deterministic frame guard");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
 };
 std::string path=(fixture/"animation_demo.judasproj").string();char name[]="judas";char* args[]={name,path.data()};Application app;
 Check(app.Run(2,args,&control)==0&&checksDone>0,"normal Application animation startup/reload/shutdown");
 std::printf("SUMMARY %d checks %d failures frames=%d\n",checks,failures,frames);return failures?1:0;
}
