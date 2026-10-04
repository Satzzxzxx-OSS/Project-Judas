#include "PerformanceProfiler.h"
#include <array>
#include "ScriptSystem.h"
#include "AssetDatabase.h"
#include "InputSystem.h"
#include "SceneFingerprint.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "ResourceManager.h"
#include "ProductionFluidCoupling.h"
#include "quickjs.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
std::string String(JSContext* c,JSValueConst v){const char* p=JS_ToCString(c,v);if(!p)return {};std::string s(p);JS_FreeCString(c,p);return s;}
std::string Exception(JSContext* c){auto e=JS_GetException(c);auto stack=JS_GetPropertyStr(c,e,"stack");auto s=String(c,e)+"\n"+String(c,stack);JS_FreeValue(c,stack);JS_FreeValue(c,e);return s;}
JSValue Vec(JSContext* c,glm::vec3 v){auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"x",JS_NewFloat64(c,v.x));JS_SetPropertyStr(c,o,"y",JS_NewFloat64(c,v.y));JS_SetPropertyStr(c,o,"z",JS_NewFloat64(c,v.z));return o;}
bool Number(JSContext* c,JSValueConst o,const char* key,float& value){auto v=JS_GetPropertyStr(c,o,key);double n=0;bool ok=JS_ToFloat64(c,&n,v)==0&&std::isfinite(n)&&std::abs(n)<=1e20;JS_FreeValue(c,v);value=static_cast<float>(n);return ok;}
bool ReadVec(JSContext* c,JSValueConst o,glm::vec3& v){return Number(c,o,"x",v.x)&&Number(c,o,"y",v.y)&&Number(c,o,"z",v.z);}
const char* library=R"JS(
const call=(op,...args)=>globalThis.__judas(op,...args);
export class Entity {
 constructor(id){this.id=String(id)}
 get valid(){return call('valid',this.id)}
 get transform(){return call('transform',this.id)}
 get presentedTransform(){return call('presentedTransform',this.id)}
 set transform(value){call('setTransform',this.id,value)}
 get parent(){return entity(call('parent',this.id))}
 get children(){return call('children',this.id).map(entity)}
 setColliderEnabled(enabled){return call('colliderEnabled',this.id,enabled)}
 destroy(){return call('destroy',this.id)}
 hasTag(tag){return call('hasTag',this.id,tag)}
 addTag(tag){return call('addTag',this.id,tag)}
 removeTag(tag){return call('removeTag',this.id,tag)}
 get classification(){return call('classification',this.id)}
 get animation(){return call('animationExists',this.id)?new Animation(this.id):null}
 get liquid(){const h=call("liquidOwner",this.id);return h?new LiquidVolume(h):null}
 get navigationObstacle(){return call("navObstacleInfo",this.id)}
 get navigationLink(){return call("navLinkInfo",this.id)}
 setNavigationEnabled(component,enabled){return call("navEnabled",this.id,component,enabled)}
 get navigation(){return call("navAgentExists",this.id)?new NavigationAgent(this.id):null}
 get character(){return call("characterExists",this.id)?new Character(this.id):null}
 get ragdoll(){return call('ragdollExists',this.id)?new Ragdoll(this.id):null}
 get audio(){return call('audioInfo',this.id)}
 setAudioEnabled(enabled){return call('audioEnabled',this.id,enabled)}
 get camera(){return call('cameraInfo',this.id)}
 material(slot=0){return new Material(this.id,slot)}
 scriptState(slot){return call('scriptState',this.id,slot)}
 applyForce(value){call('force',this.id,value)}
 applyImpulse(value){call('impulse',this.id,value)}
 applyImpulseAtPoint(impulse,point){call('impulseAtPoint',this.id,impulse,point)}
 applyTorque(value){call('torque',this.id,value)}
 get mass(){return call('mass',this.id)}
 get inertiaWorld(){return call('inertiaWorld',this.id)}
 get velocity(){return call('velocity',this.id)}
 set velocity(value){call('setVelocity',this.id,value)}
 get angularVelocity(){return call('angularVelocity',this.id)}
 set angularVelocity(value){call('setAngularVelocity',this.id,value)}
 playAudio(){return call('playAudio',this.id)}
 stopAudio(){return call('stopAudio',this.id)}
 pauseAudio(){return call('pauseAudio',this.id)}
 resumeAudio(){return call('resumeAudio',this.id)}
 burst(count){return call('burst',this.id,count)}
 setParticles(settings){return call('particles',this.id,settings)}
 setCameraEnabled(enabled){return call('camera',this.id,enabled)}
}
export const entity=id=>id&&id!=='0'?new Entity(id):null;
export const world={entity,get appearance(){return call('appearanceInfo')},setAppearance:settings=>call('appearanceSet',settings),setView:(pose,fov=70)=>call("setView",pose,fov),clearView:()=>call("clearView"),fluidSample:(point,up,halfHeight,radius,tangent)=>call("fluidSample",point,up,halfHeight,radius,tangent),get viewRay(){return call('viewRay')},queryTags:(required=[],excluded=[])=>call('queryTags',required,excluded).map(entity),
 spawnPrefab:(asset,transform)=>entity(call('spawn',asset,transform)),
 overlap:(min,max,filter={})=>call('overlap',min,max,filter).map(entity),
 sweepCapsule:(from,displacement,rotation={w:1,x:0,y:0,z:0},filter={})=>call('sweep',from,displacement,filter,rotation)};
const cast=(origin,direction,maximum,filter,shape)=>{const hit=call('cast',origin,direction,filter,{...shape,maximum});return hit?{...hit,entity:entity(hit.entityId)}:null};
export class Material {
 constructor(entityId,slot=0){this.entityId=entityId;this.slot=slot}
 get state(){return call('materialInfo',this.entityId,this.slot)}
 assign(asset){return call('materialAssign',this.entityId,this.slot,asset)}
 set(parameters){return call('materialOverride',this.entityId,this.slot,parameters)}
 clearOverrides(){return call('materialClear',this.entityId,this.slot)}
}
export class Character {
 constructor(id){this.id=id}
 get state(){const v=call('characterState',this.id);return {...v,supportEntity:entity(v.supportEntityId)}}
 get velocity(){return this.state.velocity}
 set velocity(v){call('characterVelocity',this.id,v)}
 get supported(){return this.state.supported}
 get supportNormal(){return this.state.supportNormal}
 get supportVelocity(){return this.state.supportVelocity}
 get actualDisplacement(){return this.state.actualDisplacement}
 get gravity(){return this.state.gravity}
 get up(){return this.state.up}
 set enabled(v){call('characterEnabled',this.id,v)}
 configure(v){return call('characterConfigure',this.id,v)}
 accelerate(v){return call('characterAcceleration',this.id,v)}
 ignore(entities){return call('characterIgnore',this.id,entities.map(e=>e.id))}
}
export class LiquidVolume {
 constructor(handle){this.handle=handle}
 get valid(){return call('liquidValid',this.handle)}
 get state(){const v=call('liquidState',this.handle);return {...v,entity:entity(v.entityId)}}
 set enabled(value){call('liquidEnabled',this.handle,value)}
 transferTo(destination,volume,options={}){return call('liquidTransfer',this.handle,destination.handle,volume,options)}
 applyImpulse(point,impulse){return call('liquidImpulse',this.handle,point,impulse)}
 set surfaceEnabled(value){call('liquidSurfaceEnabled',this.handle,value)}
}
export const liquid={
 sample:point=>{const v=call('liquidSample',point);return v?{...v,entity:entity(v.entityId)}:null},
 samplePresented:point=>{const v=call('liquidPresentedSample',point);return v?{...v,entity:entity(v.entityId)}:null},
 accounting:(material='water')=>call('liquidAccounting',material),
 get errors(){return call('liquidErrors')},
 get connections(){return call('liquidConnections')},
 submerged:target=>call('liquidSubmerged',target.id)
};
export class NavigationAgent {
 constructor(id){this.id=id}
 get state(){const s=call('navAgentState',this.id);return {...s,link:entity(s.linkId)}}
 set enabled(value){call("navEnabled",this.id,"agent",value)}
 setDestination(point){return call('navDestination',this.id,point)}
 clear(){return call('navClear',this.id)}
 set stopped(value){call('navStopped',this.id,value)}
 get stopped(){return this.state.stopped}
 get steering(){return this.state.steering}
 get remainingDistance(){return this.state.remainingDistance}
 completeLink(){return call('navCompleteLink',this.id)}
 configure(settings){return call('navConfigure',this.id,settings)}
}
const navResult=r=>r?{...r,surface:entity(r.surfaceId)}:null;
const navPath=p=>({...p,corners:p.corners.map(c=>({...c,link:entity(c.linkId)}))});
export const navigation={
 sample:(point,range=2,filter={})=>navResult(call('navSample',point,range,filter)),
 path:(start,end,filter={})=>navPath(call('navPath',start,end,filter)),
 raycast:(start,end,filter={})=>navResult(call('navRaycast',start,end,filter)),
 get areas(){return call('navAreas')},get profiles(){return call('navProfiles')},get errors(){return call('navErrors')}
};
export class Ragdoll {
 constructor(id){this.id=id}
 get active(){return call('ragdollActive',this.id)}
 enter(){return call('ragdollEnter',this.id)}
 leave(seconds=.4){return call('ragdollLeave',this.id,seconds)}
 set enabled(value){call('ragdollEnabled',this.id,value)}
 body(joint){return entity(call('ragdollBody',this.id,joint))}
}
export class Animation {
 constructor(entityId){this.entityId=entityId}
 get info(){return call('animationInfo',this.entityId)}
 get clips(){return this.info.clips}
 get playing(){return this.info.playing}
 get time(){return this.info.time}
 get speed(){return this.info.speed}
 set speed(speed){call('animationSet',this.entityId,{speed})}
 get loop(){return this.info.loop}
 set loop(loop){call('animationSet',this.entityId,{loop})}
 play(clip=''){return call('animationPlay',this.entityId,clip)}
 pause(){return call('animationPause',this.entityId)}
 resume(){return this.play()}
 stop(){return call('animationStop',this.entityId)}
 seek(time){return call('animationSeek',this.entityId,time)}
 crossFade(clip,seconds=.3){return call('animationFade',this.entityId,clip,seconds)}
 get layers(){return this.info.layers}
 layer(id,settings){return call('animationLayer',this.entityId,id,settings)}
 removeLayer(id){return call('animationRemoveLayer',this.entityId,id)}
}
export class Joint {
 constructor(id){this.id=String(id)}
 get valid(){return call('jointValid',this.id)}
 get state(){return call('jointState',this.id)}
 setEnabled(enabled){return call('jointSet',this.id,{enabled})}
 setLimits(lower,upper,limits=true){return call('jointSet',this.id,{lower,upper,limits})}
 setMotor(speed,maxForce,motor=true){return call('jointSet',this.id,{speed,maxForce,motor})}
 setSpring(rest,stiffness,damping,spring=true){return call('jointSet',this.id,{rest,stiffness,damping,spring})}
}
export const physics={
 joint:owner=>{const id=call('joint',owner.id);return id?new Joint(id):null},
 raycast:(origin,direction,maximum,filter={})=>cast(origin,direction,maximum,filter,{kind:'ray'}),
 sphereCast:(origin,radius,direction,maximum,filter={})=>cast(origin,direction,maximum,filter,{kind:'sphere',radius}),
 capsuleCast:(pose,radius,halfHeight,direction,maximum,filter={})=>cast(pose.position,direction,maximum,filter,{kind:'capsule',rotation:pose.rotation,radius,halfHeight}),
 boxCast:(pose,halfExtents,direction,maximum,filter={})=>cast(pose.position,direction,maximum,filter,{kind:'box',rotation:pose.rotation,halfExtents})};
export const scenes={get current(){return call('sceneCurrent')},get registered(){return call('sceneList')},load:name=>call('sceneLoad',name),reload:()=>call('sceneReload')};
export const session={get:key=>call('sessionGet',key),set:(key,value)=>call('sessionSet',key,value),delete:key=>call('sessionDelete',key)};
export const input={get pointerCapture(){return call('pointerCapture')},set pointerCapture(value){call('setPointerCapture',value)},held:name=>call('held',name),pressed:name=>call('pressed',name),released:name=>call('released',name),axis:name=>call('axis',name)};
export const profiler={scope:(label,callback)=>call("profileScope",label,callback),counter:(label,value,mode="sum")=>call("profileCounter",label,value,mode)};
export const time={get elapsed(){return call('elapsed')},get delta(){return call('delta')},get fixed(){return call('fixed')}};
export const console={log:(...args)=>call('log',args.map(String).join(' '))};

export class UIElement {
 constructor(handle,id){this.handle=handle;this.id=id}
 get text(){return call('uiGet',this.handle,this.id,'text')}
 set text(v){call('uiSet',this.handle,this.id,'text',v)}
 get visible(){return call('uiGet',this.handle,this.id,'visible')}
 set visible(v){call('uiSet',this.handle,this.id,'visible',v)}
 get enabled(){return call('uiGet',this.handle,this.id,'enabled')}
 set enabled(v){call('uiSet',this.handle,this.id,'enabled',v)}
 get value(){return call('uiGet',this.handle,this.id,'value')}
 set value(v){call('uiSet',this.handle,this.id,'value',v)}
 get texture(){return call('uiGet',this.handle,this.id,'texture')}
 set texture(v){call('uiSet',this.handle,this.id,'texture',v)}
}
export class UIDocument {
 constructor(handle){this.handle=handle}
 get(id){call('uiGet',this.handle,id,'visible');return new UIElement(this.handle,id)}
 get visible(){return call('uiGet',this.handle,'','visible')}
 set visible(v){call('uiSet',this.handle,'','visible',v)}
 get enabled(){return call('uiGet',this.handle,'','enabled')}
 set enabled(v){call('uiSet',this.handle,'','enabled',v)}
 get modal(){return call('uiGet',this.handle,'','modal')}
 set modal(v){call('uiSet',this.handle,'','modal',v)}
 show(){this.visible=true} hide(){this.visible=false}
 unload(){call('uiUnload',this.handle)}
}
export const ui={get:name=>{const id=call('uiFind',name);return id?new UIDocument(id):null},
 load:(asset,name)=>new UIDocument(call('uiLoad',asset,name)),quit:()=>call('uiQuit'),
 get debugOverlayVisible(){return call('uiDiagnostics')},set debugOverlayVisible(v){call('uiDiagnostics',v)}};

