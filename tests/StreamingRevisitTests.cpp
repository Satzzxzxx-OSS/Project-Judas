// The real project's logical movement walks to the end and back, repeatedly.
// No position teleports, fabricated request timers or scene-name runtime branches.
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "WorldStreaming.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <algorithm>
int main(int argc,char** argv){
 const std::string project=argc>1?argv[1]:"projects/streamed_range/streamed_range.judasproj";
 const std::filesystem::path out=argc>2?argv[2]:"build/m59-revisit";
 std::filesystem::create_directories(out);std::ofstream csv(out/"travel.csv");csv<<"frame,z,leg,active,pending,retained,integration_ms,unloading_regions\n";
 unsigned frames=0,leg=0,checks=0,failures=0,stalled=0;float previous=8;bool stopped=false;size_t peakRetained=0;double peakWork=0;std::vector<std::string> states;
 auto check=[&](bool ok,const char* label){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",label);};
 ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/15;};
 control.hostReady=[](EngineHost& h){SDL_GL_SetSwapInterval(0);h.GetWindow().SetTestInputMode(true);std::string e;h.Audio().Init(e,true);};
 control.beforeFrame=[&](EngineHost& h,RuntimeWorld&,InteractivePlay&){auto& input=h.GetWindow().Input();input.SetPhysical("key:W",!stopped&&leg%2==0);input.SetPhysical("key:S",!stopped&&leg%2==1);input.SetPhysical("key:F10",frames==50);};
 control.afterFrame=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){++frames;std::string error;auto* stream=w.SceneControl()->Streaming(w,error);if(!stream){check(false,"streaming service");w.UI().RequestQuit();return;}auto z=w.RuntimeDefinition(10)->transform.position.z;auto stats=stream->Stats();size_t unloading=0;states.clear();for(auto& r:stream->Regions()){if(r.state=="unloading")++unloading;states.push_back(r.id+":"+r.state+":"+r.error);}
  peakRetained=std::max(peakRetained,stats.retainedBytes);peakWork=std::max(peakWork,stats.integrationMs);csv<<frames<<','<<z<<','<<leg<<','<<stats.active<<','<<stats.pending<<','<<stats.retainedBytes<<','<<stats.integrationMs<<','<<unloading<<'\n';
  if(!stopped&&((leg%2==0&&z<=-146)||(leg%2==1&&z>=7))){check(true,leg%2==0?"actual JS motor reaches final gallery":"actual JS motor returns through unloaded galleries to bootstrap");++leg;stalled=0;std::printf("LEG %u frame=%u retained=%zu integration=%.6f\n",leg,frames,stats.retainedBytes,stats.integrationMs);if(leg==4)stopped=true;}
  if(!stopped&&frames>80){stalled=std::abs(z-previous)<.001f?stalled+1:0;if(stalled>180){check(false,"backtracking continues without permanent residency deadlock");for(auto& s:states)std::printf("REGION %s\n",s.c_str());stopped=true;}}
  previous=z;
  if(stopped||frames>2600){check(leg==4,"two complete forward/backward traversals");check(w.Scripts()&&w.Scripts()->Diagnostics().empty(),"target and player callbacks remain valid across suspension/revisit");for(auto& d:w.Scripts()->Diagnostics())std::printf("FAULT entity=%llu %s\n",(unsigned long long)d.entity,d.message.c_str());std::printf("M59_REVISIT frames=%u leg=%u peak_retained=%zu peak_integration_ms=%.6f\n",frames,leg,peakRetained,peakWork);w.UI().RequestQuit();}
 };
 Application app;char name[]="judas";char* args[]={name,const_cast<char*>(project.c_str())};check(app.Run(2,args,&control)==0,"ordinary application shutdown");std::printf("SUMMARY %u checks %u failures\n",checks,failures);return failures?1:0;
}
