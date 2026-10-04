// Real measured outer loop; no synthetic frame clock or replaced simulation.
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "ScreenshotWriter.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <filesystem>
int main(int argc,char** argv){
 std::string scene=argc>1?argv[1]:"projects/liquid_surface_demo/Scenes/lab.judas";unsigned frame=0,catchup=0,capped=0,failed=0;std::vector<double> frames,fixed,render,surface,liquid,containers,geometry,loading;
 using Clock=std::chrono::steady_clock;Clock::time_point last;
 ApplicationControl control;control.hidden=true;
 control.beforeFrame=[&](EngineHost&,RuntimeWorld& world,InteractivePlay& play){play.SetFixedStepMeasurementFlags(false,false,true);play.SetFixedStepObserver([&](const FixedStepMeasurements&,double ms){if(frame>=15){fixed.push_back(ms);auto t=world.Liquids().StepTimes();liquid.push_back(t.total*1000);containers.push_back(t.containers*1000);geometry.push_back(t.geometry*1000);loading.push_back(t.loading*1000);}});};
 control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){auto now=Clock::now();if(frame>=15){frames.push_back(std::chrono::duration<double,std::milli>(now-last).count());render.push_back(play.LastSceneMilliseconds());surface.push_back(play.LastSurfaceMilliseconds());catchup+=play.LastFixedStepsThisFrame()>1;capped+=play.LastFixedStepsThisFrame()>=8;}
  // Optional normal-loop capture includes the project JS camera and runtime UI.
  // The historical fixed-only harness does not execute those presentation hooks.
  if(frame==60&&argc>2){int width=0,height=0;SDL_GL_GetDrawableSize(host.GetWindow().NativeWindow(),&width,&height);std::vector<unsigned char> rgb;host.GetRenderer().CaptureFrame(width,height,rgb);std::filesystem::path path(argv[2]);if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());failed+=!WriteRgbPng(path.string(),width,height,rgb);}
  last=now;if(++frame==75){failed+=!world.Scripts()||!world.Scripts()->Diagnostics().empty()||!world.Liquids().Errors().empty();SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
 };
 char name[]="judas";char* args[]={name,scene.data()};Application app;int result=app.Run(2,args,&control);
 auto print=[&](const char* label,std::vector<double>& values){if(values.empty()){++failed;return;}std::sort(values.begin(),values.end());std::printf("M55 loop %s samples %zu median %.5f p95 %.5f max %.5f ms\n",label,values.size(),values[values.size()/2],values[size_t((values.size()-1)*.95)],values.back());};
 print("frame",frames);print("fixed",fixed);print("render",render);print("surface",surface);print("liquid",liquid);print("containers",containers);print("solid storage",geometry);print("loading",loading);std::printf("M55 loop catchup %u capped %u frames %u failures %u\n",catchup,capped,frame,failed);return result||failed?1:0;
}
