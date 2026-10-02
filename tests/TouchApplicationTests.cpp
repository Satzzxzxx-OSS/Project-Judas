#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "ScreenshotWriter.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <cstdio>
#include <regex>
int checks=0,failures=0;
void Check(bool ok,const char* name){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",name);}
int main(int argc,char** argv){namespace fs=std::filesystem;fs::path out="build/m42-application";if(argc==3&&std::string(argv[1])=="--output")out=argv[2];fs::create_directories(out);
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};int frame=0;
 control.hostReady=[](EngineHost& host){std::string error;host.Audio().Init(error,true);Check(!host.Resources().BlockingMode(),"normal asynchronous Application resource path");};
 control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){
  if(frame==0)Check(!play.IsPaused()&&world.AdditionalEntities().size()>0,"JS Start spawns ordinary prefab and owns authored HUD");
  if(frame==210){auto h=world.UI().Find("game_ui");auto* counter=world.UI().Element(h,"counter");std::string text=counter?counter->text:"";std::printf("HUD %s\n",text.c_str());
   Check(std::regex_search(text,std::regex("Collision [1-9][0-9]*/[1-9]")),"real fixed-step collision callbacks reach JS HUD");
   Check(std::regex_search(text,std::regex("Trigger [1-9][0-9]*/[1-9][0-9]*/[1-9]")),"real fixed-step trigger enter/stay/exit reaches HUD");
      bool excluded=false,spawned=false;for(auto& state:world.Scripts()->Capture()){std::printf("STATE %llu %s\n",(unsigned long long)state.entity,state.json.c_str());if(state.entity==7)excluded=state.json=="{\"events\":0}";if(state.entity>=9&&state.json.find("\"events\":0")==std::string::npos)spawned=true;}
   Check(excluded&&spawned,"excluded layer stays silent; spawned prefab participates");Check(world.Scripts()->Diagnostics().empty(),"demo callbacks have no script faults");
   std::vector<unsigned char> pixels;host.GetRenderer().CaptureFrame(host.GetWindow().Width(),host.GetWindow().Height(),pixels);WriteRgbPng((out/"demo.png").string(),host.GetWindow().Width(),host.GetWindow().Height(),pixels);
   SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);
  }++frame;
 };
 std::string project="projects/touch_demo/touch_demo.judasproj";char name[]="judas";char* args[]={name,project.data()};Application app;Check(app.Run(2,args,&control)==0,"ordinary standalone shutdown");std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