globalThis.console=console;
)JS";
}
struct ScriptSystem::Impl {
    std::thread::id thread=std::this_thread::get_id();
    void CheckThread() const {if(thread!=std::this_thread::get_id())throw std::logic_error("ScriptSystem is main/runtime-thread only");}
    RuntimeWorld* world;const AssetDatabase* assets;JSRuntime* rt=nullptr;JSContext* ctx=nullptr;
    struct Instance {SceneObjectId entity;SceneScriptSlot slot;JSValue value=JS_UNDEFINED;bool started=false,fault=false;size_t order=0;std::uint32_t profileLabel=0;bool profileRegistered=false;};
    std::map<std::pair<SceneObjectId,std::uint64_t>,Instance> instances;
    std::map<std::string,JSModuleDef*> modules;
    std::vector<ScriptDiagnostic> diagnostics;
    const InputSystem* input=nullptr;bool fixed=false;float delta=0;
    bool inPresentation=false;float presentationAlpha=1;
    bool hasView=false;glm::vec3 viewOrigin{0},viewDirection{0};
    unsigned budget=10000,polls=0;bool stopping=false;SceneObjectId currentOwner=0;std::uint64_t currentSlot=0;
    std::map<std::pair<SceneObjectId,std::uint64_t>,std::string> restored;
    Impl(RuntimeWorld* w,const AssetDatabase* a):world(w),assets(a){
        rt=JS_NewRuntime();JS_SetMemoryLimit(rt,64*1024*1024);JS_SetMaxStackSize(rt,512*1024);
        ctx=JS_NewContext(rt);JS_SetContextOpaque(ctx,this);
        JS_SetInterruptHandler(rt,[](JSRuntime*,void* p)->int{auto* s=static_cast<Impl*>(p);return ++s->polls>s->budget;},this);
        JS_SetModuleLoaderFunc(rt,Normalize,Load,this);
        auto global=JS_GetGlobalObject(ctx);JS_SetPropertyStr(ctx,global,"__judas",JS_NewCFunction(ctx,Native,"__judas",1));JS_FreeValue(ctx,global);
        auto lib=NamespaceLibrary();JS_FreeValue(ctx,lib);
    }
    struct CachedLabel {JSAtom atom=JS_ATOM_NULL;std::uint32_t id=0;};
    std::array<CachedLabel,128> profileLabels{};unsigned profileLabelCount=0;
    std::uint32_t CustomLabel(JSValueConst value){
        auto& p=PerformanceProfiler::Get();if(!p.Active())return 0;
        JSAtom atom=JS_ValueToAtom(ctx,value);if(atom==JS_ATOM_NULL)return 0;
        for(unsigned i=0;i<profileLabelCount;++i)if(profileLabels[i].atom==atom){JS_FreeAtom(ctx,atom);return profileLabels[i].id;}
        if(profileLabelCount==profileLabels.size()){JS_FreeAtom(ctx,atom);return p.Intern("");}
        auto id=p.Intern(String(ctx,value));profileLabels[profileLabelCount++]={atom,id};return id;
    }
    std::uint32_t AssetLabel(Instance& i){
        auto& p=PerformanceProfiler::Get();if(!p.Active()||!p.ScriptAttribution())return 0;
        if(!i.profileRegistered){i.profileRegistered=true;auto* record=assets?assets->Find(i.slot.asset):nullptr;
            if(record)i.profileLabel=p.Intern("Script asset: "+record->relativePath);
        }return i.profileLabel;
    }
    ~Impl(){Stop();for(unsigned i=0;i<profileLabelCount;++i)JS_FreeAtom(ctx,profileLabels[i].atom);JS_FreeContext(ctx);JS_FreeRuntime(rt);}
    static char* Normalize(JSContext* c,const char* base,const char* name,void* p){
        auto* s=static_cast<Impl*>(p);std::string result;
        if(std::string(name)=="judas")result="judas";
        else if(std::string(name).rfind("./",0)==0||std::string(name).rfind("../",0)==0){result=(std::filesystem::path(base).parent_path()/name).lexically_normal().generic_string();}
        else {JS_ThrowReferenceError(c,"only relative registered project modules and judas are allowed");return nullptr;}
        if(result!="judas"&&(!s->assets||!s->assets->FindByRelativePath(result)||s->assets->FindByRelativePath(result)->type!=AssetType::Script)){
            JS_ThrowReferenceError(c,"unregistered script import: %s",result.c_str());return nullptr;}
        return js_strdup(c,result.c_str());
    }
    static JSModuleDef* Load(JSContext*,const char* name,void* p){return static_cast<Impl*>(p)->Module(name);}
    JSModuleDef* Module(const std::string& path){
        if(auto it=modules.find(path);it!=modules.end())return it->second;
        std::string source;
        if(path=="judas")source=library;
        else {const auto* record=assets?assets->FindByRelativePath(path):nullptr;
            if(!record||record->missing||record->type!=AssetType::Script){JS_ThrowReferenceError(ctx,"missing script %s",path.c_str());return nullptr;}
            std::ifstream file(record->path);std::ostringstream out;out<<file.rdbuf();source=out.str();
            if(!file||source.size()>1024*1024){JS_ThrowReferenceError(ctx,"unreadable or oversized script %s",path.c_str());return nullptr;}
        }
        auto value=JS_Eval(ctx,source.data(),source.size(),path.c_str(),JS_EVAL_TYPE_MODULE|JS_EVAL_FLAG_COMPILE_ONLY);
        if(JS_IsException(value))return nullptr;
        auto* module=static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(value));modules[path]=module;JS_FreeValue(ctx,value);return module;
    }
    void Error(Instance& i,const char* callback){i.fault=true;auto message=Exception(ctx);diagnostics.push_back({i.entity,i.slot.id,i.slot.asset,callback,message});std::fprintf(stderr,"script asset=%s entity=%llu slot=%llu %s: %s\n",i.slot.asset.c_str(),(unsigned long long)i.entity,(unsigned long long)i.slot.id,callback,message.c_str());}
    std::set<std::string> evaluated;
    bool Evaluate(JSModuleDef* module,const std::string& path){
        if(evaluated.count(path))return true;
        auto value=JS_MKPTR(JS_TAG_MODULE,module);
        if(JS_ResolveModule(ctx,value)<0)return false;
        auto result=JS_EvalFunction(ctx,JS_DupValue(ctx,value));
        if(JS_IsException(result))return false;
        auto state=JS_PromiseState(ctx,result);
        if(state==JS_PROMISE_REJECTED){auto reason=JS_PromiseResult(ctx,result);JS_FreeValue(ctx,result);JS_Throw(ctx,reason);return false;}
        if(state==JS_PROMISE_PENDING){JS_FreeValue(ctx,result);JS_ThrowTypeError(ctx,"asynchronous module initialization is unsupported");return false;}
        JS_FreeValue(ctx,result);evaluated.insert(path);return true;
    }
    JSValue NamespaceLibrary(){auto* module=Module("judas");if(!module)return JS_EXCEPTION;
        if(!Evaluate(module,"judas"))return JS_EXCEPTION;
        return JS_GetModuleNamespace(ctx,module);}
    JSValue Namespace(const std::string& asset){const auto* record=assets?assets->Find(asset):nullptr;
        if(!record||record->type!=AssetType::Script)return JS_ThrowReferenceError(ctx,"missing script asset %s",asset.c_str());
        auto* module=Module(record->relativePath);if(!module)return JS_EXCEPTION;
        if(!Evaluate(module,record->relativePath))return JS_EXCEPTION;
        return JS_GetModuleNamespace(ctx,module);
    }
    bool Json(JSValueConst value,std::string& text,std::string& error) const {
        // No coercion/toJSON: plain own data only, bounded and cycle-free.
        size_t nodes=0;std::set<void*> ancestors;
        std::function<bool(JSValueConst,unsigned)> check=[&](JSValueConst v,unsigned depth){
            if(depth>16||++nodes>4096)return false;
            if(JS_IsNull(v)||JS_IsBool(v)||JS_IsString(v))return true;
            if(JS_IsNumber(v)){double d;return JS_ToFloat64(ctx,&d,v)==0&&std::isfinite(d);}
            if(!JS_IsObject(v)||JS_IsFunction(ctx,v))return false;
            if(JS_IsArray(v)){auto length=JS_GetPropertyStr(ctx,v,"length");uint32_t n=0;JS_ToUint32(ctx,&n,length);JS_FreeValue(ctx,length);if(n>4096)return false;}
            void* ptr=JS_VALUE_GET_PTR(v);if(!ancestors.insert(ptr).second)return false;
            auto proto=JS_GetPrototype(ctx,v);auto global=JS_GetGlobalObject(ctx);auto obj=JS_GetPropertyStr(ctx,global,JS_IsArray(v)?"Array":"Object");auto expected=JS_GetPropertyStr(ctx,obj,"prototype");
            bool plain=JS_IsNull(proto)||JS_VALUE_GET_PTR(proto)==JS_VALUE_GET_PTR(expected);JS_FreeValue(ctx,proto);JS_FreeValue(ctx,expected);JS_FreeValue(ctx,obj);JS_FreeValue(ctx,global);
            JSPropertyEnum* props=nullptr;uint32_t count=0;
            bool ok=plain&&JS_GetOwnPropertyNames(ctx,&props,&count,v,JS_GPN_STRING_MASK|JS_GPN_SYMBOL_MASK)==0;
            for(uint32_t i=0;ok&&i<count;++i){auto key=JS_AtomToValue(ctx,props[i].atom);if(JS_IsSymbol(key))ok=false;JS_FreeValue(ctx,key);JSPropertyDescriptor desc{};int result=JS_GetOwnProperty(ctx,&desc,v,props[i].atom);ok=result==1&&JS_IsUndefined(desc.getter)&&JS_IsUndefined(desc.setter)&&check(desc.value,depth+1);JS_FreeValue(ctx,desc.value);JS_FreeValue(ctx,desc.getter);JS_FreeValue(ctx,desc.setter);}
            JS_FreePropertyEnum(ctx,props,count);ancestors.erase(ptr);return ok;
        };
        if(!check(value,0)){error="state must be bounded plain JSON data (no accessors/cycles/nonfinite values)";return false;}
        auto json=JS_JSONStringify(ctx,value,JS_UNDEFINED,JS_UNDEFINED);if(JS_IsException(json)){error=Exception(ctx);return false;}text=String(ctx,json);JS_FreeValue(ctx,json);
        if(text.size()>65536){error="state exceeds 64KiB";return false;}return true;
    }
    void Callback(Instance& i,const char* name,bool presentation=false){if(i.fault)return;
        JUDAS_PROFILE_SCOPE("Script lifecycle callback");
        ProfileScope assetScope(AssetLabel(i));
currentOwner=i.entity;currentSlot=i.slot.id;polls=0;auto fn=JS_GetPropertyStr(ctx,i.value,name);
        if(JS_IsException(fn)){Error(i,name);return;}if(JS_IsFunction(ctx,fn)){JUDAS_PROFILE_COUNTER("Script callback calls",1,ProfileCounterMode::Sum);JSValue args[]={JS_NewFloat64(ctx,delta),JS_NewFloat64(ctx,presentationAlpha)};auto result=JS_Call(ctx,fn,i.value,presentation?2:1,args);if(JS_IsException(result))Error(i,name);else if(JS_PromiseState(ctx,result)!=JS_PROMISE_NOT_A_PROMISE){JS_ThrowTypeError(ctx,"async gameplay callbacks are unsupported");Error(i,name);}JS_FreeValue(ctx,result);}JS_FreeValue(ctx,fn);
    }
    void Stop(){if(stopping)return;stopping=true;std::vector<Instance*> order;
        for(auto& entry:instances)order.push_back(&entry.second);
        std::sort(order.begin(),order.end(),[](auto* a,auto* b){return a->entity!=b->entity?a->entity<b->entity:a->order<b->order;});
        for(auto* i:order){Callback(*i,"destroy");JS_FreeValue(ctx,i->value);}instances.clear();}
    static JSValue Native(JSContext* c,JSValueConst,int argc,JSValueConst* argv);
};
JSValue ScriptSystem::Impl::Native(JSContext* c,JSValueConst,int argc,JSValueConst* argv){
    auto* s=static_cast<Impl*>(JS_GetContextOpaque(c));
    if(argc<1)return JS_ThrowTypeError(c,"missing operation");
    auto op=String(c,argv[0]);
    auto arg=[&](int i){return i<argc?argv[i]:JS_UNDEFINED;};
    if(op=="log"){std::fprintf(stdout,"JS: %s\n",String(c,arg(1)).c_str());return JS_UNDEFINED;}
    if(op=="profileScope"){
        if(!JS_IsString(arg(1))||!JS_IsFunction(c,arg(2)))return JS_ThrowTypeError(c,"profiler.scope requires a string and callback");
        ProfileScope scope(s->CustomLabel(arg(1)));
        return JS_Call(c,arg(2),JS_UNDEFINED,0,nullptr); // exactly once; exception and interrupt pass through
    }
    if(op=="profileCounter"){
        double value;if(!JS_IsString(arg(1))||JS_ToFloat64(c,&value,arg(2))!=0||!std::isfinite(value))return JS_ThrowTypeError(c,"profiler.counter requires a string and finite value");
        auto mode=String(c,arg(3));if(mode!="sum"&&mode!="latest"&&mode!="max")return JS_ThrowTypeError(c,"counter mode must be sum, latest or max");
        PerformanceProfiler::Get().Counter(s->CustomLabel(arg(1)),value,mode=="sum"?ProfileCounterMode::Sum:mode=="max"?ProfileCounterMode::Maximum:ProfileCounterMode::Latest);
        return JS_UNDEFINED;
    }
    if(!s->world)return JS_ThrowTypeError(c,"world API unavailable in metadata context");
    auto& world=*s->world;


    if(op=="appearanceInfo"||op=="appearanceSet"){
        auto appearance=world.Settings();auto options=arg(1);
        auto has=[&](const char* name){auto v=JS_GetPropertyStr(c,options,name);bool found=!JS_IsUndefined(v);JS_FreeValue(c,v);return found;};
        if(op=="appearanceSet"){
            for(auto pair:{std::pair<const char*,float*>{"exposure",&appearance.exposure},{"environmentIntensity",&appearance.environmentIntensity}})if(has(pair.first)&&!Number(c,options,pair.first,*pair.second))return JS_ThrowTypeError(c,"finite appearance parameter required");
            for(auto pair:{std::pair<const char*,bool*>{"linearRendering",&appearance.linearRendering},{"environmentBackground",&appearance.environmentBackground}})if(has(pair.first)){auto v=JS_GetPropertyStr(c,options,pair.first);if(!JS_IsBool(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"boolean appearance setting required");}*pair.second=JS_ToBool(c,v);JS_FreeValue(c,v);}
            if(has("environmentAsset")){auto v=JS_GetPropertyStr(c,options,"environmentAsset");if(!JS_IsString(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"environment asset ID required");}appearance.environmentAsset=String(c,v);JS_FreeValue(c,v);}
            if(has("environmentRotation")){auto v=JS_GetPropertyStr(c,options,"environmentRotation");bool ok=Number(c,v,"w",appearance.environmentRotation.w)&&Number(c,v,"x",appearance.environmentRotation.x)&&Number(c,v,"y",appearance.environmentRotation.y)&&Number(c,v,"z",appearance.environmentRotation.z);JS_FreeValue(c,v);if(!ok)return JS_ThrowTypeError(c,"finite environment quaternion required");}
            if(!world.SetAppearance(appearance))return JS_ThrowTypeError(c,"invalid appearance/environment asset");
            return JS_TRUE;
        }
        auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"linearRendering",JS_NewBool(c,appearance.linearRendering));JS_SetPropertyStr(c,result,"exposure",JS_NewFloat64(c,appearance.exposure));JS_SetPropertyStr(c,result,"environmentAsset",JS_NewString(c,appearance.environmentAsset.c_str()));JS_SetPropertyStr(c,result,"environmentIntensity",JS_NewFloat64(c,appearance.environmentIntensity));JS_SetPropertyStr(c,result,"environmentBackground",JS_NewBool(c,appearance.environmentBackground));auto q=Vec(c,{appearance.environmentRotation.x,appearance.environmentRotation.y,appearance.environmentRotation.z});JS_SetPropertyStr(c,q,"w",JS_NewFloat64(c,appearance.environmentRotation.w));JS_SetPropertyStr(c,result,"environmentRotation",q);return result;
    }
    auto liquidHandle=[&](JSValueConst value){LiquidHandle h;std::istringstream text(String(c,value));char separator=0;if(!(text>>h.generation>>separator>>h.id)||separator!=':')return LiquidHandle{};text>>std::ws;return text.eof()?h:LiquidHandle{};};
    if(op=="liquidValid")return JS_NewBool(c,world.Liquids().Get(liquidHandle(arg(1)))!=nullptr);
    if(op=="liquidState"||op=="liquidEnabled"||op=="liquidTransfer"||op=="liquidImpulse"||op=="liquidSurfaceEnabled"){
      auto h=liquidHandle(arg(1));auto* state=world.Liquids().Get(h);if(!state)return JS_ThrowReferenceError(c,"stale liquid handle");
      if(op=="liquidEnabled"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");return JS_NewBool(c,world.Liquids().SetEnabled(h,JS_ToBool(c,arg(2))));}
      if(op=="liquidImpulse"){
        if(!s->fixed)return JS_ThrowTypeError(c,"surface impulse belongs in fixedUpdate");
        glm::vec3 p,v;
        if(!ReadVec(c,arg(2),p)||!ReadVec(c,arg(3),v))return JS_ThrowTypeError(c,"finite point and impulse required");
        return JS_NewBool(c,world.Liquids().SurfaceImpulse(h,p,v));
      }
      if(op=="liquidSurfaceEnabled"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");return JS_NewBool(c,world.Liquids().SurfaceEnabled(h,JS_ToBool(c,arg(2))));}
      if(op=="liquidTransfer"){if(!s->fixed)return JS_ThrowTypeError(c,"liquid transfer belongs in fixedUpdate");auto other=liquidHandle(arg(2));if(!world.Liquids().Get(other))return JS_ThrowReferenceError(c,"stale destination");double volume;if(JS_ToFloat64(c,&volume,arg(3))||!std::isfinite(volume)||volume<0)return JS_ThrowTypeError(c,"finite nonnegative volume required");std::optional<glm::vec3> sourcePoint,destinationPoint;auto options=arg(4);
        for(auto entry:{std::pair<const char*,std::optional<glm::vec3>*>{"sourcePoint",&sourcePoint},{"destinationPoint",&destinationPoint}}){
          auto v=JS_GetPropertyStr(c,options,entry.first);if(!JS_IsUndefined(v)){glm::vec3 p;if(!ReadVec(c,v,p)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"finite exchange point required");}*entry.second=p;}JS_FreeValue(c,v);
        }
        return JS_NewFloat64(c,world.Liquids().Transfer(h,other,volume,sourcePoint,destinationPoint));}
      auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"entityId",JS_NewString(c,std::to_string(state->entity).c_str()));JS_SetPropertyStr(c,v,"enabled",JS_NewBool(c,state->enabled));JS_SetPropertyStr(c,v,"container",JS_NewBool(c,state->container));JS_SetPropertyStr(c,v,"equilibriumValid",JS_NewBool(c,state->equilibriumValid));JS_SetPropertyStr(c,v,"material",JS_NewString(c,state->material.id.c_str()));JS_SetPropertyStr(c,v,"density",JS_NewFloat64(c,state->material.density));JS_SetPropertyStr(c,v,"volume",JS_NewFloat64(c,state->volume));JS_SetPropertyStr(c,v,"capacity",JS_NewFloat64(c,state->capacity));JS_SetPropertyStr(c,v,"stableCapacity",JS_NewFloat64(c,state->stableCapacity));JS_SetPropertyStr(c,v,"coordinate",JS_NewFloat64(c,state->q));
      auto surface=state->dynamicSurface;JSValue info=JS_NULL;
      if(surface){info=JS_NewObject(c);JS_SetPropertyStr(c,info,"enabled",JS_NewBool(c,surface->enabled));
        for(auto field:{std::pair<const char*,double>{"cells",double(surface->cells.size())},{"faces",double(surface->discharge.size())},{"volume",surface->Volume()},{"iterations",double(surface->stats.iterations)},{"retries",double(surface->stats.retries)},{"limitedFaces",double(surface->stats.limitedFaces)},{"residual",surface->stats.residual},{"partitionError",surface->Volume()-state->volume},{"stepSeconds",surface->stats.seconds}})JS_SetPropertyStr(c,info,field.first,JS_NewFloat64(c,field.second));
      }JS_SetPropertyStr(c,v,"surface",info);return v;
    }
    if(op=="liquidAccounting"){auto a=world.Liquids().Accounting(String(c,arg(1)));auto v=JS_NewObject(c);
      for(auto field:{std::pair<const char*,double>{"reservoirs",a.reservoirs},{"containers",a.containers},{"detached",a.detached},{"total",a.total},{"expected",a.expected},{"error",a.error},{"tolerance",a.tolerance}}){JS_SetPropertyStr(c,v,field.first,JS_NewFloat64(c,field.second));}
      return v;
    }
    if(op=="liquidSample"||op=="liquidPresentedSample"){glm::vec3 p;if(!ReadVec(c,arg(1),p))return JS_ThrowTypeError(c,"finite point required");auto sample=world.Liquids().Sample(p,{},op=="liquidPresentedSample"&&s->inPresentation?s->presentationAlpha:1);if(!sample)return JS_NULL;auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"entityId",JS_NewString(c,std::to_string(sample->entity).c_str()));JS_SetPropertyStr(c,v,"material",JS_NewString(c,sample->material.id.c_str()));JS_SetPropertyStr(c,v,"density",JS_NewFloat64(c,sample->material.density));JS_SetPropertyStr(c,v,"depth",JS_NewFloat64(c,sample->depth));JS_SetPropertyStr(c,v,"coordinate",JS_NewFloat64(c,sample->coordinate));JS_SetPropertyStr(c,v,"surfacePoint",Vec(c,sample->surfacePoint));JS_SetPropertyStr(c,v,"normal",Vec(c,sample->normal));JS_SetPropertyStr(c,v,"up",Vec(c,sample->up));JS_SetPropertyStr(c,v,"velocity",Vec(c,sample->velocity));return v;}
    if(op=="liquidErrors"||op=="liquidConnections"){auto v=JS_NewArray(c);uint32_t i=0;if(op=="liquidErrors")for(auto [id,error]:world.Liquids().Errors()){auto e=JS_NewObject(c);JS_SetPropertyStr(c,e,"entityId",JS_NewString(c,std::to_string(id).c_str()));JS_SetPropertyStr(c,e,"message",JS_NewString(c,error.c_str()));JS_SetPropertyUint32(c,v,i++,e);}else for(auto [id,active]:world.Liquids().Connections()){auto e=JS_NewObject(c);JS_SetPropertyStr(c,e,"entityId",JS_NewString(c,std::to_string(id).c_str()));JS_SetPropertyStr(c,e,"active",JS_NewBool(c,active));JS_SetPropertyUint32(c,v,i++,e);}return v;}
    if(op=="liquidSubmerged"){EntityId id=0;try{id=std::stoull(String(c,arg(1)));}catch(...){return JS_ThrowReferenceError(c,"invalid entity");}auto* o=world.RuntimeDefinition(id);if(!o||!o->body)return JS_ThrowReferenceError(c,"missing collider");auto h=world.RuntimeBody(id);if(!h.IsValid())return JS_ThrowReferenceError(c,"stale collider");if(!world.Physics().IsBodyEnabled(h)){auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"volume",JS_NewFloat64(c,0));JS_SetPropertyStr(c,v,"center",Vec(c,glm::vec3(0)));JS_SetPropertyStr(c,v,"buoyancy",Vec(c,glm::vec3(0)));return v;}auto pose=world.Physics().GetTransform(h);LiquidGeometry geometry;if(o->body->shape==SceneShape::Box)geometry=LiquidBox(-glm::dvec3(o->body->halfExtents),glm::dvec3(o->body->halfExtents));else if(o->body->shape==SceneShape::Compound)for(auto box:o->body->compoundBoxes){auto g=LiquidBox(glm::dvec3(box.localCenter-box.halfExtents),glm::dvec3(box.localCenter+box.halfExtents));geometry.cells.insert(geometry.cells.end(),g.cells.begin(),g.cells.end());}else return JS_ThrowTypeError(c,"submerged supports box/compound colliders");for(auto& t:geometry.cells)for(auto& p:t)p=glm::dvec3(pose.position)+glm::dquat(pose.rotation)*p;auto a=world.Liquids().Submerged(geometry);auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"volume",JS_NewFloat64(c,a.volume));JS_SetPropertyStr(c,v,"center",Vec(c,a.center));JS_SetPropertyStr(c,v,"buoyancy",Vec(c,a.buoyancy));return v;}
    auto navPath=[&](const NavigationPath& path){auto v=JS_NewObject(c);const char* status=path.status==NavigationPath::Status::Complete?"complete":path.status==NavigationPath::Status::Partial?"partial":"failed";JS_SetPropertyStr(c,v,"status",JS_NewString(c,status));JS_SetPropertyStr(c,v,"distance",JS_NewFloat64(c,path.distance));JS_SetPropertyStr(c,v,"revision",JS_NewUint32(c,path.revision));auto corners=JS_NewArray(c);uint32_t i=0;for(auto p:path.corners){auto corner=JS_NewObject(c);JS_SetPropertyStr(c,corner,"position",Vec(c,p.position));JS_SetPropertyStr(c,corner,"linkId",JS_NewString(c,std::to_string(p.link).c_str()));JS_SetPropertyStr(c,corner,"linkEnd",Vec(c,p.linkEnd));JS_SetPropertyUint32(c,corners,i++,corner);}JS_SetPropertyStr(c,v,"corners",corners);auto areas=JS_NewArray(c);i=0;for(auto id:path.areas)JS_SetPropertyUint32(c,areas,i++,JS_NewUint32(c,id));JS_SetPropertyStr(c,v,"areas",areas);return v;};
    auto navFilter=[&](JSValueConst options,NavigationFilter& f){const auto& config=world.Navigation().Configuration();auto p=JS_GetPropertyStr(c,options,"profile");if(!JS_IsUndefined(p)){if(JS_IsString(p)){auto name=String(c,p);bool found=false;for(auto [id,profile]:config.profiles)if(profile.name==name){f.profile=id;found=true;}if(!found){JS_FreeValue(c,p);return false;}}else if(JS_ToUint32(c,&f.profile,p)!=0){JS_FreeValue(c,p);return false;}}JS_FreeValue(c,p);if(!config.profiles.count(f.profile))return false;
        for(auto entry:{std::pair<const char*,CategoryMask*>{"includeAreas",&f.include},{"excludeAreas",&f.exclude}}){auto list=JS_GetPropertyStr(c,options,entry.first);if(!JS_IsUndefined(list)){if(!JS_IsArray(list)){JS_FreeValue(c,list);return false;}auto len=JS_GetPropertyStr(c,list,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);*entry.second=0;if(n>62){JS_FreeValue(c,list);return false;}for(uint32_t i=0;i<n;++i){auto v=JS_GetPropertyUint32(c,list,i);int id=config.areas.Find(String(c,v));JS_FreeValue(c,v);if(id<0){JS_FreeValue(c,list);return false;}*entry.second|=CategoryBit(id);}}JS_FreeValue(c,list);}
        auto costs=JS_GetPropertyStr(c,options,"costs");if(!JS_IsUndefined(costs)){if(!JS_IsObject(costs)){JS_FreeValue(c,costs);return false;}for(auto [id,name]:config.areas.names){auto v=JS_GetPropertyStr(c,costs,name.c_str());if(!JS_IsUndefined(v)){double value;if(JS_ToFloat64(c,&value,v)!=0||!std::isfinite(value)||value<1||value>1e6){JS_FreeValue(c,v);JS_FreeValue(c,costs);return false;}f.costs[id]=float(value);}JS_FreeValue(c,v);}}JS_FreeValue(c,costs);return true;};
    if(op=="navSample"||op=="navPath"||op=="navRaycast"){
        glm::vec3 a,b;NavigationFilter f;if(!ReadVec(c,arg(1),a)||!navFilter(arg(3),f))return JS_ThrowTypeError(c,"invalid navigation point/filter");
        if(op!="navSample"&&!ReadVec(c,arg(2),b))return JS_ThrowTypeError(c,"invalid navigation endpoint");
        if(op=="navPath")return navPath(world.Navigation().FindPath(a,b,f));
        std::optional<NavigationLocation> location;if(op=="navSample"){double range;if(JS_ToFloat64(c,&range,arg(2))!=0||!std::isfinite(range)||range<=0||range>100)return JS_ThrowTypeError(c,"invalid sample range");location=world.Navigation().Sample(a,float(range),f);}else location=world.Navigation().Raycast(a,b,f);if(!location)return JS_NULL;auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"position",Vec(c,location->position));JS_SetPropertyStr(c,result,"surfaceId",JS_NewString(c,std::to_string(location->surface).c_str()));JS_SetPropertyStr(c,result,"area",JS_NewUint32(c,location->area));return result;
    }
    if(op=="navAreas"||op=="navProfiles"||op=="navErrors"){auto result=JS_NewArray(c);uint32_t i=0;const auto& config=world.Navigation().Configuration();if(op=="navErrors"){for(auto [id,error]:world.Navigation().Errors()){auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"entityId",JS_NewString(c,std::to_string(id).c_str()));JS_SetPropertyStr(c,v,"message",JS_NewString(c,error.c_str()));JS_SetPropertyUint32(c,result,i++,v);}}else if(op=="navAreas"){for(auto [id,name]:config.areas.names){auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"id",JS_NewUint32(c,id));JS_SetPropertyStr(c,v,"name",JS_NewString(c,name.c_str()));JS_SetPropertyUint32(c,result,i++,v);}}else for(auto [id,p]:config.profiles){auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"id",JS_NewUint32(c,id));JS_SetPropertyStr(c,v,"name",JS_NewString(c,p.name.c_str()));JS_SetPropertyStr(c,v,"radius",JS_NewFloat64(c,p.radius));JS_SetPropertyStr(c,v,"height",JS_NewFloat64(c,p.height));JS_SetPropertyUint32(c,result,i++,v);}return result;}
    if(op=="joint"||op=="jointValid"||op=="jointState"||op=="jointSet") {
        uint64_t id=0;try{const auto text=String(c,arg(1));size_t end;id=std::stoull(text,&end);if(end!=text.size())id=0;}catch(...){id=0;}
        if(op=="joint"){auto h=world.RuntimeJoint(id);return h.IsValid()?JS_NewString(c,std::to_string(h.id).c_str()):JS_NULL;}
        JointHandle h{id};JointState state;bool valid=world.Physics().GetJoint(h,state);
        if(op=="jointValid")return JS_NewBool(c,valid);
        if(!valid)return JS_ThrowReferenceError(c,"stale joint handle");
        if(op=="jointSet"){
            auto settings=state.settings;
            for(auto entry:{std::pair<const char*,bool*>{"enabled",&settings.enabled},{"limits",&settings.limits},{"motor",&settings.motor},{"spring",&settings.spring}}){
                auto value=JS_GetPropertyStr(c,arg(2),entry.first);if(!JS_IsUndefined(value)){if(!JS_IsBool(value)){JS_FreeValue(c,value);return JS_ThrowTypeError(c,"joint flag must be boolean");}*entry.second=JS_ToBool(c,value);}JS_FreeValue(c,value);
            }
            for(auto entry:{std::pair<const char*,float*>{"lower",&settings.lower},{"upper",&settings.upper},{"speed",&settings.speed},{"maxForce",&settings.maxForce},{"rest",&settings.rest},{"stiffness",&settings.stiffness},{"damping",&settings.damping}}){
                auto value=JS_GetPropertyStr(c,arg(2),entry.first);bool present=!JS_IsUndefined(value);JS_FreeValue(c,value);if(present&&!Number(c,arg(2),entry.first,*entry.second))return JS_ThrowTypeError(c,"invalid joint number");
            }
            if(!world.Physics().SetJoint(h,settings))return JS_ThrowTypeError(c,"invalid joint settings");
            return JS_TRUE;
        }
        auto result=JS_NewObject(c);
        JS_SetPropertyStr(c,result,"active",JS_NewBool(c,state.active));
        JS_SetPropertyStr(c,result,"enabled",JS_NewBool(c,state.settings.enabled));
        JS_SetPropertyStr(c,result,"coordinate",JS_NewFloat64(c,state.coordinate));
        JS_SetPropertyStr(c,result,"motorImpulse",JS_NewFloat64(c,state.motorImpulse));
        JS_SetPropertyStr(c,result,"type",JS_NewInt32(c,int(state.settings.type)));
        return result;
    }

    if(op=="sceneCurrent"||op=="sceneList"||op=="sceneLoad"||op=="sceneReload"||op=="sessionGet"||op=="sessionSet"||op=="sessionDelete") {
        auto scenes=world.SceneControl();if(!scenes)return JS_ThrowTypeError(c,"No project scene session is active");
        if(op=="sceneCurrent")return JS_NewString(c,scenes->Current().c_str());
        if(op=="sceneList"){auto a=JS_NewArray(c);uint32_t i=0;for(auto& name:scenes->Scenes())JS_SetPropertyUint32(c,a,i++,JS_NewString(c,name.c_str()));return a;}
        std::string error;
        if(op=="sceneLoad"||op=="sceneReload"){
            bool ok=op=="sceneLoad"?scenes->Request(String(c,arg(1)),error):scenes->Reload(error);
            return ok?JS_TRUE:JS_ThrowTypeError(c,"%s",error.c_str());
        }
        if(!JS_IsString(arg(1)))return JS_ThrowTypeError(c,"Session key must be a string");
        auto key=String(c,arg(1));
        if(op=="sessionGet"){auto json=scenes->Get(key);return JS_ParseJSON(c,json.c_str(),json.size(),"session");}
        if(op=="sessionDelete"){scenes->Erase(key);return JS_UNDEFINED;}
        std::string json;if(!s->Json(arg(2),json,error)||!scenes->Set(key,json,error))return JS_ThrowTypeError(c,"%s",error.c_str());
        return JS_UNDEFINED;
    }
    if(op=="uiDiagnostics"){if(argc>1){if(!JS_IsBool(arg(1)))return JS_ThrowTypeError(c,"debug visibility requires boolean");world.UI().debugOverlayVisible=JS_ToBool(c,arg(1));}return JS_NewBool(c,world.UI().debugOverlayVisible);}
    if(op=="uiFind")return JS_NewUint32(c,world.UI().Find(String(c,arg(1))));
    if(op=="uiQuit"){world.UI().RequestQuit();return JS_UNDEFINED;}
    if(op=="uiLoad"){std::string error;auto h=world.UI().Load(String(c,arg(1)),String(c,arg(2)),s->currentOwner,error,s->currentSlot);if(!h)return JS_ThrowTypeError(c,"UI: %s",error.c_str());return JS_NewUint32(c,h);}
    if(op=="uiGet"||op=="uiSet"||op=="uiUnload"){
        uint32_t h=0;JS_ToUint32(c,&h,arg(1));auto* d=world.UI().Document(h);if(!d)return JS_ThrowReferenceError(c,"stale/unloaded UI document");
        if(op=="uiUnload"){world.UI().Unload(h);return JS_TRUE;}
        auto id=String(c,arg(2)),key=String(c,arg(3));UIElement* e=id.empty()?nullptr:world.UI().Element(h,id);
        if(!id.empty()&&!e)return JS_ThrowReferenceError(c,"unknown UI element %s",id.c_str());
        bool* flag=key=="visible"?(e?&e->visible:&d->visible):key=="enabled"?(e?&e->enabled:&d->enabled):key=="modal"&&!e?&d->modal:nullptr;
        if(flag){if(op=="uiSet"){if(!JS_IsBool(arg(4)))return JS_ThrowTypeError(c,"UI flag requires boolean");*flag=JS_ToBool(c,arg(4));}return JS_NewBool(c,*flag);}
        if(e&&(key=="text"||key=="texture")){auto& value=key=="text"?e->text:e->texture;if(op=="uiSet"){if(!JS_IsString(arg(4)))return JS_ThrowTypeError(c,"UI string required");auto v=String(c,arg(4));if(v.size()>16384)return JS_ThrowTypeError(c,"UI text too long");if(key=="texture"&&!v.empty()){auto* a=s->assets?s->assets->Find(v):nullptr;if(!a||a->missing||a->type!=AssetType::Texture)return JS_ThrowTypeError(c,"invalid UI texture asset");}value=v;}return JS_NewString(c,value.c_str());}
        if(e&&key=="value"){if(op=="uiSet"){double v;if(JS_ToFloat64(c,&v,arg(4))||!std::isfinite(v)||v<e->minimum||v>e->maximum)return JS_ThrowTypeError(c,"UI value outside authored range");e->value=float(v);}return JS_NewFloat64(c,e->value);}
        return JS_ThrowTypeError(c,"unknown UI property");
    }

    if(op=="elapsed")return JS_NewFloat64(c,world.SimulationTimeSeconds());
    if(op=="delta")return JS_NewFloat64(c,s->delta);
    if(op=="fixed")return JS_NewBool(c,s->fixed);
    if(op=="pointerCapture")return JS_NewBool(c,world.pointerCapture);
    if(op=="setPointerCapture"){if(!JS_IsBool(arg(1)))return JS_ThrowTypeError(c,"pointerCapture must be boolean");world.pointerCapture=JS_ToBool(c,arg(1));return JS_TRUE;}
    if(op=="held"||op=="pressed"||op=="released"||op=="axis"){
        auto name=String(c,arg(1));if(!s->input)return op=="axis"?JS_NewFloat64(c,0):JS_FALSE;
        if(op=="axis")return JS_NewFloat64(c,s->input->Axis(name));
        auto a=s->fixed?s->input->FixedAction(name):s->input->Action(name);
        return JS_NewBool(c,op=="held"?a.held:op=="pressed"?a.pressed:a.released);
    }
    auto tagMask=[&](JSValueConst array,CategoryMask& mask){if(!JS_IsArray(array))return false;mask=0;auto length=JS_GetPropertyStr(c,array,"length");uint32_t n=0;JS_ToUint32(c,&n,length);JS_FreeValue(c,length);if(n>64)return false;
        for(uint32_t i=0;i<n;++i){auto item=JS_GetPropertyUint32(c,array,i);auto name=String(c,item);JS_FreeValue(c,item);int id=world.Categories().tags.Find(name);if(id<0)return false;mask|=CategoryBit(id);}return true;};
    auto ids=[&](const std::vector<EntityId>& values){auto array=JS_NewArray(c);uint32_t i=0;for(auto id:values)JS_SetPropertyUint32(c,array,i++,JS_NewString(c,std::to_string(id).c_str()));return array;};
    auto readTransform=[&](JSValueConst value,SceneTransform& t){auto p=JS_GetPropertyStr(c,value,"position"),r=JS_GetPropertyStr(c,value,"rotation"),scale=JS_GetPropertyStr(c,value,"scale");
        bool ok=JS_IsUndefined(p)||ReadVec(c,p,t.position);
        if(!JS_IsUndefined(r))ok=ok&&Number(c,r,"w",t.rotation.w)&&Number(c,r,"x",t.rotation.x)&&Number(c,r,"y",t.rotation.y)&&Number(c,r,"z",t.rotation.z)&&std::isfinite(glm::dot(t.rotation,t.rotation))&&glm::dot(t.rotation,t.rotation)>1e-12f;
        if(!JS_IsUndefined(scale))ok=ok&&ReadVec(c,scale,t.scale)&&t.scale.x>0&&t.scale.y>0&&t.scale.z>0;
        JS_FreeValue(c,p);JS_FreeValue(c,r);JS_FreeValue(c,scale);return ok;};
    if(op=="setView"){
        SceneTransform t;double fov=70;if(!readTransform(arg(1),t)||JS_ToFloat64(c,&fov,arg(2))||!world.SetRuntimeView(t,float(fov)))return JS_ThrowTypeError(c,"invalid runtime view");return JS_TRUE;
    }
    if(op=="clearView"){world.view.reset();return JS_TRUE;}
    if(op=="fluidSample"){
        glm::vec3 p,up,tangent;double height,radius;
        if(!ReadVec(c,arg(1),p)||!ReadVec(c,arg(2),up)||!ReadVec(c,arg(5),tangent)||JS_ToFloat64(c,&height,arg(3))||JS_ToFloat64(c,&radius,arg(4))||!std::isfinite(height)||!std::isfinite(radius)||height<0||radius<=0||glm::length(up)<1e-6f||glm::length(glm::cross(up,tangent))<1e-6f)return JS_ThrowTypeError(c,"invalid liquid query");
        auto sample=world.HasFluid()?world.FluidCoupling().SampleField(p,glm::normalize(up),float(height),float(radius),tangent):FluidFieldSample{};
        auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"immersion",JS_NewFloat64(c,sample.fraction));JS_SetPropertyStr(c,o,"density",JS_NewFloat64(c,sample.density));JS_SetPropertyStr(c,o,"velocity",Vec(c,sample.velocity));JS_SetPropertyStr(c,o,"acceleration",Vec(c,sample.acceleration));return o;
    }
    if(op.rfind("character",0)==0){
        uint64_t id=0;try{id=std::stoull(String(c,arg(1)));}catch(...){return JS_ThrowReferenceError(c,"invalid character entity");}
        auto* m=world.RuntimeCharacter(id);if(op=="characterExists")return JS_NewBool(c,m!=nullptr);
        if(!m)return JS_ThrowReferenceError(c,"stale entity or missing character motor");
        if(op=="characterVelocity"||op=="characterAcceleration"){
            if(!s->fixed)return JS_ThrowTypeError(c,"character intent must be supplied in fixedUpdate");
            glm::vec3 v;if(!ReadVec(c,arg(2),v))return JS_ThrowTypeError(c,"finite vector required");
            if(op=="characterVelocity")m->velocity=v;else m->acceleration+=v;return JS_TRUE;
        }
        if(op=="characterEnabled"||op=="characterConfigure"){
            auto config=m->settings;
            if(op=="characterEnabled"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");config.enabled=JS_ToBool(c,arg(2));}
            else {
                const char* keys[]={"radius","halfHeight","stepHeight","supportDistance","skin","maxSlopeDegrees","gravityScale","reorientationDegreesPerSecond","interactionMass","maxPushImpulse"};
                float* values[]={&config.radius,&config.halfHeight,&config.stepHeight,&config.supportDistance,&config.skin,&config.maxSlopeDegrees,&config.gravityScale,&config.reorientationDegreesPerSecond,&config.interactionMass,&config.maxPushImpulse};
                for(size_t i=0;i<10;++i){auto v=JS_GetPropertyStr(c,arg(2),keys[i]);bool exists=!JS_IsUndefined(v);JS_FreeValue(c,v);if(exists&&!Number(c,arg(2),keys[i],*values[i]))return JS_ThrowTypeError(c,"invalid motor setting");}
            }
            if(op=="characterConfigure"){
                auto offset=JS_GetPropertyStr(c,arg(2),"offset");bool ok=JS_IsUndefined(offset)||ReadVec(c,offset,config.offset);JS_FreeValue(c,offset);if(!ok)return JS_ThrowTypeError(c,"invalid offset");
                auto layer=JS_GetPropertyStr(c,arg(2),"collisionLayer");if(!JS_IsUndefined(layer)){int found=world.Categories().collision.Find(String(c,layer));JS_FreeValue(c,layer);if(found<0)return JS_ThrowTypeError(c,"unknown collision layer");config.collisionLayer=unsigned(found);}else JS_FreeValue(c,layer);
                for(const auto& entry:std::vector<std::pair<const char*,CategoryMask*>>{{"requiredTags",&config.requiredTags},{"excludedTags",&config.excludedTags}}){auto a=JS_GetPropertyStr(c,arg(2),entry.first);bool valid=JS_IsUndefined(a)||tagMask(a,*entry.second);JS_FreeValue(c,a);if(!valid)return JS_ThrowTypeError(c,"unknown motor tag");}
                auto mask=JS_GetPropertyStr(c,arg(2),"collisionMask");if(!JS_IsUndefined(mask)){uint32_t count=0;auto n=JS_GetPropertyStr(c,mask,"length");bool valid=JS_IsArray(mask)&&JS_ToUint32(c,&count,n)==0&&count<=64;JS_FreeValue(c,n);CategoryMask bits=0;for(uint32_t i=0;valid&&i<count;++i){auto v=JS_GetPropertyUint32(c,mask,i);int found=world.Categories().collision.Find(String(c,v));JS_FreeValue(c,v);if(found<0)valid=false;else bits|=CategoryBit(found);}JS_FreeValue(c,mask);if(!valid)return JS_ThrowTypeError(c,"unknown motor layer mask");config.collisionMask=bits;}else JS_FreeValue(c,mask);
            }
            std::string error;if(!ValidCharacterMotor(config,error))return JS_ThrowTypeError(c,"%s",error.c_str());world.SetCharacterSettings(id,config);return JS_TRUE;
        }
        if(op=="characterIgnore"){
            if(!JS_IsArray(arg(2)))return JS_ThrowTypeError(c,"entity array required");
            auto n=JS_GetPropertyStr(c,arg(2),"length");uint32_t length=0;JS_ToUint32(c,&length,n);JS_FreeValue(c,n);if(length>64)return JS_ThrowTypeError(c,"too many ignored entities");
            std::vector<BodyHandle> handles;for(uint32_t i=0;i<length;++i){auto v=JS_GetPropertyUint32(c,arg(2),i);uint64_t other=0;try{other=std::stoull(String(c,v));}catch(...){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"invalid ignored entity");}JS_FreeValue(c,v);auto body=world.RuntimeBody(other);if(body.IsValid())handles.push_back(body);}m->filter.ignoredBodies=std::move(handles);return JS_TRUE;
        }
        if(op!="characterState")return JS_ThrowTypeError(c,"unknown character operation");
        auto o=JS_NewObject(c);const auto& r=m->result;
        JS_SetPropertyStr(c,o,"velocity",Vec(c,m->velocity));JS_SetPropertyStr(c,o,"actualDisplacement",Vec(c,r.displacement));
        JS_SetPropertyStr(c,o,"supportNormal",Vec(c,r.supportNormal));JS_SetPropertyStr(c,o,"supportVelocity",Vec(c,r.supportVelocity));JS_SetPropertyStr(c,o,"gravity",Vec(c,world.Gravity().Sample(m->position)));JS_SetPropertyStr(c,o,"up",Vec(c,m->orientation*glm::vec3(0,1,0)));
        JS_SetPropertyStr(c,o,"supported",JS_NewBool(c,r.supported));JS_SetPropertyStr(c,o,"collided",JS_NewBool(c,r.collided));
        JS_SetPropertyStr(c,o,"supportEntityId",JS_NewString(c,std::to_string(world.EntityIdOfBody(r.support)).c_str()));
        return o;
    }
    if(op=="queryTags"){CategoryMask required,excluded;if(!tagMask(arg(1),required)||!tagMask(arg(2),excluded))return JS_ThrowTypeError(c,"unknown tag");return ids(world.QueryEntities(required,excluded));}
    if(op=="spawn"){SceneTransform t;if(!readTransform(arg(2),t))return JS_ThrowTypeError(c,"invalid transform");std::string error;auto id=world.SpawnPrefab(String(c,arg(1)),t,error);if(!id)return JS_ThrowTypeError(c,"spawn: %s",error.c_str());return JS_NewString(c,std::to_string(id).c_str());}
    if(op=="viewRay"){if(!s->hasView)return JS_NULL;auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"origin",Vec(c,s->viewOrigin));JS_SetPropertyStr(c,o,"direction",Vec(c,s->viewDirection));return o;}
    if(op=="overlap"||op=="sweep"||op=="cast"){
        glm::vec3 min,max;if(!ReadVec(c,arg(1),min)||!ReadVec(c,arg(2),max))return JS_ThrowTypeError(c,"invalid bounds");PhysicsQueryFilter filter;
        auto include=JS_GetPropertyStr(c,arg(3),"includeLayers");auto exclude=JS_GetPropertyStr(c,arg(3),"excludeLayers");
        auto layerMask=[&](JSValueConst a,CategoryMask& mask){if(JS_IsUndefined(a))return true;if(!JS_IsArray(a))return false;auto len=JS_GetPropertyStr(c,a,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);if(n>64)return false;mask=0;
            for(uint32_t i=0;i<n;++i){auto v=JS_GetPropertyUint32(c,a,i);auto name=String(c,v);JS_FreeValue(c,v);int id=world.Categories().collision.Find(name);if(id<0)return false;mask|=CategoryBit(id);}return true;};
        auto sensors=JS_GetPropertyStr(c,arg(3),"includeSensors");if(!JS_IsUndefined(sensors)&&!JS_IsBool(sensors)){JS_FreeValue(c,sensors);return JS_ThrowTypeError(c,"includeSensors must be boolean");}filter.includeSensors=JS_ToBool(c,sensors)>0;JS_FreeValue(c,sensors);
        auto required=JS_GetPropertyStr(c,arg(3),"requiredTags"),excluded=JS_GetPropertyStr(c,arg(3),"excludedTags"),ignored=JS_GetPropertyStr(c,arg(3),"ignored");
        bool tagsOk=(JS_IsUndefined(required)||tagMask(required,filter.requiredTags))&&(JS_IsUndefined(excluded)||tagMask(excluded,filter.excludedTags));
        JS_FreeValue(c,required);JS_FreeValue(c,excluded);
        if(!JS_IsUndefined(ignored)){auto len=JS_GetPropertyStr(c,ignored,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);if(n>256)tagsOk=false;
            for(uint32_t i=0;tagsOk&&i<n;++i){auto v=JS_GetPropertyUint32(c,ignored,i);auto idv=JS_GetPropertyStr(c,v,"id");try{auto body=world.RuntimeBody(std::stoull(String(c,idv)));if(body.IsValid())filter.ignoredBodies.push_back(body);}catch(...){tagsOk=false;}JS_FreeValue(c,idv);JS_FreeValue(c,v);}}
        JS_FreeValue(c,ignored);
        bool ok=tagsOk&&layerMask(include,filter.includeLayers)&&layerMask(exclude,filter.excludeLayers);JS_FreeValue(c,include);JS_FreeValue(c,exclude);if(!ok)return JS_ThrowTypeError(c,"unknown collision layer");
        if(op=="cast") {
            auto spec=arg(4);float maximum=0,radius=0,halfHeight=0;BodyTransform pose;pose.position=min;
            auto kindValue=JS_GetPropertyStr(c,spec,"kind");auto kind=String(c,kindValue);JS_FreeValue(c,kindValue);
            if(!Number(c,spec,"maximum",maximum))return JS_ThrowTypeError(c,"invalid cast maximum");
            if(kind=="sphere"||kind=="capsule")if(!Number(c,spec,"radius",radius))return JS_ThrowTypeError(c,"invalid radius");
            if(kind=="capsule"&&!Number(c,spec,"halfHeight",halfHeight))return JS_ThrowTypeError(c,"invalid capsule halfHeight");
            glm::vec3 halfExtents{0};
            if(kind=="box"){auto h=JS_GetPropertyStr(c,spec,"halfExtents");bool valid=ReadVec(c,h,halfExtents);JS_FreeValue(c,h);if(!valid)return JS_ThrowTypeError(c,"invalid halfExtents");}
            if(kind=="box"||kind=="capsule") {auto r=JS_GetPropertyStr(c,spec,"rotation");
                bool valid=JS_IsUndefined(r)||(Number(c,r,"w",pose.rotation.w)&&Number(c,r,"x",pose.rotation.x)&&Number(c,r,"y",pose.rotation.y)&&Number(c,r,"z",pose.rotation.z));
                JS_FreeValue(c,r);if(!valid)return JS_ThrowTypeError(c,"invalid cast rotation");}
            PhysicsCastHit hit;
            try {
                if(kind=="ray")hit=world.Physics().Raycast(min,max,maximum,filter);
                else if(kind=="sphere")hit=world.Physics().SphereCast(min,radius,max,maximum,filter);
                else if(kind=="capsule")hit=world.Physics().CapsuleCast(pose,radius,halfHeight,max,maximum,filter);
                else if(kind=="box")hit=world.Physics().BoxCast(pose,halfExtents,max,maximum,filter);
                else return JS_ThrowTypeError(c,"unknown cast shape");
            }catch(const std::exception& e){return JS_ThrowTypeError(c,"cast: %s",e.what());}
            if(!hit.hit)return JS_NULL;
            auto o=JS_NewObject(c);auto id=world.EntityIdOfBody(hit.body);
            JS_SetPropertyStr(c,o,"entityId",JS_NewString(c,std::to_string(id).c_str()));
            JS_SetPropertyStr(c,o,"bodyId",JS_NewUint32(c,hit.body.id));
            JS_SetPropertyStr(c,o,"point",Vec(c,hit.point));JS_SetPropertyStr(c,o,"normal",Vec(c,hit.normal));
            JS_SetPropertyStr(c,o,"distance",JS_NewFloat64(c,hit.distance));JS_SetPropertyStr(c,o,"fraction",JS_NewFloat64(c,hit.fraction));
            JS_SetPropertyStr(c,o,"primitiveIndex",JS_NewInt32(c,hit.primitiveIndex));
            JS_SetPropertyStr(c,o,"initialOverlap",JS_NewBool(c,hit.initialOverlap));
            JS_SetPropertyStr(c,o,"shape",JS_NewString(c,hit.shape==ShapeType::Sphere?"sphere":hit.shape==ShapeType::Terrain?"terrain":"box"));
            return o;
        }
        if(op=="sweep"){glm::quat rotation;auto r=arg(4);
            if(!Number(c,r,"w",rotation.w)||!Number(c,r,"x",rotation.x)||!Number(c,r,"y",rotation.y)||!Number(c,r,"z",rotation.z)||!std::isfinite(glm::dot(rotation,rotation))||glm::dot(rotation,rotation)<1e-12f)return JS_ThrowTypeError(c,"invalid rotation");
            auto hit=world.Physics().SweepPlayerShape(min,glm::normalize(rotation),max,false,0,1,&filter);auto o=JS_NewObject(c);
            JS_SetPropertyStr(c,o,"hit",JS_NewBool(c,hit.hit));JS_SetPropertyStr(c,o,"distance",JS_NewFloat64(c,hit.distance));JS_SetPropertyStr(c,o,"normal",Vec(c,hit.normal));
            auto entityId=world.EntityIdOfBody(hit.hitBody);JS_SetPropertyStr(c,o,"entityId",JS_NewString(c,std::to_string(entityId).c_str()));return o;
        }
        std::vector<EntityId> result;for(auto handle:world.Physics().QueryBodiesInAabb(min,max,filter)){auto id=world.EntityIdOfBody(handle);if(!id)for(const auto& o:world.ScriptObjects())if(world.RuntimeBody(o.id).id==handle.id){id=o.id;break;}if(id)result.push_back(id);}return ids(result);
    }
    EntityId id=0;try{auto text=String(c,arg(1));size_t end;id=std::stoull(text,&end);if(end!=text.size())id=0;}catch(...){id=0;}
    const auto* definition=world.RuntimeDefinition(id);
    if(op=="valid")return JS_NewBool(c,definition!=nullptr);
    if(!definition)return JS_ThrowReferenceError(c,"stale or invalid entity %llu",(unsigned long long)id);

    if(op=="materialInfo"||op=="materialAssign"||op=="materialOverride"||op=="materialClear"){
        unsigned slot=0;double number;if(JS_ToFloat64(c,&number,arg(2))||!std::isfinite(number)||number<0||number>=64||std::floor(number)!=number)return JS_ThrowTypeError(c,"material slot must be integer 0..63");slot=unsigned(number);
        if(!definition->render)return JS_ThrowTypeError(c,"entity has no render component");
        MaterialSlot value;const auto* current=world.FindEntity(id);const auto& currentSlots=current->definition.render->materials;if(slot<currentSlots.size())value=currentSlots[slot];
        if(op=="materialAssign"){if(!JS_IsString(arg(3)))return JS_ThrowTypeError(c,"registered material ID required");value.asset=String(c,arg(3));if(!world.SetMaterialSlot(id,slot,value))return JS_ThrowTypeError(c,"missing/wrong-type material asset");return JS_TRUE;}
        if(op=="materialClear"){value.overrides={};return JS_NewBool(c,world.SetMaterialSlot(id,slot,value));}
        if(op=="materialOverride"){
            auto options=arg(3);auto has=[&](const char* name){auto v=JS_GetPropertyStr(c,options,name);bool found=!JS_IsUndefined(v);JS_FreeValue(c,v);return found;};
            if(has("baseColor")){auto v=JS_GetPropertyStr(c,options,"baseColor");glm::vec3 rgb;float alpha;bool ok=ReadVec(c,v,rgb)&&Number(c,v,"a",alpha);JS_FreeValue(c,v);if(!ok)return JS_ThrowTypeError(c,"baseColor requires finite x,y,z,a");value.overrides.baseColor=glm::vec4(rgb,alpha);}
            if(has("emissive")){auto v=JS_GetPropertyStr(c,options,"emissive");glm::vec3 rgb;bool ok=ReadVec(c,v,rgb);JS_FreeValue(c,v);if(!ok)return JS_ThrowTypeError(c,"emissive requires finite x,y,z");value.overrides.emissive=rgb;}
            for(auto pair:{std::pair<const char*,std::optional<float>*>{"metallic",&value.overrides.metallic},{"roughness",&value.overrides.roughness},{"emissiveIntensity",&value.overrides.emissiveIntensity}})if(has(pair.first)){float v;if(!Number(c,options,pair.first,v))return JS_ThrowTypeError(c,"finite material parameter required");*pair.second=v;}
            if(!world.SetMaterialSlot(id,slot,value))return JS_ThrowTypeError(c,"material parameter out of range");
            return JS_TRUE;
        }
        MaterialDefinition source;source.model=MaterialModel::Legacy;bool ready=value.asset.empty();if(!value.asset.empty()&&world.Resources()){auto m=world.Resources()->TryGetMaterialDefinition(value.asset);ready=bool(m);if(m)source=*m;}else if(world.Resources()){auto m=world.Resources()->TryGetMeshMaterial(definition->render->meshAsset,slot);if(m)source=*m;}
        auto m=ApplyMaterialOverride(source,value.overrides);auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"asset",JS_NewString(c,value.asset.c_str()));JS_SetPropertyStr(c,result,"ready",JS_NewBool(c,ready));const char* models[]={"legacy","pbr","unlit"};const char* alphas[]={"opaque","mask","blend"};JS_SetPropertyStr(c,result,"model",JS_NewString(c,models[int(m.model)]));JS_SetPropertyStr(c,result,"alphaMode",JS_NewString(c,alphas[int(m.alpha)]));auto base=Vec(c,glm::vec3(m.baseColor));JS_SetPropertyStr(c,base,"a",JS_NewFloat64(c,m.baseColor.a));JS_SetPropertyStr(c,result,"baseColor",base);JS_SetPropertyStr(c,result,"emissive",Vec(c,m.emissive));JS_SetPropertyStr(c,result,"metallic",JS_NewFloat64(c,m.metallic));JS_SetPropertyStr(c,result,"roughness",JS_NewFloat64(c,m.roughness));JS_SetPropertyStr(c,result,"emissiveIntensity",JS_NewFloat64(c,m.emissiveIntensity));JS_SetPropertyStr(c,result,"overridden",JS_NewBool(c,bool(value.overrides.baseColor||value.overrides.emissive||value.overrides.roughness||value.overrides.metallic||value.overrides.emissiveIntensity)));return result;
    }
    if(op=="liquidOwner"){auto h=world.Liquids().Handle(id);if(!world.Liquids().Get(h))return JS_NULL;return JS_NewString(c,(std::to_string(h.generation)+":"+std::to_string(h.id)).c_str());}
    if(op=="navEnabled"){if(!JS_IsBool(arg(3)))return JS_ThrowTypeError(c,"boolean required");return JS_NewBool(c,world.SetNavigationEnabled(id,String(c,arg(2)),JS_ToBool(c,arg(3))));}
    if(op=="navObstacleInfo"){if(!definition->navigationObstacle)return JS_NULL;const auto& n=*definition->navigationObstacle;auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"enabled",JS_NewBool(c,n.enabled));JS_SetPropertyStr(c,v,"cylinder",JS_NewBool(c,n.cylinder));JS_SetPropertyStr(c,v,"halfExtents",Vec(c,n.halfExtents));JS_SetPropertyStr(c,v,"radius",JS_NewFloat64(c,n.radius));JS_SetPropertyStr(c,v,"height",JS_NewFloat64(c,n.height));return v;}
    if(op=="navLinkInfo"){if(!definition->navigationLink)return JS_NULL;const auto& n=*definition->navigationLink;auto t=world.PresentedTransform(id,definition->transform,1);auto v=JS_NewObject(c);JS_SetPropertyStr(c,v,"enabled",JS_NewBool(c,n.enabled));JS_SetPropertyStr(c,v,"bidirectional",JS_NewBool(c,n.bidirectional));JS_SetPropertyStr(c,v,"start",Vec(c,t.position+t.rotation*n.start));JS_SetPropertyStr(c,v,"end",Vec(c,t.position+t.rotation*n.end));JS_SetPropertyStr(c,v,"area",JS_NewUint32(c,n.area));return v;}
    if(op=="navAgentExists")return JS_NewBool(c,definition->navigationAgent.has_value());
    if(op=="navAgentState"||op=="navDestination"||op=="navClear"||op=="navStopped"||op=="navCompleteLink"||op=="navConfigure"){
        if(!definition->navigationAgent)return JS_ThrowTypeError(c,"entity has no navigation agent");
        if(definition->navigationAgent->enabled)world.Navigation().RegisterAgent(id);
        auto* a=world.Navigation().Agent(id);if(!a||!definition->navigationAgent->enabled)return JS_ThrowReferenceError(c,"disabled/unregistered navigation agent");
        if(op=="navDestination"){glm::vec3 v;if(!ReadVec(c,arg(2),v))return JS_ThrowTypeError(c,"finite destination required");return JS_NewBool(c,world.Navigation().SetDestination(id,v));}
        if(op=="navClear"){world.Navigation().ClearDestination(id);return JS_TRUE;}if(op=="navStopped"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");a->stopped=JS_ToBool(c,arg(2));return JS_TRUE;}if(op=="navCompleteLink")return JS_NewBool(c,world.Navigation().CompleteLink(id));
        if(op=="navConfigure"){NavigationFilter f;f.profile=definition->navigationAgent->profile;f.include=definition->navigationAgent->areas;f.costs=definition->navigationAgent->costs;if(!navFilter(arg(2),f))return JS_ThrowTypeError(c,"invalid navigation filter");auto config=*definition->navigationAgent;config.profile=f.profile;config.areas=f.include&~f.exclude;config.costs=f.costs;for(auto field:{std::pair<const char*,float*>{"speed",&config.speed},{"arrival",&config.arrival},{"repathSeconds",&config.repathSeconds}}){auto v=JS_GetPropertyStr(c,arg(2),field.first);bool present=!JS_IsUndefined(v);JS_FreeValue(c,v);if(present&&!Number(c,arg(2),field.first,*field.second))return JS_ThrowTypeError(c,"invalid navigation setting");}auto v=JS_GetPropertyStr(c,arg(2),"avoidance");if(!JS_IsUndefined(v)){if(!JS_IsBool(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"boolean required");}config.avoidance=JS_ToBool(c,v);}JS_FreeValue(c,v);SceneObject check;check.navigationAgent=config;std::string error;if(!ValidateNavigationComponents(check,error))return JS_ThrowTypeError(c,"%s",error.c_str());world.SetNavigationAgentSettings(id,config);return JS_TRUE;}
        auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"stopped",JS_NewBool(c,a->stopped));JS_SetPropertyStr(c,result,"hasDestination",JS_NewBool(c,a->hasDestination));JS_SetPropertyStr(c,result,"destination",Vec(c,a->destination));JS_SetPropertyStr(c,result,"steering",Vec(c,a->steering));JS_SetPropertyStr(c,result,"remainingDistance",JS_NewFloat64(c,a->remaining));JS_SetPropertyStr(c,result,"reached",JS_NewBool(c,a->hasDestination&&a->path.status==NavigationPath::Status::Complete&&a->remaining<=definition->navigationAgent->arrival));JS_SetPropertyStr(c,result,"nextCorner",a->corner<a->path.corners.size()?Vec(c,a->path.corners[a->corner].position):JS_NULL);JS_SetPropertyStr(c,result,"onLink",JS_NewBool(c,a->onLink));JS_SetPropertyStr(c,result,"linkId",JS_NewString(c,std::to_string(a->onLink&&a->corner<a->path.corners.size()?a->path.corners[a->corner].link:0).c_str()));JS_SetPropertyStr(c,result,"linkEnd",Vec(c,a->onLink&&a->corner<a->path.corners.size()?a->path.corners[a->corner].linkEnd:glm::vec3(0)));JS_SetPropertyStr(c,result,"path",navPath(a->path));return result;
    }
    if(op.rfind("ragdoll",0)==0){
        if(op=="ragdollExists")return JS_NewBool(c,definition->ragdoll.has_value());
        if(!definition->ragdoll)return JS_ThrowTypeError(c,"entity has no ragdoll mapping");
        if(op=="ragdollActive")return JS_NewBool(c,world.RagdollActive(id));
        if(op=="ragdollBody"){if(!JS_IsString(arg(2)))return JS_ThrowTypeError(c,"joint key required");auto body=world.RagdollBody(id,String(c,arg(2)));return JS_NewString(c,std::to_string(body).c_str());}
        std::string error;bool ok=false;
        if(op=="ragdollEnter")ok=world.EnterRagdoll(id,error);
        else if(op=="ragdollLeave"){double seconds=0;if(JS_ToFloat64(c,&seconds,arg(2))!=0||!std::isfinite(seconds)||seconds<0||seconds>3600)return JS_ThrowTypeError(c,"invalid return duration");ok=world.LeaveRagdoll(id,float(seconds),error);}
        else if(op=="ragdollEnabled"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");ok=world.SetRagdollEnabled(id,JS_ToBool(c,arg(2)),error);}
        if(!ok)return JS_ThrowTypeError(c,"ragdoll: %s",error.c_str());
        return JS_TRUE;
    }
    if(op.rfind("animation",0)==0){
        if(op=="animationExists")return JS_NewBool(c,definition->animation.has_value());
        auto* instance=world.RuntimeAnimation(id);if(!instance)return JS_ThrowTypeError(c,"entity has no animated mesh");
        auto& player=instance->playback;
        if(op=="animationInfo"){
            auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"ready",JS_NewBool(c,bool(instance->asset)));JS_SetPropertyStr(c,result,"playing",JS_NewBool(c,player.playing&&!player.stopped));JS_SetPropertyStr(c,result,"loop",JS_NewBool(c,player.loop));JS_SetPropertyStr(c,result,"speed",JS_NewFloat64(c,player.speed));JS_SetPropertyStr(c,result,"time",JS_NewFloat64(c,player.time));JS_SetPropertyStr(c,result,"clip",JS_NewString(c,player.clip.c_str()));
            auto clips=JS_NewArray(c);uint32_t i=0;if(instance->asset)for(const auto& clip:instance->asset->clips){auto value=JS_NewObject(c);JS_SetPropertyStr(c,value,"name",JS_NewString(c,clip.name.c_str()));JS_SetPropertyStr(c,value,"duration",JS_NewFloat64(c,clip.duration));JS_SetPropertyUint32(c,clips,i++,value);}JS_SetPropertyStr(c,result,"clips",clips);
            JS_SetPropertyStr(c,result,"transitioning",JS_NewBool(c,instance->mixer.Transitioning()));JS_SetPropertyStr(c,result,"transitionFraction",JS_NewFloat64(c,instance->mixer.Fraction()));JS_SetPropertyStr(c,result,"error",JS_NewString(c,instance->error.c_str()));
            auto joints=JS_NewArray(c);i=0;if(instance->asset)for(size_t node=0;node<instance->asset->skeleton.names.size();++node)JS_SetPropertyUint32(c,joints,i++,JS_NewString(c,SkeletonJointKey(instance->asset->skeleton,int(node)).c_str()));JS_SetPropertyStr(c,result,"joints",joints);
            auto layers=JS_NewArray(c);i=0;for(const auto& l:instance->layers){auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"id",JS_NewString(c,l.settings.id.c_str()));JS_SetPropertyStr(c,o,"clip",JS_NewString(c,l.settings.clip.c_str()));JS_SetPropertyStr(c,o,"weight",JS_NewFloat64(c,l.settings.weight));JS_SetPropertyStr(c,o,"enabled",JS_NewBool(c,l.settings.enabled));JS_SetPropertyStr(c,o,"additive",JS_NewBool(c,l.settings.additive));JS_SetPropertyUint32(c,layers,i++,o);}JS_SetPropertyStr(c,result,"layers",layers);return result;
        }
        if(op=="animationSet"){
            auto copy=player;auto v=JS_GetPropertyStr(c,arg(2),"speed");bool has=!JS_IsUndefined(v);JS_FreeValue(c,v);if(has&&!Number(c,arg(2),"speed",copy.speed))return JS_ThrowTypeError(c,"invalid playback speed");
            v=JS_GetPropertyStr(c,arg(2),"loop");if(!JS_IsUndefined(v)){if(!JS_IsBool(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"loop must be boolean");}copy.loop=JS_ToBool(c,v);}JS_FreeValue(c,v);player=copy;return JS_TRUE;
        }
        if(op=="animationPause"){player.playing=false;instance->mixer.paused=true;return JS_TRUE;}
        if(!instance->asset)return JS_FALSE; // request remains in the ordinary async resource path
        if(op=="animationFade"){
            double seconds;if(!JS_IsString(arg(2))||JS_ToFloat64(c,&seconds,arg(3))!=0||!std::isfinite(seconds)||seconds<0||seconds>3600)return JS_ThrowTypeError(c,"invalid fade");
            auto clip=String(c,arg(2));if(std::none_of(instance->asset->clips.begin(),instance->asset->clips.end(),[&](const auto& v){return v.name==clip;}))return JS_ThrowTypeError(c,"unknown clip");
            if(seconds>0&&instance->mixer.outgoing.size()>=16)return JS_ThrowRangeError(c,"too many interrupted fade contributors");
            instance->mixer.CrossFade(player,clip,float(seconds));world.ResolveAnimationPose(*instance,0);return JS_TRUE;
        }
        if(op=="animationLayer"||op=="animationRemoveLayer"){
            if(!JS_IsString(arg(2)))return JS_ThrowTypeError(c,"layer ID required");
            AnimationLayerSettings settings;settings.id=String(c,arg(2));
            for(const auto& l:instance->layers)if(l.settings.id==settings.id)settings=l.settings;
            if(op=="animationLayer"){
                if(!JS_IsObject(arg(3)))return JS_ThrowTypeError(c,"layer settings required");
                for(auto field:{"clip","referenceClip"}){auto v=JS_GetPropertyStr(c,arg(3),field);if(!JS_IsUndefined(v)){if(!JS_IsString(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"invalid layer string");}(std::string(field)=="clip"?settings.clip:settings.referenceClip)=String(c,v);}JS_FreeValue(c,v);}
                for(auto item:{std::pair<const char*,float*>{"weight",&settings.weight},{"speed",&settings.speed},{"time",&settings.time},{"referenceTime",&settings.referenceTime}}){auto v=JS_GetPropertyStr(c,arg(3),item.first);bool present=!JS_IsUndefined(v);JS_FreeValue(c,v);if(present&&!Number(c,arg(3),item.first,*item.second))return JS_ThrowTypeError(c,"invalid layer number");}
                for(auto item:{std::pair<const char*,bool*>{"enabled",&settings.enabled},{"additive",&settings.additive}}){auto v=JS_GetPropertyStr(c,arg(3),item.first);if(!JS_IsUndefined(v)){if(!JS_IsBool(v)){JS_FreeValue(c,v);return JS_ThrowTypeError(c,"invalid layer flag");}*item.second=JS_ToBool(c,v);}JS_FreeValue(c,v);}
                auto mask=JS_GetPropertyStr(c,arg(3),"mask");if(!JS_IsUndefined(mask)){if(!JS_IsArray(mask)){JS_FreeValue(c,mask);return JS_ThrowTypeError(c,"mask must be an array of joint keys");}auto len=JS_GetPropertyStr(c,mask,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);if(n>128){JS_FreeValue(c,mask);return JS_ThrowRangeError(c,"too many joints");}settings.mask.clear();for(uint32_t i=0;i<n;++i){auto v=JS_GetPropertyUint32(c,mask,i);if(!JS_IsString(v)){JS_FreeValue(c,v);JS_FreeValue(c,mask);return JS_ThrowTypeError(c,"joint key must be a string");}settings.mask.push_back(String(c,v));JS_FreeValue(c,v);}}JS_FreeValue(c,mask);
            }
            std::string error;if(!world.SetAnimationLayer(id,settings,op=="animationRemoveLayer",error))return JS_ThrowTypeError(c,"layer: %s",error.c_str());return JS_TRUE;
        }
        if(op=="animationPlay"){
            if(!JS_IsString(arg(2)))return JS_ThrowTypeError(c,"clip name must be a string");
            auto clip=String(c,arg(2));
            if(!clip.empty()){if(std::none_of(instance->asset->clips.begin(),instance->asset->clips.end(),[&](const auto& v){return v.name==clip;}))return JS_ThrowTypeError(c,"unknown animation clip");player.time=0;player.clip=clip;instance->mixer.Clear();}
            player.playing=true;player.stopped=false;instance->mixer.paused=false;
        }else if(op=="animationStop"){player.Stop(*instance->asset);instance->mixer.Clear();instance->mixer.paused=true;}
        else if(op=="animationSeek"){double seconds;if(JS_ToFloat64(c,&seconds,arg(2))!=0||!std::isfinite(seconds)||seconds<0||seconds>1e20)return JS_ThrowTypeError(c,"invalid animation seek");player.Seek(*instance->asset,float(seconds));}
        else return JS_ThrowTypeError(c,"unknown animation operation");
        world.ResolveAnimationPose(*instance,0);return JS_TRUE;
    }
    if(op=="colliderEnabled"){if(!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"boolean required");return JS_NewBool(c,world.SetColliderEnabled(id,JS_ToBool(c,arg(2))));}
    if(op=="scriptState"){auto slot=String(c,arg(2));uint64_t slotId=0;try{slotId=std::stoull(slot);}catch(...){return JS_ThrowTypeError(c,"invalid slot");}
        auto it=s->instances.find({id,slotId});if(it==s->instances.end()||it->second.fault)return JS_NULL;
        auto value=JS_GetPropertyStr(c,it->second.value,"state");std::string text,error;bool ok=s->Json(value,text,error);JS_FreeValue(c,value);
        if(!ok)return JS_ThrowTypeError(c,"state: %s",error.c_str());
        return JS_ParseJSON(c,text.data(),text.size(),"state snapshot");}
    if(op=="cameraInfo"){for(const auto& camera:world.PresentationCameras())if(camera.id==id){auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"enabled",JS_NewBool(c,camera.settings.enabled));JS_SetPropertyStr(c,o,"width",JS_NewInt32(c,camera.settings.width));JS_SetPropertyStr(c,o,"height",JS_NewInt32(c,camera.settings.height));return o;}return JS_NULL;}
    if(op=="parent")return JS_NewString(c,std::to_string(definition->parent).c_str());
    if(op=="children"){std::vector<EntityId> result;for(const auto& o:world.ScriptObjects())if(o.parent==id)result.push_back(o.id);return ids(result);}
    if(op=="destroy"){std::string error;if(!world.DestroyHierarchy(id,error))return JS_ThrowTypeError(c,"destroy: %s",error.c_str());return JS_TRUE;}
    if(op=="transform"||op=="presentedTransform"){auto t=world.PresentedTransform(id,definition->transform,op=="presentedTransform"&&s->inPresentation?s->presentationAlpha:1);auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"position",Vec(c,t.position));JS_SetPropertyStr(c,result,"scale",Vec(c,t.scale));auto q=Vec(c,{t.rotation.x,t.rotation.y,t.rotation.z});JS_SetPropertyStr(c,q,"w",JS_NewFloat64(c,t.rotation.w));JS_SetPropertyStr(c,result,"rotation",q);return result;}
    if(op=="setTransform"){auto t=world.PresentedTransform(id,definition->transform,1);if(!readTransform(arg(2),t)||!world.SetRuntimeTransform(id,t))return JS_ThrowTypeError(c,"invalid transform");return JS_TRUE;}
    if(op=="hasTag"||op=="addTag"||op=="removeTag"){auto tag=world.Categories().tags.Find(String(c,arg(2)));if(tag<0)return JS_ThrowTypeError(c,"unknown tag");return JS_NewBool(c,op=="hasTag"?world.HasTag(id,tag):op=="addTag"?world.AddTag(id,tag):world.RemoveTag(id,tag));}
    if(op=="classification"){auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"renderLayer",JS_NewUint32(c,world.RenderLayerOf(id)));unsigned layer=0;CategoryMask mask=0;world.Physics().GetCollisionFilter(world.RuntimeBody(id),layer,mask);JS_SetPropertyStr(c,o,"collisionLayer",JS_NewUint32(c,layer));JS_SetPropertyStr(c,o,"collisionMask",JS_NewString(c,std::to_string(mask).c_str()));return o;}
    if(op=="audioEnabled"&&!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"audio enabled must be boolean");
    if(op=="audioEnabled")return JS_NewBool(c,world.SetAudioEnabled(id,JS_ToBool(c,arg(2))));
    if(op=="audioInfo"){for(const auto& emitter:world.AudioEmitters())if(emitter.id==id){auto o=JS_NewObject(c);AudioVoiceSnapshot snapshot;
        auto* audio=world.Resources()?world.Resources()->GetAudioSystem():nullptr;bool loaded=audio&&audio->Snapshot(emitter.voice,snapshot);
        JS_SetPropertyStr(c,o,"enabled",JS_NewBool(c,emitter.settings.enabled));JS_SetPropertyStr(c,o,"playing",JS_NewBool(c,loaded&&snapshot.state==AudioPlaybackState::Playing));JS_SetPropertyStr(c,o,"requested",JS_NewBool(c,emitter.wantPlay));return o;}return JS_NULL;}
    if(op=="playAudio"||op=="stopAudio"||op=="pauseAudio"||op=="resumeAudio")return JS_NewBool(c,op=="playAudio"?world.PlayAudio(id):op=="stopAudio"?world.StopAudio(id):op=="pauseAudio"?world.PauseAudio(id):world.ResumeAudio(id));
    if(op=="burst"){uint32_t n=0;JS_ToUint32(c,&n,arg(2));if(n>65536)return JS_ThrowTypeError(c,"burst too large");return JS_NewBool(c,world.EmitParticleBurst(id,n));}
    if(op=="particles"){for(auto& e:world.VisualEmitters())if(e.id==id){auto settings=e.pool.settings;auto enabled=JS_GetPropertyStr(c,arg(2),"enabled");if(!JS_IsUndefined(enabled))settings.enabled=JS_ToBool(c,enabled);JS_FreeValue(c,enabled);
            auto rate=JS_GetPropertyStr(c,arg(2),"rate");if(!JS_IsUndefined(rate)&&!Number(c,arg(2),"rate",settings.rate)){JS_FreeValue(c,rate);return JS_ThrowTypeError(c,"invalid rate");}JS_FreeValue(c,rate);
            if(!ValidParticleSettings(settings))return JS_ThrowTypeError(c,"invalid particles");
            e.pool.settings=settings;return JS_TRUE;}return JS_FALSE;}
    if(op=="camera"&&!JS_IsBool(arg(2)))return JS_ThrowTypeError(c,"camera enabled must be boolean");
    if(op=="camera"){for(auto& camera:world.PresentationCameras())if(camera.id==id){camera.settings.enabled=JS_ToBool(c,arg(2));return JS_TRUE;}return JS_FALSE;}
    auto body=world.RuntimeBody(id);if(!body.IsValid()||!world.Physics().IsDynamicBody(body))return JS_ThrowTypeError(c,"entity has no active dynamic body");
    if(op=="mass")return JS_NewFloat64(c,world.Physics().GetMass(body));
    if(op=="inertiaWorld"){const auto matrix=world.Physics().GetInertiaWorld(body);auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"x",Vec(c,matrix[0]));JS_SetPropertyStr(c,result,"y",Vec(c,matrix[1]));JS_SetPropertyStr(c,result,"z",Vec(c,matrix[2]));return result;}
    if(op=="velocity")return Vec(c,world.Physics().GetLinearVelocity(body));
    if(op=="angularVelocity")return Vec(c,world.Physics().GetAngularVelocity(body));
    glm::vec3 v;if(!ReadVec(c,arg(2),v))return JS_ThrowTypeError(c,"invalid vector");
    if(op=="force")world.Physics().ApplyForce(body,v);
    else if(op=="impulse")world.Physics().ApplyLinearImpulse(body,v);
    else if(op=="impulseAtPoint"){glm::vec3 point;if(!ReadVec(c,arg(3),point))return JS_ThrowTypeError(c,"invalid impulse point");world.Physics().ApplyImpulseAtPoint(body,v,point);}
    else if(op=="torque")world.Physics().ApplyTorque(body,v);
    else if(op=="setVelocity")world.Physics().SetLinearVelocity(body,v);
    else if(op=="setAngularVelocity")world.Physics().SetAngularVelocity(body,v);
    else return JS_ThrowTypeError(c,"unknown engine operation");
    return JS_UNDEFINED;
}
ScriptSystem::ScriptSystem(RuntimeWorld* world,const AssetDatabase* assets):m(std::make_unique<Impl>(world,assets)){}
ScriptSystem::~ScriptSystem()=default;
void ScriptSystem::SetBudget(unsigned n){m->budget=std::max(1u,n);}
const std::vector<ScriptDiagnostic>& ScriptSystem::Diagnostics()const{return m->diagnostics;}
void ScriptSystem::Stop(){
 JUDAS_PROFILE_SCOPE("JavaScript destroy");m->Stop();}
