#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
int checks=0,failures=0;
void Check(bool ok,const char* text){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",text);}
int main(int argc,char** argv){
 namespace fs=std::filesystem;
 fs::path out=argc==3?argv[2]:"build/m44-application";fs::create_directories(out);
 fs::path fixture=out/"project";fs::remove_all(fixture);fs::copy("projects/query_demo",fixture,fs::copy_options::recursive);
 // Assertions are project JS in a disposable fixture. All calls use the real
 // VM/runtime/physics bridge and the normal Application loop.
 fs::rename(fixture/"Assets/scripts/queries.js",fixture/"Assets/scripts/demo.js");
 fs::copy_file(fixture/"Assets/scripts/queries.js.judasmeta",fixture/"Assets/scripts/demo.js.judasmeta");
 std::ofstream(fixture/"Assets/scripts/demo.js.judasmeta")<<"JudasAssetMeta 1\nid \"44444444444444444444444444444403\"\ntype script\nsource \"\"\n";
 std::ofstream(fixture/"Assets/scripts/queries.js")<<R"JS(
import Demo from './demo.js';import {world,physics} from 'judas';
export default class extends Demo {
 start(){super.start();const assert=(x,s)=>{if(!x)throw Error(s);};
 const v=(x,y,z)=>({x,y,z}),filter={excludeLayers:['Excluded'],excludedTags:['IgnoreQuery'],ignored:[world.entity('5')]};
 const before=JSON.stringify(world.entity('4').transform);
 let h=physics.raycast(v(-2,1.5,8),v(0,0,-2),100,filter);
 assert(h&&h.entity.id==='4'&&h.entity.valid&&Math.abs(h.distance-8.2)<.0001&&h.normal.z===1&&Math.abs(h.point.z+.2)<.0001,'ray data');
 assert(!physics.raycast(v(20,1.5,8),v(0,0,-1),100,filter),'miss');
 assert(!physics.raycast(v(2,1.5,8),v(0,0,-1),10,filter),'classification');
 assert(!physics.raycast(v(0,1.5,8),v(0,0,-1),8,filter),'ignored entity');
 assert(Math.abs(physics.sphereCast(v(-2,1.5,8),.4,v(0,0,-1),100,filter).distance-7.8)<.0001,'sphere');
 const pose={position:v(-2,1.5,8),rotation:{w:1,x:0,y:0,z:0}};
 assert(Math.abs(physics.capsuleCast(pose,.2,.4,v(0,0,-1),100,filter).distance-8)<.0001,'capsule');
 assert(Math.abs(physics.boxCast(pose,v(.3,.3,.3),v(0,0,-1),100,filter).distance-7.9)<.0001,'box');
 h=physics.raycast(v(-1,1.5,8),v(0,0,-1),100,filter);
 assert(h&&h.entity.id===this.spawned[0].id,'runtime prefab hit');
 const retained=h.entity;retained.destroy();assert(!retained.valid,'safe destroyed hit wrapper');
 assert(JSON.stringify(world.entity('4').transform)===before,'read only');
 this.spawn();this.state.checked=true;
 }
})JS";
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[](EngineHost& host){std::string error;host.Audio().Init(error,true);};
 int frames=0;
 control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){
  if(++frames==3){
   Check(world.Scripts()&&world.Scripts()->Diagnostics().empty(),"actual JS cast callbacks have no faults");
   if(world.Scripts()){for(auto& fault:world.Scripts()->Diagnostics())std::printf("FAULT %s\n",fault.message.c_str());
    auto states=world.Scripts()->Capture();Check(!states.empty()&&states[0].json.find("\"checked\":true")!=std::string::npos,"JS checks ray/shape hits, filtering, prefab, read-only and safe destroyed wrapper");}
   auto doc=world.UI().Find("game_ui");Check(doc&&world.UI().Element(doc,"counter")->text.find("Ray:")!=std::string::npos,"normal active-camera snapshot drives authored query HUD");
   Check(host.GetWindow().Input().Map().Find("spawn_prefab")!=nullptr,"project spawn binding loaded");
   SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.scancode=SDL_SCANCODE_P;SDL_PushEvent(&event);
  }else if(frames==4){
   auto states=world.Scripts()->Capture();Check(states[0].json.find("\"spawns\":3")!=std::string::npos,"physical P activates demo prefab spawn");
   SDL_Event event{};event.type=SDL_KEYUP;event.key.keysym.scancode=SDL_SCANCODE_P;SDL_PushEvent(&event);
   event.type=SDL_KEYDOWN;event.key.keysym.scancode=SDL_SCANCODE_ESCAPE;SDL_PushEvent(&event);
  }else if(frames==5){
   auto doc=world.UI().Find("game_ui");Check(play.IsPaused()&&world.UI().Element(doc,"pause")->visible,"physical Escape displays authored pause panel");
   SDL_Event event{};event.type=SDL_KEYUP;event.key.keysym.scancode=SDL_SCANCODE_ESCAPE;SDL_PushEvent(&event);
  }else if(frames==6){
   SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.scancode=SDL_SCANCODE_ESCAPE;SDL_PushEvent(&event);
  }else if(frames==7){
   auto doc=world.UI().Find("game_ui");Check(!play.IsPaused()&&!world.UI().Element(doc,"pause")->visible,"physical Escape resumes demo");
   SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);
  }
 };
 std::string project=(fixture/"query_demo.judasproj").string();char name[]="judas";char* args[]={name,project.data()};Application app;
 Check(app.Run(2,args,&control)==0&&frames>=7,"normal Application startup, rendering and shutdown");
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
