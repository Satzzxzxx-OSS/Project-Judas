#include "Application.h"
#include "EngineHost.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "WorldStreaming.h"
#include "InteractivePlay.h"
#include "PerformanceProfiler.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
namespace {using Clock=std::chrono::steady_clock;long rss(){std::ifstream f("/proc/self/status");std::string l;while(std::getline(f,l))if(l.rfind("VmRSS:",0)==0){std::istringstream s(l.substr(6));long kb;s>>kb;return kb;}return 0;}void summary(const char* mode,const char* metric,std::vector<double> v){std::sort(v.begin(),v.end());std::printf("M59_PERF %s %s median=%.6f p95=%.6f max=%.6f ms\n",mode,metric,v[v.size()/2],v[size_t((v.size()-1)*.95)],v.back());}}
int main(int argc,char** argv){std::filesystem::path out=argc>1?argv[1]:"build/m59-perf";std::filesystem::create_directories(out);setenv("JUDAS_PROFILE","1",1);
 for(const char* mode:{"original","all-resident","streamed","streamed-real-time"}){bool real=std::string(mode)=="streamed-real-time";bool original=std::string(mode)=="original",all=std::string(mode)=="all-resident";std::string project=original?"projects/shooter_game/shooter_game.judasproj":"projects/streamed_range/streamed_range.judasproj";unsigned frame=0;std::vector<double> intervals,integration,fixed;Clock::time_point previous;WorldStreaming* regions=nullptr;std::string error;std::vector<uint64_t> manual;
 std::ofstream csv(out/(std::string(mode)+".csv"));csv<<"frame,wall_ms,fixed_ms,steps,integration_ms,bodies,active,pending,live_estimate,retained_estimate,resource_bytes,rss_kb\n";
 ApplicationControl control;control.hidden=true;if(!real)control.frameSeconds=[](float){return 1.f/60;};
 control.hostReady=[](EngineHost& host){SDL_GL_SetSwapInterval(0);host.GetWindow().SetTestInputMode(true);std::string e;host.Audio().Init(e,true);};
 control.worldReady=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){if(!original){regions=w.SceneControl()->Streaming(w,error);if(all)for(int i=0;i<6;++i)manual.push_back(regions->Request("gallery-"+std::to_string(i),false,error));}previous=Clock::now();};
 control.beforeFrame=[&](EngineHost& host,RuntimeWorld&,InteractivePlay& play){play.SetFixedStepMeasurementFlags(false,false,true);auto& input=host.GetWindow().Input();input.SetPhysical("key:W",frame<260||(frame>=420&&frame<700));input.SetPhysical("key:S",(frame>=260&&frame<420)||frame>=700);};
 control.afterFrame=[&](EngineHost& host,RuntimeWorld& w,InteractivePlay& play){auto now=Clock::now();double elapsed=std::chrono::duration<double,std::milli>(now-previous).count();previous=now;StreamingStats s;if(regions)s=regions->Stats();if(frame>30){intervals.push_back(elapsed);integration.push_back(s.integrationMs);fixed.push_back(play.LastFixedStepMilliseconds());}csv<<frame<<','<<elapsed<<','<<play.LastFixedStepMilliseconds()<<','<<play.LastFixedStepsThisFrame()<<','<<s.integrationMs<<','<<w.Physics().AliveBodyCount()<<','<<s.active<<','<<s.pending<<','<<s.liveBytes<<','<<s.retainedBytes<<','<<host.Resources().Stats().bytesResident<<','<<rss()<<'\n';
 if(frame==400){JUDAS_PROFILE_SCOPE("Harness capture export");PerformanceProfiler::Get().Export((out/(std::string(mode)+"-cold-profile.json")).string(),error);previous=Clock::now();}
 if(++frame>=(real?460u:920u)){if(regions)for(auto& r:regions->Regions())std::printf("REGION %s %s state=%s preparation=%.6f load=%.6f integration=%.6f largest_unit=%.6f bytes=%zu retained=%zu\n",mode,r.id.c_str(),r.state.c_str(),r.preparationMs,r.loadMs,r.integrationMs,r.largestUnitMs,r.bytes,r.retained);for(auto& d:w.Scripts()->Diagnostics())std::printf("FAULT %s\n",d.message.c_str());PerformanceProfiler::Get().Export((out/(std::string(mode)+"-profile.json")).string(),error);w.UI().RequestQuit();}
 };
 Application app;char program[]="judas";char* args[]={program,project.data()};if(app.Run(2,args,&control)!=0)return 1;summary(mode,"frame interval",intervals);summary(mode,"integration",integration);summary(mode,"fixed",fixed);PerformanceProfiler::Get().Clear();
 }
 return 0;
}