void ScriptSystem::Synchronize(const std::vector<SceneObject>& objects){
 JUDAS_PROFILE_SCOPE("Script synchronization");
 JUDAS_PROFILE_COUNTER("Script instances",double(m->instances.size()),ProfileCounterMode::Latest);m->CheckThread();
    std::set<std::pair<SceneObjectId,uint64_t>> alive;
    for(const auto& object:objects)for(const auto& slot:object.scripts)if(slot.enabled){auto key=std::make_pair(object.id,slot.id);alive.insert(key);
        if(m->instances.count(key))continue;
        Impl::Instance i;i.order=&slot-object.scripts.data();i.entity=object.id;i.slot=slot;m->currentOwner=object.id;m->currentSlot=slot.id;m->polls=0;
        auto ns=m->Namespace(slot.asset);if(JS_IsException(ns)){m->Error(i,"module");m->instances.emplace(key,std::move(i));continue;}
        auto schema=JS_GetPropertyStr(m->ctx,ns,"properties");std::string schemaText="{}",schemaError;
        if(!JS_IsUndefined(schema))m->Json(schema,schemaText,schemaError);
        JS_FreeValue(m->ctx,schema);
        std::vector<ScriptProperty> fields;
        if(!schemaError.empty()||!ReadProperties(schemaText,slot.properties,fields,schemaError)){
            JS_FreeValue(m->ctx,ns);JS_ThrowTypeError(m->ctx,"properties: %s",schemaError.c_str());m->Error(i,"properties");m->instances.emplace(key,std::move(i));continue;}
        auto ctor=JS_GetPropertyStr(m->ctx,ns,"default");JS_FreeValue(m->ctx,ns);
        auto propertyText=WriteProperties(fields);
        auto props=JS_ParseJSON(m->ctx,propertyText.data(),propertyText.size(),"authored properties");
        auto args=JS_NewObject(m->ctx);JS_SetPropertyStr(m->ctx,args,"properties",props);
        auto libraryNs=m->NamespaceLibrary();
        auto entityFn=JS_GetPropertyStr(m->ctx,libraryNs,"entity");auto id=JS_NewString(m->ctx,std::to_string(object.id).c_str());auto self=JS_Call(m->ctx,entityFn,JS_UNDEFINED,1,&id);
        JS_FreeValue(m->ctx,id);JS_FreeValue(m->ctx,entityFn);JS_FreeValue(m->ctx,libraryNs);JS_SetPropertyStr(m->ctx,args,"entity",self);
        i.value=JS_CallConstructor(m->ctx,ctor,1,&args);JS_FreeValue(m->ctx,ctor);JS_FreeValue(m->ctx,args);
        if(JS_IsException(i.value)){i.value=JS_UNDEFINED;m->Error(i,"construct");}
        else {auto state=m->restored.find(key);std::string text=state==m->restored.end()?"{}":state->second;
            auto existing=JS_GetPropertyStr(m->ctx,i.value,"state");if(state!=m->restored.end()||JS_IsUndefined(existing))JS_SetPropertyStr(m->ctx,i.value,"state",JS_ParseJSON(m->ctx,text.data(),text.size(),"saved state"));JS_FreeValue(m->ctx,existing);}
        m->instances.emplace(key,std::move(i));
    }
    for(auto it=m->instances.begin();it!=m->instances.end();)if(!alive.count(it->first)){m->Callback(it->second,"destroy");if(m->world&&m->world->UIIfLoaded())m->world->UI().RemoveSlotOwner(it->first.first,it->first.second);JS_FreeValue(m->ctx,it->second.value);it=m->instances.erase(it);}else ++it;
}
void ScriptSystem::Frame(const InputSystem* input,float dt){
 JUDAS_PROFILE_SCOPE("JavaScript update");m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    std::vector<std::pair<SceneObjectId,uint64_t>> order;for(const auto& entry:m->instances)order.push_back(entry.first);
    // Slot order follows authored vector order, not numeric slot identity.
    if(m->world){order.clear();for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts)if(m->instances.count({o.id,slot.id}))order.push_back({o.id,slot.id});}
    for(auto key:order){auto it=m->instances.find(key);if(it==m->instances.end()||(m->world&&!m->world->RuntimeDefinition(key.first)))continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"update");}}
