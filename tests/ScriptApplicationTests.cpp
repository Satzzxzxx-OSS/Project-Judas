#include "Application.h"
#include "EngineHost.h"
#include "EnginePaths.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "ProjectExporter.h"
#include "WorldState.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
namespace fs=std::filesystem;
int checks=0,failures=0;
void Check(bool good,const char* label){++checks;failures+=!good;std::printf("%s %s\n",good?"PASS":"FAIL",label);}
void Key(SDL_Scancode key,bool down){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=key;e.key.keysym.sym=SDL_GetKeyFromScancode(key);e.key.repeat=0;SDL_PushEvent(&e);}
int Run(const std::string& projectFile,const fs::path& save,bool restored){
    int frames=0;ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
    control.hostReady=[](EngineHost& host){std::string error;Check(host.Audio().Init(error,true),"real miniaudio engine in device-free mode; no listening claim");Check(!host.Resources().BlockingMode(),"scripted game uses actual asynchronous application resources");};
    control.worldReady=[&](EngineHost&,RuntimeWorld& world,InteractivePlay&){if(restored){std::string error;WorldState state;
        Check(LoadWorldStateFromFile(save.string(),state,error)&&ApplyWorldState(world,state,error),"normal world save loads before first script callback");}};
    control.beforeFrame=[&](EngineHost&,RuntimeWorld&,InteractivePlay&){
        if(!restored){if(frames==0)Key(SDL_SCANCODE_K,true);if(frames==1){Key(SDL_SCANCODE_K,false);Key(SDL_SCANCODE_G,true);}
            if(frames==2){Key(SDL_SCANCODE_G,false);Key(SDL_SCANCODE_P,true);}if(frames==3)Key(SDL_SCANCODE_P,false);
            if(frames==4)Key(SDL_SCANCODE_P,true);
            if(frames==5){Key(SDL_SCANCODE_P,false);Key(SDL_SCANCODE_J,true);}if(frames==6)Key(SDL_SCANCODE_J,false);}
    };
    control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){++frames;
        if(frames==10){Check(world.Scripts()&&world.Scripts()->Diagnostics().empty(),"JS gameplay executes inside real rendered application frames");
            const auto states=world.Scripts()->Capture();for(const auto& s:states)std::printf("APP_STATE %llu/%llu %s\n",(unsigned long long)s.entity,(unsigned long long)s.slot,s.json.c_str());
            bool collected=false,open=false,spawned=false,pulsed=false;
            for(const auto& s:states){if(s.entity==4)collected=s.json.find("\"collected\":true")!=std::string::npos;
                if(s.entity==5)open=s.json.find("\"open\":true")!=std::string::npos;
                if(s.entity==6)spawned=s.json.find("\"spawned\":2")!=std::string::npos;
                if(s.entity>=kRuntimeEntityIdBase)pulsed|=s.json.find("\"pulses\":1")!=std::string::npos;}
            Check(collected&&open&&spawned&&pulsed,"JS key/locked-door/prefab/effects game rules and controlled state");
            Check(play.FixedStepsSinceReset()==10,"one authoritative fixed callback cadence per actual fixed step");
            bool requested=false;for(const auto& e:world.AudioEmitters())requested|=e.wantPlay;
            Check((restored||requested)&&host.Resources().Stats().failed==0,"scripted authored audio routes through normal resources");
            std::string error;if(!restored)Check(SaveWorldStateToFile(CaptureWorldState(world),save.string(),error),"ordinary save captures authored and runtime-spawned JS state");
            SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
        }
        if(frames>12){Check(false,"application terminates deterministically");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
    };
    char program[]="judas";std::string mutableProject=projectFile;char* args[]={program,mutableProject.empty()?nullptr:mutableProject.data()};
    Application app;Check(app.Run(mutableProject.empty()?1:2,args,&control)==0,"ordinary runtime shutdown");return failures?1:0;
}
int main(int argc,char** argv){
    fs::path output="docs/evidence/m40/focused";
    for(int i=1;i+1<argc;++i)if(std::string(argv[i])=="--output")output=argv[++i];
    output=fs::absolute(output);fs::create_directories(output);
    if(argc>1&&std::string(argv[1])=="--probe"){fs::remove(output/"package.judasstate");Check(fs::current_path()=="/","moved package runs from unrelated working directory");return Run("",output/"package.judasstate",false);}
    fs::path working="build/m40-application";fs::remove_all(working);fs::copy("projects/script_demo",working,fs::copy_options::recursive);
    auto projectFile=fs::absolute(working/"script_demo.judasproj").string();auto save=output/"gameplay.judasstate";fs::remove(save);
    Run(projectFile,save,false);Run(projectFile,save,true);
    Project project;std::string error;Check(project.Load(projectFile,error),"script project loads through normal project configuration");
    auto package=fs::absolute("build/m40-script-export");fs::remove_all(package);ProjectExportOptions options;options.destination=package.string();options.runtimeExecutable=fs::absolute("build/judas").string();ProjectExportResult result;
    Check(ExportProject(project,options,result,error),"normal M38 export packages registered JS and imported modules");
    if(failures){std::printf("ERROR %s\n",error.c_str());return 1;}
    auto moved=fs::absolute("build/m40-moved-game");fs::remove_all(moved);fs::rename(package,moved);
    auto probe=moved/"script_probe";fs::copy_file(fs::path(EngineExecutableDir())/"judas_script_application_tests",probe);
    auto pid=fork();if(pid==0){if(chdir("/")!=0)_exit(126);setenv("JUDAS_ENGINE_ROOT","/missing/development/path",1);execl(probe.c_str(),probe.c_str(),"--probe","--output",output.c_str(),nullptr);_exit(127);}
    int status=0;while(waitpid(pid,&status,0)<0&&errno==EINTR){}
    Check(pid>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"moved exported application runs JS gameplay without repository/Node");
    std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
