#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "SceneSerialization.h"
#include "ScreenshotWriter.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
int main(int argc,char**argv){
 int checks=0,failures=0,frames=0;bool observed=false;std::filesystem::path out="build/m37-application";
 if(argc==3&&std::string(argv[1])=="--output")out=argv[2];
 std::filesystem::create_directories(out);
 auto check=[&](bool b,const char* label){++checks;failures+=!b;std::printf("%s %s\n",b?"PASS":"FAIL",label);};
 unsetenv("JUDAS_TEST_SCRIPT");setenv("JUDAS_WORLD_STATE","none",1);
 Scene authored;std::string error;check(LoadSceneFromFile("assets/scenes/particle_demo.judas",authored,error),"authored demo loads");
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[&](EngineHost& h){check(!h.Resources().BlockingMode(),"actual async application path");};
 control.worldReady=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){check(w.VisualEmitters().size()==3,"normal runtime constructs three emitters");check(w.PresentationCameras().size()==1,"normal runtime secondary camera");};
 control.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& p){++frames;
  if(p.FixedStepsSinceReset()>=120&&!observed){
   check(w.VisualEmitters()[0].pool.Particles().size()>0,"loop emitter updates through ordinary fixed steps");
   check(w.VisualEmitters()[1].pool.Particles().size()>0,"gravity emitter updates through ordinary fixed steps");
   check(w.VisualEmitters()[2].pool.Particles().size()==100,"startup burst persists at expected lifetime");
   check(w.PresentationCameras()[0].updates>0,"actual offscreen camera passes update");
   check(h.GetRenderer().Stats().particlesSubmitted>0&&h.GetRenderer().Stats().particleEmittersConsidered>0,"normal scene pass submits particles");
   Scene disk;check(LoadSceneFromFile("assets/scenes/particle_demo.judas",disk,error)&&ScenesEqual(authored,disk),"Play does not rewrite emitter configuration");
   std::vector<unsigned char> pixels;const int width=h.GetWindow().Width(),height=h.GetWindow().Height();h.GetRenderer().CaptureFrame(width,height,pixels);
   WriteRgbPng((out/"demo.png").string(),width,height,pixels);
   observed=true;SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
  }
  if(frames==1000){check(false,"progress watchdog");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
 };
 char program[]="judas",scene[]="assets/scenes/particle_demo.judas";char* args[]={program,scene};Application app;
 check(app.Run(2,args,&control)==0&&observed,"normal demo startup/render/fixed-step/shutdown");
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