void ScriptSystem::Fixed(const InputSystem* input,float dt){
 JUDAS_PROFILE_SCOPE("JavaScript fixedUpdate");m->CheckThread();m->input=input;m->delta=dt;m->fixed=true;
    auto objects=m->world?m->world->ScriptObjects():std::vector<SceneObject>{};
    for(const auto& o:objects)for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end()||!m->world->RuntimeDefinition(o.id))continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"fixedUpdate");}}
std::vector<ScriptStateRecord> ScriptSystem::Capture()const{m->CheckThread();std::vector<ScriptStateRecord> result;
    for(const auto& entry:m->instances){const auto& i=entry.second;if(i.fault||(m->world&&!m->world->RuntimeDefinition(i.entity)))continue;auto value=JS_GetPropertyStr(m->ctx,i.value,"state");std::string text,error;
        if(m->Json(value,text,error))result.push_back({i.entity,i.slot.id,text});else {result.push_back({i.entity,i.slot.id,""});std::fprintf(stderr,"script state %llu/%llu: %s\n",(unsigned long long)i.entity,(unsigned long long)i.slot.id,error.c_str());}JS_FreeValue(m->ctx,value);}
    return result;}
bool ScriptSystem::Restore(const std::vector<ScriptStateRecord>& records,std::string& error){m->CheckThread();for(const auto& r:records)if(!ValidateJson(r.json,error))return false;
    for(const auto& r:records){auto key=std::make_pair(r.entity,r.slot);m->restored[key]=r.json;if(auto it=m->instances.find(key);it!=m->instances.end())JS_SetPropertyStr(m->ctx,it->second.value,"state",JS_ParseJSON(m->ctx,r.json.data(),r.json.size(),"saved state"));}return true;}
