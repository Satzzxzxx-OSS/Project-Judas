#include "PerformanceProfiler.h"
#include "ScriptSystem.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include "InputSystem.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
int main(){
    unsigned checks=0,failures=0;auto check=[&](bool v,const char* label){++checks;failures+=!v;std::printf("%s %s\n",v?"PASS":"FAIL",label);};
    auto& profiler=PerformanceProfiler::Get();std::string previous;
    for(bool enabled:{false,true}){
        profiler.Enable(enabled);profiler.Clear();
        auto root=std::filesystem::absolute("build/m56-js-scopes");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"Assets");
        std::ofstream(root/"Assets/scopes.js")<<R"JS(import {profiler} from 'judas';
export default class {
 constructor(){this.state={calls:0,value:0,caught:false,invalid:false};}
 start(){this.state.value=profiler.scope('Return',()=>profiler.scope('Nested',()=>{this.state.calls++;return 42;}));try{profiler.scope('Throw',()=>{this.state.calls++;throw Error('original');});}catch(e){this.state.caught=e.message==='original';}try{profiler.counter('Not finite',Infinity);}catch(e){this.state.invalid=true;}for(let i=0;i<200;i++)profiler.scope('Bounded '+i,()=>{});}
 fixedUpdate(){profiler.counter('Ticks',1);this.state.calls++;}
})JS";
        AssetDatabase assets;assets.Scan(root.string(),(root/"Assets").string());AssetRecord script;std::string error;check(assets.Track((root/"Assets/scopes.js").string(),script,error),"registered custom scope script");
        ResourceManager resources(nullptr,&assets);Scene scene;auto& entity=scene.CreateObject("Scope fixture");entity.scripts.push_back({1,script.id,true,"{}"});RuntimeWorld world;
        check(world.Build(scene,&resources,error),"headless normal script world");
        if(!world.IsBuilt()){std::puts(error.c_str());return 1;}
        {ProfileFrame frame("JS fixture");InputSystem input;world.UpdateScripts(&input,.02f);world.FixedScripts(&input,.02f);}
        check(world.Scripts()->Diagnostics().empty(),"nested/throw/counter behaviour stays safe");auto state=world.Scripts()->Capture()[0].json;
        check(state.find("\"value\":42")!=std::string::npos&&state.find("\"calls\":3")!=std::string::npos&&state.find("\"caught\":true")!=std::string::npos&&state.find("\"invalid\":true")!=std::string::npos,"callback executes once and preserves return/exception");
        if(enabled)check(previous==state,"disabled and enabled custom callbacks have identical game state");else previous=state;
        world.Destroy();check(profiler.Diagnostics().openScopes==0,"script/entity destruction leaves no open scopes");
    }
    check(profiler.Diagnostics().exhaustedLabels>0,"unique JS markers exhaust bounded cache visibly");
    // Interrupt is QuickJS's existing runaway budget; the profiling wrapper must
    // preserve that exception and close its RAII token during unwinding.
    auto root=std::filesystem::absolute("build/m56-js-interrupt");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"Assets");
    std::ofstream(root/"Assets/interrupt.js")<<"import {profiler} from 'judas';export default class {start(){profiler.scope('Runaway',()=>{while(true){}});}}";
    AssetDatabase assets;assets.Scan(root.string(),(root/"Assets").string());AssetRecord script;std::string error;assets.Track((root/"Assets/interrupt.js").string(),script,error);ResourceManager resources(nullptr,&assets);Scene scene;auto& entity=scene.CreateObject("Interrupt");entity.scripts.push_back({1,script.id,true,"{}"});RuntimeWorld world;world.Build(scene,&resources,error);world.UpdateScripts(nullptr,0); // default bounded interrupt
    check(!world.Scripts()->Diagnostics().empty(),"runaway interruption is preserved");world.Destroy();check(profiler.Diagnostics().openScopes==0,"interrupt and destroy cannot unbalance recording");
    std::printf("M56 JS %u checks %u failures\n",checks,failures);return failures?1:0;
}
