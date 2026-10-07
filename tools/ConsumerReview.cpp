// Post-M65 consumer review driver. This observes the ordinary Application loop
// and queues physical input. It does not replace a game controller or timestep.
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "WorldStreaming.h"
#include "PerformanceProfiler.h"
#include "ScreenshotWriter.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <cmath>

int main(int argc,char** argv) {
    if(argc!=5){std::cerr<<"usage: judas_consumer_review project-or-scene events.txt output-directory frames\n";return 2;}
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    const unsigned limit=unsigned(std::stoul(argv[4]));const unsigned captureEvery=std::getenv("JUDAS_REVIEW_CAPTURE_EVERY")?unsigned(std::stoul(std::getenv("JUDAS_REVIEW_CAPTURE_EVERY"))):120;unsigned frame=0,errors=0;double audioPeak=0;
    struct Event{std::string control;float value;};std::multimap<unsigned,Event> events;
    std::ifstream commands(argv[2]);unsigned at;std::string code;float value;
    while(commands>>at>>code>>value)events.emplace(at,Event{code,value});
    std::ofstream samples(output/"observations.jsonl");
    ApplicationControl control;control.hidden=true;
    control.hostReady=[](EngineHost& h){h.GetWindow().SetTestInputMode(true);SDL_GL_SetSwapInterval(0);std::string e;h.Audio().Init(e,true);};
    control.frameSeconds=[](float measured){return std::getenv("JUDAS_REVIEW_REAL_TIME")?measured:1.f/60;};
    control.worldReady=[](EngineHost&,RuntimeWorld& w,InteractivePlay&){if(std::getenv("JUDAS_REVIEW_NO_SLEEP"))w.Physics().SetSleepingEnabled(false);};
    control.beforeFrame=[&](EngineHost& h,RuntimeWorld&,InteractivePlay&){++frame;auto range=events.equal_range(frame);for(auto i=range.first;i!=range.second;++i)h.GetWindow().QueueTestPhysical(i->second.control,i->second.value);};
    control.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& play){
        float mix[1600]{};if(h.Audio().ReadWithoutDevice(mix,800))for(float sample:mix)audioPeak=std::max(audioPeak,double(std::abs(sample)));
        if(w.Scripts())for(const auto& d:w.Scripts()->Diagnostics()){
            ++errors;std::cerr<<"SCRIPT "<<d.callback<<": "<<d.message<<"\n";w.UI().RequestQuit();return;
        }
        if(frame%60==0||frame==1||frame==limit){
            unsigned asleep=0,dynamic=0;for(auto b:w.Physics().AliveBodies())if(w.Physics().IsDynamicBody(b)){++dynamic;asleep+=w.Physics().IsSleeping(b);}
            samples<<"{\"frame\":"<<frame<<",\"scene\":"<<std::quoted(w.SceneControl()->Current())<<",\"paused\":"<<(play.IsPaused()?"true":"false")<<",\"bodies\":"<<w.Physics().AliveBodyCount()<<",\"dynamic\":"<<dynamic<<",\"sleeping\":"<<asleep<<",\"audioVoices\":"<<h.Audio().VoiceCount()<<",\"mixedPeak\":"<<audioPeak<<",\"contacts\":"<<w.Physics().LastStepContactCount()<<",\"states\":[";
            bool first=true;if(w.Scripts())for(const auto& s:w.Scripts()->Capture()){if(!first)samples<<',';first=false;samples<<"{\"entity\":"<<std::quoted(std::to_string(s.entity))<<",\"state\":"<<(s.json.empty()?"null":s.json)<<'}';}samples<<"]";
            if(auto* stream=w.SceneControl()->StreamingIfLoaded()){
                samples<<",\"regions\":[";first=true;for(const auto& s:stream->Regions()){if(!first)samples<<',';first=false;samples<<"{\"id\":"<<std::quoted(s.id)<<",\"state\":"<<std::quoted(s.state)<<",\"entities\":"<<s.entities<<",\"integrationMs\":"<<s.integrationMs<<",\"largestUnitMs\":"<<s.largestUnitMs<<'}';}samples<<']';
            }
            samples<<",\"objects\":[";first=true;for(const auto& o:w.ScriptObjects())if(o.characterMotor||o.animation||(o.id==10&&o.body)||(o.id==2&&o.body)){
                if(!first)samples<<',';
                first=false;const auto* d=w.RuntimeDefinition(o.id);const auto p=w.PresentedTransform(o.id,d->transform,1).position;
                samples<<"{\"id\":"<<std::quoted(std::to_string(o.id))<<",\"p\":["<<p.x<<','<<p.y<<','<<p.z<<"]";
                if(auto* motor=w.RuntimeCharacter(o.id)){samples<<",\"supported\":"<<(motor->result.supported?"true":"false")<<",\"v\":["<<motor->velocity.x<<','<<motor->velocity.y<<','<<motor->velocity.z<<"],\"normal\":["<<motor->result.supportNormal.x<<','<<motor->result.supportNormal.y<<','<<motor->result.supportNormal.z<<"]";}
                if(o.animation){SceneTransform foot;if(w.JointPose(o.id,"mixamorig:LeftFoot","model",1,foot))samples<<",\"leftFoot\":["<<foot.position.x<<','<<foot.position.y<<','<<foot.position.z<<"]";if(w.JointPose(o.id,"mixamorig:RightFoot","model",1,foot))samples<<",\"rightFoot\":["<<foot.position.x<<','<<foot.position.y<<','<<foot.position.z<<"]";}
                samples<<'}';
            }samples<<"]}\n";samples.flush();
        }
        // Captures include paused presentation and the actual script-owned view.
        if(frame%captureEvery==0||frame==limit){std::vector<unsigned char> pixels;const auto name=std::to_string(frame);h.GetRenderer().CaptureFrame(h.GetWindow().Width(),h.GetWindow().Height(),pixels);WriteRgbPng((output/(name+".png")).string(),h.GetWindow().Width(),h.GetWindow().Height(),pixels);std::string e;PerformanceProfiler::Get().Export((output/(name+"-profile.json")).string(),e);}
        if(frame>=limit)w.UI().RequestQuit();
        // Let normal worker publications progress in accelerated scenarios.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    };
    control.beforeShutdown=[&](EngineHost&,RuntimeWorld&,InteractivePlay&){std::string e;if(!PerformanceProfiler::Get().Export((output/"profile.json").string(),e)){std::cerr<<e<<'\n';++errors;}};
    setenv("JUDAS_PROFILE","1",1);const int result=Application{}.Run(2,argv,&control);
    std::cout<<"Consumer observation frames="<<frame<<" scriptErrors="<<errors<<"\n";return result?result:errors?1:0;
}