bool ScriptSystem::ValidateJson(const std::string& text,std::string& error,bool requireObject){error.clear();if(text.size()>65536){error="JSON exceeds 64KiB";return false;}Impl vm(nullptr,nullptr);vm.polls=0;auto value=JS_ParseJSON(vm.ctx,text.data(),text.size(),"JSON");if(JS_IsException(value)){error=Exception(vm.ctx);return false;}std::string canonical;bool ok=(!requireObject||JS_IsObject(value))&&vm.Json(value,canonical,error);if(!ok&&error.empty())error="JSON root must be object";JS_FreeValue(vm.ctx,value);return ok;}
bool ScriptSystem::Inspect(const AssetDatabase& assets,const std::string& asset,std::string& schema,std::string& error){Impl vm(nullptr,&assets);vm.polls=0;auto ns=vm.Namespace(asset);if(JS_IsException(ns)){error=Exception(vm.ctx);return false;}auto value=JS_GetPropertyStr(vm.ctx,ns,"properties");JS_FreeValue(vm.ctx,ns);if(JS_IsUndefined(value)){JS_FreeValue(vm.ctx,value);schema="{}";return true;}bool ok=vm.Json(value,schema,error);JS_FreeValue(vm.ctx,value);return ok;}
bool ScriptSystem::ReadProperties(const std::string& schema,const std::string& values,std::vector<ScriptProperty>& out,std::string& error){
    if(!ValidateJson(schema,error)||!ValidateJson(values,error))return false;
    Impl vm(nullptr,nullptr);auto* c=vm.ctx;auto definition=JS_ParseJSON(c,schema.data(),schema.size(),"schema"),data=JS_ParseJSON(c,values.data(),values.size(),"properties");
    JSPropertyEnum* keys=nullptr;uint32_t count=0;bool ok=JS_GetOwnPropertyNames(c,&keys,&count,definition,JS_GPN_STRING_MASK)==0;
    std::vector<ScriptProperty> fields;
    for(uint32_t i=0;ok&&i<count;++i){ScriptProperty field;auto name=JS_AtomToString(c,keys[i].atom);field.name=String(c,name);JS_FreeValue(c,name);
        auto spec=JS_GetProperty(c,definition,keys[i].atom);auto type=JS_GetPropertyStr(c,spec,"type");field.type=String(c,type);JS_FreeValue(c,type);
        auto value=JS_GetProperty(c,data,keys[i].atom);if(JS_IsUndefined(value)){JS_FreeValue(c,value);value=JS_GetPropertyStr(c,spec,"default");}
        if(field.type=="number"){double number=0;ok=JS_IsNumber(value)&&JS_ToFloat64(c,&number,value)==0&&std::isfinite(number);field.number=number;}
        else if(field.type=="boolean"){ok=JS_IsBool(value);field.boolean=JS_ToBool(c,value);}
        else if(field.type=="string"){ok=JS_IsString(value);field.text=String(c,value);}
        else ok=false;
        JS_FreeValue(c,value);JS_FreeValue(c,spec);fields.push_back(field);
        if(!ok)error="invalid schema/default/value for "+field.name;
    }
    JS_FreePropertyEnum(c,keys,count);
    keys=nullptr;count=0;JS_GetOwnPropertyNames(c,&keys,&count,data,JS_GPN_STRING_MASK);
    for(uint32_t i=0;ok&&i<count;++i){auto name=JS_AtomToString(c,keys[i].atom);auto text=String(c,name);JS_FreeValue(c,name);
        if(std::none_of(fields.begin(),fields.end(),[&](const auto& f){return f.name==text;})){error="undeclared property "+text;ok=false;}}
    JS_FreePropertyEnum(c,keys,count);JS_FreeValue(c,definition);JS_FreeValue(c,data);if(ok)out=std::move(fields);return ok;
}
std::string ScriptSystem::WriteProperties(const std::vector<ScriptProperty>& values){Impl vm(nullptr,nullptr);auto o=JS_NewObject(vm.ctx);
    for(const auto& field:values){auto v=field.type=="number"?JS_NewFloat64(vm.ctx,field.number):field.type=="boolean"?JS_NewBool(vm.ctx,field.boolean):JS_NewString(vm.ctx,field.text.c_str());JS_SetPropertyStr(vm.ctx,o,field.name.c_str(),v);}
    auto json=JS_JSONStringify(vm.ctx,o,JS_UNDEFINED,JS_UNDEFINED);auto text=String(vm.ctx,json);JS_FreeValue(vm.ctx,json);JS_FreeValue(vm.ctx,o);return text;}
