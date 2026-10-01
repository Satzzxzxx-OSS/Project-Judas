// Uses the ordinary async application/render/fixed-step loop. No prefab-only
// simulation or renderer. Hidden GL is automated evidence, not human acceptance.
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "Prefab.h"
#include "SceneSerialization.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
int main(){
    int checks=0,failures=0,frames=0;bool observed=false;
    auto check=[&](bool b,const char* s){++checks;if(!b)++failures;std::printf("%s %s\n",b?"PASS":"FAIL",s);};
    unsetenv("JUDAS_TEST_SCRIPT");setenv("JUDAS_WORLD_STATE","none",1);
    Scene authored;std::string error;check(LoadSceneFromFile("assets/scenes/prefab_demo.judas",authored,error),"demo authored scene loads");
    ApplicationControl control;control.hidden=true;control.frameSeconds=[](float){return 1.0f/60;};
    control.hostReady=[&](EngineHost& h){check(!h.Resources().BlockingMode(),"demo actual async application");};
    control.worldReady=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay&){
        int roots=0;for(const auto& o:authored.Objects())if(!o.prefabAsset.empty())++roots;
        check(roots==2,"demo two linked authored instances");
        const auto before=w.Entities().size();SceneTransform placement;placement.position={0,1,3};
        auto spawned=w.SpawnPrefab("36363636363636363636363636363636",placement,error);
        check(spawned&&w.Entities().size()==before+1&&w.AdditionalEntities().size()==2,"runtime independent root and children created");
        check(w.DestroyHierarchy(spawned,error),"runtime hierarchy deletion succeeds");
        check(w.Physics().AliveBodyCount()>0&&h.Resources().GetRenderer()==&h.GetRenderer(),"ordinary physics and renderer remain live");
    };
    control.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& p){
        ++frames;
        if(p.FixedStepsSinceReset()>=30&&!observed){
            check(h.GetRenderer().Stats().drawCalls>0,"demo ordinary scene rendering submits draws");
            check(p.FixedStepsSinceReset()>=30,"demo ordinary simulation advances");
            for(const auto& o:authored.Objects())if(!o.prefabAsset.empty()){
                EntityPhysicalState state;check(w.GetEntityState(o.id,state),"authored prefab root independent runtime state");
            }
            Scene disk;std::string e;check(LoadSceneFromFile("assets/scenes/prefab_demo.judas",disk,e)&&ScenesEqual(disk,authored),"runtime does not rewrite authored prefab scene");
            observed=true;SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
        }
        if(frames==1000){check(false,"demo progress watchdog");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
    };
    char program[]="judas",scene[]="assets/scenes/prefab_demo.judas";char* args[]={program,scene};Application app;
    check(app.Run(2,args,&control)==0&&observed,"demo application startup and shutdown");
    std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;
}
