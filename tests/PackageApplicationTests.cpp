#include "Application.h"
#include "EngineHost.h"
#include "EnginePaths.h"
#include "GamePackage.h"
#include "InteractivePlay.h"
#include "ProjectExporter.h"
#include "RuntimeWorld.h"
#include "SceneSerialization.h"
#include "ScreenshotWriter.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
namespace fs=std::filesystem;
int Probe(const fs::path& output) {
    int checks=0,failures=0,frames=0;bool completed=false;
    auto check=[&](bool good,const char* label){++checks;failures+=!good;std::printf("%s %s\n",good?"PASS":"FAIL",label);};
    std::string error;GamePackage package;Project project;
    check(ReadGamePackage(EngineExecutableDir(),package,error)&&project.Load((fs::path(EngineExecutableDir())/package.projectFile).string(),error),"explicit executable-relative project");
    check(fs::current_path()=="/","unrelated working directory");
    const auto font=ResolveEngineDataPath("assets/fonts/DejaVuSans.ttf");
    check(font.find(EngineExecutableDir()+"/engine/")==0&&fs::exists(font),"packaged font ignores poisoned development override");
    ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.f/60;};
    control.hostReady=[&](EngineHost& host){
      check(!host.Resources().BlockingMode(),"ordinary non-blocking resources");
      check(host.GetWindow().Input().Map().Serialize()==project.Settings().input.Serialize(),"normal application installs packaged input map");
    };
    control.worldReady=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay&){
      check(world.VisualEmitters().size()==3&&world.PresentationCameras().size()==1,"ordinary particles and render-target camera constructed");
      check(host.Assets().Find("36363636363636363636363636363636")!=nullptr,"prefab registered from package");
      SceneTransform placement;placement.position={0,2,-3};EntityId spawned=0;
      spawned=world.SpawnPrefab("36363636363636363636363636363636",placement,error);
      check(spawned!=0,"normal runtime SpawnPrefab from packaged stable ID");
    };
    control.afterFrame=[&](EngineHost& host,RuntimeWorld& world,InteractivePlay& play){
      ++frames;const auto stats=host.Resources().Stats();
      if(!completed&&play.FixedStepsSinceReset()>=60&&stats.loadedMeshes>=1&&stats.loadedTextures>=2&&stats.loadedAudio>=3){
        check(stats.failed==0,"actual mesh texture and audio decoders all succeed");
        check(host.Audio().VoiceCount()>0,"packaged audio creates runtime voices (not listening evidence)");
        check(world.PresentationCameras()[0].updates>0&&host.GetRenderer().Stats().particlesSubmitted>0,"normal rendering camera/particle work");
        std::vector<unsigned char> pixels;host.GetRenderer().CaptureFrame(host.GetWindow().Width(),host.GetWindow().Height(),pixels);
        check(WriteRgbPng((output/"moved-package.png").string(),host.GetWindow().Width(),host.GetWindow().Height(),pixels),"rendered package screenshot");
        completed=true;SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
      }
      if(frames==1000){check(false,"startup/progress watchdog");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
    };
    unsetenv("JUDAS_TEST_SCRIPT");unsetenv("JUDAS_RESOURCE_MODE");setenv("JUDAS_WORLD_STATE","none",1);
    char name[]="judas";char* args[]={name};Application application;
    check(application.Run(1,args,&control)==0&&completed,"moved-package normal startup and shutdown");
    std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
int main(int argc,char** argv){
    if(argc==3&&std::string(argv[1])=="--probe")return Probe(argv[2]);
    const fs::path output=fs::absolute(argc==3&&std::string(argv[1])=="--output"?argv[2]:"build/m38-application");fs::create_directories(output);
    const fs::path temp=fs::temp_directory_path()/("judas-m38-app-"+std::to_string(getpid()));fs::create_directories(temp);
    Project project;ProjectExportOptions options;ProjectExportResult result;std::string error;
    options.destination=(temp/"original").string();
    if(!project.Load("projects/export_demo/export_demo.judasproj",error)||!ExportProject(project,options,result,error)){
      std::fprintf(stderr,"%s\n",error.c_str());fs::remove_all(temp);return 1;
    }
    fs::rename(temp/"original",temp/"moved package");
    const auto probe=temp/"moved package/.m38-probe";
    fs::copy_file(fs::path(EngineExecutableDir())/"judas_package_application_tests",probe);
    const pid_t pid=fork();
    if(pid==0){if(chdir("/")!=0)_exit(126);setenv("JUDAS_ENGINE_ROOT","/does/not/exist",1);execl(probe.c_str(),probe.c_str(),"--probe",output.c_str(),nullptr);_exit(127);}
    int status=0;if(pid>0)while(waitpid(pid,&status,0)<0&&errno==EINTR){}
    fs::remove_all(temp);return pid>0&&WIFEXITED(status)?WEXITSTATUS(status):1;
}