bool ScriptSystem::SourceFingerprint(const AssetDatabase& assets,const Scene& scene,std::string& digest,std::string& error,bool strict){
    Impl vm(nullptr,&assets);std::set<std::string> roots;std::map<std::string,std::string> sources;
    for(const auto& object:scene.Objects())for(const auto& slot:object.scripts){const auto* record=assets.Find(slot.asset);
        if(!slot.id||!ValidateJson(slot.properties,error))return false;
        if(!record||record->missing||record->type!=AssetType::Script){
            if(strict){error="missing script asset "+slot.asset;return false;}
            sources[slot.asset]="MISSING";continue;}
        roots.insert(record->relativePath);
    }
    for(const auto& root:roots){vm.polls=0;auto* module=vm.Module(root);
        if(!module||JS_ResolveModule(vm.ctx,JS_MKPTR(JS_TAG_MODULE,module))<0){error=root+": "+Exception(vm.ctx);
            if(strict)return false;
            break;}}
    // Strict project-code compatibility also covers scripts loaded by future
    // runtime SpawnPrefab calls. IDs/content are portable; absolute paths are not.
    for(const auto& entry:assets.Records())if(entry.second.type==AssetType::Script){const auto& record=entry.second;
        if(record.missing){sources[record.id]="MISSING";continue;}std::ifstream file(record.path);std::ostringstream data;data<<file.rdbuf();sources[record.id]=data.str();}
    std::string bytes="Judas.ScriptSources.1";for(const auto& entry:sources){bytes+=entry.first;bytes+=SceneFingerprintSha256(entry.second);}digest=SceneFingerprintSha256(bytes);error.clear();return true;
}

