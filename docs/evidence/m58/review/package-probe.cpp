// Narrow actual-Application probe copied beside the unchanged exported executable.
// The package marker and executable-directory resolution are ordinary runtime paths.
#include "Application.h"
#include "EngineHost.h"
#include "EnginePaths.h"
#include "GamePackage.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "InteractivePlay.h"
#include "PerformanceProfiler.h"
#include "ScreenshotWriter.h"
#include <filesystem>
#include <cstdio>
#include <cstdlib>
namespace fs=std::filesystem;
int main(int argc,char** argv){if(argc!=2)return 2;fs::path out=fs::absolute(argv[1]);fs::create_directories(out);int checks=0,failures=0,frame=0,published=-1;std::string error;auto check=[&](bool ok,const char* why){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",why);};GamePackage package;Project project;check(ReadGamePackage(EngineExecutableDir(),package,error)&&project.Load((fs::path(EngineExecutableDir())/package.projectFile).string(),error),"unchanged exported project resolves beside executable");check(fs::current_path()=="/tmp","unrelated cwd");check(ResolveEngineDataPath("assets/fonts/DejaVuSans.ttf").find(EngineExecutableDir()+"/engine/")==0,"package engine font ignores poisoned development root");ApplicationControl c;c.hidden=true;c.frameSeconds=[](float){return 1.f/60;};bool range=project.Settings().localization.locales.size()==4;size_t pausedAt=0;std::string firstFingerprint;
 c.hostReady=[&](EngineHost& h){h.GetWindow().SetTestInputMode(true);h.Audio().Init(error,true);check(!h.Resources().BlockingMode(),"normal asynchronous resource path");std::printf("GL %s / %s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER));};
 c.worldReady=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){firstFingerprint=w.BaselineFingerprint();check(w.Localization().Locale()=="en","fresh package session authored default");};
 c.beforeFrame=[&](EngineHost&,RuntimeWorld& w,InteractivePlay& play){if(frame==18||frame==19||frame==34||frame==35){if(range){SDL_Event e{};e.type=(frame==18||frame==34)?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=SDL_SCANCODE_ESCAPE;e.key.keysym.sym=SDLK_ESCAPE;SDL_PushEvent(&e);}}if(frame==21){pausedAt=play.FixedStepsSinceReset();check(w.Localization().SetLocale("ja",error),"queued locale switch without WaitForAll");std::printf("SWITCH requested_frame=%d\n",frame);}if(frame==48)check(w.SceneControl()->Reload(error),"normal queued scene reload");};
 c.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& play){if(frame>=21&&published<0&&w.Localization().Locale()=="ja"){published=frame;std::printf("SWITCH published_frame=%d revision=%llu\n",frame,(unsigned long long)w.Localization().Revision());}if(frame==32&&range){check(w.UI().Paused()&&!w.pointerCapture,"paused language publication retains menu/pointer policy");check(play.FixedStepsSinceReset()==pausedAt,"language change does not advance paused simulation");}if(frame==46){check(published>=21&&w.Localization().Fonts().size()>0,"asynchronous locale/fonts published in normal runtime");check(!w.UI().Paused()&&(!range||w.pointerCapture),"continues same game after locale change");check(w.Scripts()->Diagnostics().empty(),"ordinary package JavaScript fault-free");}if(frame==62){check(w.Localization().Locale()=="ja"&&w.BaselineFingerprint()==firstFingerprint,"scene reload retains locale and content baseline");std::vector<unsigned char> pixels;h.GetRenderer().CaptureFrame(h.GetWindow().Width(),h.GetWindow().Height(),pixels);check(WriteRgbPng((out/"moved-readonly.png").string(),h.GetWindow().Width(),h.GetWindow().Height(),pixels),"actual packaged shaped text screenshot");}if(++frame==110)w.UI().RequestQuit();};
 c.beforeShutdown=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){check(w.Scripts()->Diagnostics().empty(),"reload/quit leaves no script faults");check(PerformanceProfiler::Get().Export((out/"profile.json").string(),error),"M56 actual async package frames exported");};char name[]="judas";char* runtimeArgs[]={name};Application app;check(app.Run(1,runtimeArgs,&c)==0&&frame==110,"moved read-only startup/locale/pause/reload/ordinary quit");std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;}
