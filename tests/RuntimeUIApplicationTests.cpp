#include "Application.h"
#include "EngineHost.h"
#include "EnginePaths.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "ProjectExporter.h"
#include "ScreenshotWriter.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
namespace fs=std::filesystem;
int checks=0,failures=0;
void Check(bool ok,const char* label){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",label);}
void Key(SDL_Scancode key,bool down){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=key;e.key.keysym.sym=SDL_GetKeyFromScancode(key);SDL_PushEvent(&e);}
int Run(std::string project,const fs::path& out){int frame=0;size_t pausedSteps=0;ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[](EngineHost& host){std::string error;host.Audio().Init(error,true);Check(!host.Resources().BlockingMode(),"normal asynchronous Application resources");};
 control.beforeFrame=[&](EngineHost&,RuntimeWorld&,InteractivePlay&){
  if(frame==1)Key(SDL_SCANCODE_RETURN,true);
  if(frame==2)Key(SDL_SCANCODE_RETURN,false);
  if(frame==3)Key(SDL_SCANCODE_P,true);
  if(frame==4)Key(SDL_SCANCODE_P,false);
  if(frame==5)Key(SDL_SCANCODE_ESCAPE,true);
  if(frame==6){Key(SDL_SCANCODE_ESCAPE,false);Key(SDL_SCANCODE_DOWN,true);}
  if(frame==7){Key(SDL_SCANCODE_DOWN,false);Key(SDL_SCANCODE_RETURN,true);}
  if(frame==8){Key(SDL_SCANCODE_RETURN,false);Key(SDL_SCANCODE_RIGHT,true);}
  if(frame==9){Key(SDL_SCANCODE_RIGHT,false);Key(SDL_SCANCODE_DOWN,true);}
  if(frame==10){Key(SDL_SCANCODE_DOWN,false);Key(SDL_SCANCODE_RETURN,true);}
  if(frame==11){Key(SDL_SCANCODE_RETURN,false);Key(SDL_SCANCODE_ESCAPE,true);}
  if(frame==12)Key(SDL_SCANCODE_ESCAPE,false);
  if(frame==13)Key(SDL_SCANCODE_ESCAPE,true);
  if(frame==14)Key(SDL_SCANCODE_ESCAPE,false);
 };
 control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){auto h=world.UI().Find("game_ui");auto* hud=world.UI().Element(h,"hud");
  if(frame==0){Check(play.IsPaused()&&play.LastFixedStepsThisFrame()==0,"authored main menu owns pause before first physics step");std::vector<unsigned char> pixels;host.GetRenderer().CaptureFrame(host.GetWindow().Width(),host.GetWindow().Height(),pixels);WriteRgbPng((out/"main.png").string(),host.GetWindow().Width(),host.GetWindow().Height(),pixels);}
  if(frame==2)Check(!play.IsPaused()&&hud&&hud->visible&&play.LastFixedStepsThisFrame()==1,"JS Start releases cursor ownership and resumes ordinary simulation");
  if(frame==4)Check(world.UI().Element(h,"counter")->text=="Spawned props: 1","live JS HUD reflects ordinary prefab spawning");
  if(frame==5){Check(play.IsPaused()&&play.LastFixedStepsThisFrame()==0,"Escape opens authored pause without simulation step");pausedSteps=play.FixedStepsSinceReset();}
  if(frame==7)Check(world.UI().Element(h,"options")->visible,"keyboard activates authored nested Options");
  if(frame==8)Check(world.UI().Element(h,"strength")->value>.6f&&world.UI().Element(h,"level")->text.find('%')!=std::string::npos,"slider input invokes JS change event");
  if(frame==10)Check(!hud->visible,"toggle value-change controls HUD through JS");
  if(frame==11)Check(world.UI().Element(h,"pause")->visible&&play.FixedStepsSinceReset()==pausedSteps,"Back returns to pause and no fixed time was invented");
  if(frame==14)Check(!play.IsPaused()&&play.LastFixedStepsThisFrame()==1,"Back resumes ordinary fixed step");
  if(frame==15){Check(world.Scripts()&&world.Scripts()->Diagnostics().empty(),"UI script callbacks remain fault-free");auto stats=world.UI().Stats();std::printf("PERFORMANCE UI elements=%u update_us=%.3f render_us=%.3f draws=%u\n",stats.elements,stats.updateUs,stats.renderUs,stats.draws);SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}++frame;
  if(frame>18){Check(false,"bounded application shutdown");SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
 };
 char name[]="judas";char* args[]={name,project.empty()?nullptr:project.data()};Application app;Check(app.Run(project.empty()?1:2,args,&control)==0,"ordinary UI runtime shutdown");return failures?1:0;
}
int main(int argc,char** argv){fs::path out="build/m41-ui-application";for(int i=1;i+1<argc;++i)if(std::string(argv[i])=="--output")out=argv[++i];out=fs::absolute(out);fs::create_directories(out);
 if(argc>1&&std::string(argv[1])=="--probe"){Check(fs::current_path()=="/","moved package unrelated working directory");return Run("",out);}
 fs::path projectDir="build/m41-application-project";fs::remove_all(projectDir);fs::copy("projects/ui_demo",projectDir,fs::copy_options::recursive);auto file=fs::absolute(projectDir/"ui_demo.judasproj").string();Run(file,out);
 Project project;std::string error;Check(project.Load(file,error),"normal UI project settings load");ProjectExportOptions options;options.destination=fs::absolute("build/m41-ui-export").string();options.runtimeExecutable=fs::absolute("build/judas").string();fs::remove_all(options.destination);ProjectExportResult result;Check(ExportProject(project,options,result,error),"M38 packages UI documents, scripts, fonts and images");if(failures){std::printf("ERROR %s\n",error.c_str());return 1;}
 auto moved=fs::absolute("build/m41-moved-game");fs::remove_all(moved);fs::rename(options.destination,moved);Check(fs::exists(moved/"Assets/ui/menu.judasui")&&fs::exists(moved/"Assets/fonts/DejaVuSans.ttf"),"UI document transitive dependencies packaged");auto exe=moved/"ui_probe";fs::copy_file(fs::path(EngineExecutableDir())/"judas_runtime_ui_application_tests",exe);
 auto pid=fork();if(pid==0){if(chdir("/"))_exit(126);setenv("JUDAS_ENGINE_ROOT","/missing/development/path",1);execl(exe.c_str(),exe.c_str(),"--probe","--output",out.c_str(),nullptr);_exit(127);}int status=0;while(waitpid(pid,&status,0)<0&&errno==EINTR){}Check(pid>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"moved exported game executes real UI/script interaction");std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;}