void ScriptSystem::UIFrame(const InputSystem* input,float dt){
 JUDAS_PROFILE_SCOPE("JavaScript UI update");m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end())continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"uiUpdate");}
}
void ScriptSystem::UIEvents(const InputSystem* input,float dt){
 JUDAS_PROFILE_SCOPE("JavaScript UI events");m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    if(!m->world->UIIfLoaded())return;
    auto events=m->world->UI().TakeEvents();
    for(const auto& event:events)for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end()||it->second.fault||!m->world->RuntimeDefinition(o.id))continue;
        auto& i=it->second;ProfileScope assetScope(m->AssetLabel(i));m->polls=0;m->currentOwner=i.entity;m->currentSlot=i.slot.id;auto fn=JS_GetPropertyStr(m->ctx,i.value,"onUI");if(JS_IsException(fn)){m->Error(i,"onUI");JS_FreeValue(m->ctx,fn);continue;}if(JS_IsFunction(m->ctx,fn)){
            auto e=JS_NewObject(m->ctx);JS_SetPropertyStr(m->ctx,e,"document",JS_NewString(m->ctx,event.document.c_str()));JS_SetPropertyStr(m->ctx,e,"element",JS_NewString(m->ctx,event.element.c_str()));JS_SetPropertyStr(m->ctx,e,"type",JS_NewString(m->ctx,event.type.c_str()));JS_SetPropertyStr(m->ctx,e,"value",JS_NewFloat64(m->ctx,event.value));
            auto result=JS_Call(m->ctx,fn,i.value,1,&e);if(JS_IsException(result))m->Error(i,"onUI");else if(JS_PromiseState(m->ctx,result)!=JS_PROMISE_NOT_A_PROMISE){JS_ThrowTypeError(m->ctx,"async UI callback unsupported");m->Error(i,"onUI");}JS_FreeValue(m->ctx,result);JS_FreeValue(m->ctx,e);
        }JS_FreeValue(m->ctx,fn);
    }
}

void ScriptSystem::PhysicsEvent(SceneObjectId self,SceneObjectId other,const PhysicsWorld::TouchEvent& event,bool reverse){
 JUDAS_PROFILE_SCOPE("JavaScript contact events");
    m->CheckThread();m->fixed=true;
    auto* definition=m->world->RuntimeDefinition(self);if(!definition)return;
    const auto slots=definition->scripts;
    const char* names[2][3]={{"onCollisionEnter","onCollisionStay","onCollisionExit"},{"onTriggerEnter","onTriggerStay","onTriggerExit"}};
    const char* name=names[event.sensor?1:0][static_cast<int>(event.phase)];
    for(const auto& slot:slots){
        auto* live=m->world->RuntimeDefinition(self);if(!live)return;
        if(event.phase!=PhysicsWorld::TouchPhase::Exit&&!m->world->Physics().IsBodyEnabled(m->world->RuntimeBody(self)))return;
        if(std::none_of(live->scripts.begin(),live->scripts.end(),[&](const auto& s){return s.id==slot.id&&s.enabled;}))continue;
        auto it=m->instances.find({self,slot.id});if(it==m->instances.end()||it->second.fault)continue;
        auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}
        if(!m->world->RuntimeDefinition(self)||i.fault)continue;
        ProfileScope assetScope(m->AssetLabel(i));
        m->polls=0;m->currentOwner=self;m->currentSlot=slot.id;
        auto fn=JS_GetPropertyStr(m->ctx,i.value,name);if(JS_IsException(fn)){m->Error(i,name);JS_FreeValue(m->ctx,fn);continue;}
        if(JS_IsFunction(m->ctx,fn)){
            auto e=JS_NewObject(m->ctx);
            auto ns=m->NamespaceLibrary();auto entityFn=JS_GetPropertyStr(m->ctx,ns,"entity");auto id=JS_NewString(m->ctx,std::to_string(other).c_str());
            auto handle=JS_Call(m->ctx,entityFn,JS_UNDEFINED,1,&id);
            JS_FreeValue(m->ctx,id);JS_FreeValue(m->ctx,entityFn);JS_FreeValue(m->ctx,ns);
            JS_SetPropertyStr(m->ctx,e,"other",handle);
            JS_SetPropertyStr(m->ctx,e,"point",Vec(m->ctx,event.point));
            JS_SetPropertyStr(m->ctx,e,"normal",Vec(m->ctx,reverse?-event.normal:event.normal));
            JS_SetPropertyStr(m->ctx,e,"relativeVelocity",Vec(m->ctx,reverse?-event.relativeVelocity:event.relativeVelocity));
            JS_SetPropertyStr(m->ctx,e,"normalImpulse",event.impulseAvailable?JS_NewFloat64(m->ctx,event.normalImpulse):JS_NULL);
            auto result=JS_Call(m->ctx,fn,i.value,1,&e);
            if(JS_IsException(result))m->Error(i,name);else if(JS_PromiseState(m->ctx,result)!=JS_PROMISE_NOT_A_PROMISE){JS_ThrowTypeError(m->ctx,"async contact callbacks unsupported");m->Error(i,name);}
            JS_FreeValue(m->ctx,result);JS_FreeValue(m->ctx,e);
        }JS_FreeValue(m->ctx,fn);
    }
}

void ScriptSystem::SetView(const glm::mat4& view) {
    m->CheckThread();auto inverse=glm::inverse(view);m->viewOrigin=glm::vec3(inverse[3]);
    m->viewDirection=-glm::vec3(inverse[2]);m->hasView=true;
}

void ScriptSystem::Presentation(const InputSystem* input,float dt,float alpha){
 JUDAS_PROFILE_SCOPE("JavaScript presentation");
    m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    m->presentationAlpha=std::clamp(alpha,0.f,1.f);m->inPresentation=true;
    for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts){
        auto it=m->instances.find({o.id,slot.id});
        if(it==m->instances.end()||!m->world->RuntimeDefinition(o.id))continue;
        auto& instance=it->second;
        if(!instance.started){instance.started=true;m->Callback(instance,"start");}
        m->Callback(instance,"presentationUpdate",true);
    }
    m->inPresentation=false;
}
