#include "ScriptSystem.h"
#include "AssetDatabase.h"
#include "InputSystem.h"
#include "SceneFingerprint.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
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
 set transform(value){call('setTransform',this.id,value)}
 get parent(){return entity(call('parent',this.id))}
 get children(){return call('children',this.id).map(entity)}
 destroy(){return call('destroy',this.id)}
 hasTag(tag){return call('hasTag',this.id,tag)}
 addTag(tag){return call('addTag',this.id,tag)}
 removeTag(tag){return call('removeTag',this.id,tag)}
 get classification(){return call('classification',this.id)}
 get audio(){return call('audioInfo',this.id)}
 setAudioEnabled(enabled){return call('audioEnabled',this.id,enabled)}
 get camera(){return call('cameraInfo',this.id)}
 scriptState(slot){return call('scriptState',this.id,slot)}
 applyForce(value){call('force',this.id,value)}
 applyImpulse(value){call('impulse',this.id,value)}
 applyTorque(value){call('torque',this.id,value)}
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
export const world={entity,queryTags:(required=[],excluded=[])=>call('queryTags',required,excluded).map(entity),
 spawnPrefab:(asset,transform)=>entity(call('spawn',asset,transform)),
 overlap:(min,max,filter={})=>call('overlap',min,max,filter).map(entity),
 sweepCapsule:(from,displacement,rotation={w:1,x:0,y:0,z:0},filter={})=>call('sweep',from,displacement,filter,rotation)};
export const input={held:name=>call('held',name),pressed:name=>call('pressed',name),released:name=>call('released',name),axis:name=>call('axis',name)};
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
    struct Instance {SceneObjectId entity;SceneScriptSlot slot;JSValue value=JS_UNDEFINED;bool started=false,fault=false;size_t order=0;};
    std::map<std::pair<SceneObjectId,std::uint64_t>,Instance> instances;
    std::map<std::string,JSModuleDef*> modules;
    std::vector<ScriptDiagnostic> diagnostics;
    const InputSystem* input=nullptr;bool fixed=false;float delta=0;
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
    ~Impl(){Stop();JS_FreeContext(ctx);JS_FreeRuntime(rt);}
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
    void Callback(Instance& i,const char* name){if(i.fault)return;currentOwner=i.entity;currentSlot=i.slot.id;polls=0;auto fn=JS_GetPropertyStr(ctx,i.value,name);
        if(JS_IsException(fn)){Error(i,name);return;}if(JS_IsFunction(ctx,fn)){auto dt=JS_NewFloat64(ctx,delta);auto result=JS_Call(ctx,fn,i.value,1,&dt);if(JS_IsException(result))Error(i,name);else if(JS_PromiseState(ctx,result)!=JS_PROMISE_NOT_A_PROMISE){JS_ThrowTypeError(ctx,"async gameplay callbacks are unsupported");Error(i,name);}JS_FreeValue(ctx,result);}JS_FreeValue(ctx,fn);
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
    if(!s->world)return JS_ThrowTypeError(c,"world API unavailable in metadata context");
    auto& world=*s->world;

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
    if(op=="queryTags"){CategoryMask required,excluded;if(!tagMask(arg(1),required)||!tagMask(arg(2),excluded))return JS_ThrowTypeError(c,"unknown tag");return ids(world.QueryEntities(required,excluded));}
    if(op=="spawn"){SceneTransform t;if(!readTransform(arg(2),t))return JS_ThrowTypeError(c,"invalid transform");std::string error;auto id=world.SpawnPrefab(String(c,arg(1)),t,error);if(!id)return JS_ThrowTypeError(c,"spawn: %s",error.c_str());return JS_NewString(c,std::to_string(id).c_str());}
    if(op=="overlap"||op=="sweep"){
        glm::vec3 min,max;if(!ReadVec(c,arg(1),min)||!ReadVec(c,arg(2),max))return JS_ThrowTypeError(c,"invalid bounds");PhysicsQueryFilter filter;
        auto include=JS_GetPropertyStr(c,arg(3),"includeLayers");auto exclude=JS_GetPropertyStr(c,arg(3),"excludeLayers");
        auto layerMask=[&](JSValueConst a,CategoryMask& mask){if(JS_IsUndefined(a))return true;if(!JS_IsArray(a))return false;auto len=JS_GetPropertyStr(c,a,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);if(n>64)return false;mask=0;
            for(uint32_t i=0;i<n;++i){auto v=JS_GetPropertyUint32(c,a,i);auto name=String(c,v);JS_FreeValue(c,v);int id=world.Categories().collision.Find(name);if(id<0)return false;mask|=CategoryBit(id);}return true;};
        auto required=JS_GetPropertyStr(c,arg(3),"requiredTags"),excluded=JS_GetPropertyStr(c,arg(3),"excludedTags"),ignored=JS_GetPropertyStr(c,arg(3),"ignored");
        bool tagsOk=(JS_IsUndefined(required)||tagMask(required,filter.requiredTags))&&(JS_IsUndefined(excluded)||tagMask(excluded,filter.excludedTags));
        JS_FreeValue(c,required);JS_FreeValue(c,excluded);
        if(!JS_IsUndefined(ignored)){auto len=JS_GetPropertyStr(c,ignored,"length");uint32_t n=0;JS_ToUint32(c,&n,len);JS_FreeValue(c,len);if(n>256)tagsOk=false;
            for(uint32_t i=0;tagsOk&&i<n;++i){auto v=JS_GetPropertyUint32(c,ignored,i);auto idv=JS_GetPropertyStr(c,v,"id");try{auto body=world.RuntimeBody(std::stoull(String(c,idv)));if(body.IsValid())filter.ignoredBodies.push_back(body);}catch(...){tagsOk=false;}JS_FreeValue(c,idv);JS_FreeValue(c,v);}}
        JS_FreeValue(c,ignored);
        bool ok=tagsOk&&layerMask(include,filter.includeLayers)&&layerMask(exclude,filter.excludeLayers);JS_FreeValue(c,include);JS_FreeValue(c,exclude);if(!ok)return JS_ThrowTypeError(c,"unknown collision layer");
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
    if(op=="scriptState"){auto slot=String(c,arg(2));uint64_t slotId=0;try{slotId=std::stoull(slot);}catch(...){return JS_ThrowTypeError(c,"invalid slot");}
        auto it=s->instances.find({id,slotId});if(it==s->instances.end()||it->second.fault)return JS_NULL;
        auto value=JS_GetPropertyStr(c,it->second.value,"state");std::string text,error;bool ok=s->Json(value,text,error);JS_FreeValue(c,value);
        if(!ok)return JS_ThrowTypeError(c,"state: %s",error.c_str());
        return JS_ParseJSON(c,text.data(),text.size(),"state snapshot");}
    if(op=="cameraInfo"){for(const auto& camera:world.PresentationCameras())if(camera.id==id){auto o=JS_NewObject(c);JS_SetPropertyStr(c,o,"enabled",JS_NewBool(c,camera.settings.enabled));JS_SetPropertyStr(c,o,"width",JS_NewInt32(c,camera.settings.width));JS_SetPropertyStr(c,o,"height",JS_NewInt32(c,camera.settings.height));return o;}return JS_NULL;}
    if(op=="parent")return JS_NewString(c,std::to_string(definition->parent).c_str());
    if(op=="children"){std::vector<EntityId> result;for(const auto& o:world.ScriptObjects())if(o.parent==id)result.push_back(o.id);return ids(result);}
    if(op=="destroy"){std::string error;if(!world.DestroyHierarchy(id,error))return JS_ThrowTypeError(c,"destroy: %s",error.c_str());return JS_TRUE;}
    if(op=="transform"){auto t=world.PresentedTransform(id,definition->transform,1);auto result=JS_NewObject(c);JS_SetPropertyStr(c,result,"position",Vec(c,t.position));JS_SetPropertyStr(c,result,"scale",Vec(c,t.scale));auto q=Vec(c,{t.rotation.x,t.rotation.y,t.rotation.z});JS_SetPropertyStr(c,q,"w",JS_NewFloat64(c,t.rotation.w));JS_SetPropertyStr(c,result,"rotation",q);return result;}
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
    if(op=="velocity")return Vec(c,world.Physics().GetLinearVelocity(body));
    if(op=="angularVelocity")return Vec(c,world.Physics().GetAngularVelocity(body));
    glm::vec3 v;if(!ReadVec(c,arg(2),v))return JS_ThrowTypeError(c,"invalid vector");
    if(op=="force")world.Physics().ApplyForce(body,v);
    else if(op=="impulse")world.Physics().ApplyLinearImpulse(body,v);
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
void ScriptSystem::Stop(){m->Stop();}
void ScriptSystem::Synchronize(const std::vector<SceneObject>& objects){m->CheckThread();
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
void ScriptSystem::Frame(const InputSystem* input,float dt){m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    std::vector<std::pair<SceneObjectId,uint64_t>> order;for(const auto& entry:m->instances)order.push_back(entry.first);
    // Slot order follows authored vector order, not numeric slot identity.
    if(m->world){order.clear();for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts)if(m->instances.count({o.id,slot.id}))order.push_back({o.id,slot.id});}
    for(auto key:order){auto it=m->instances.find(key);if(it==m->instances.end()||(m->world&&!m->world->RuntimeDefinition(key.first)))continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"update");}}
void ScriptSystem::Fixed(const InputSystem* input,float dt){m->CheckThread();m->input=input;m->delta=dt;m->fixed=true;
    auto objects=m->world?m->world->ScriptObjects():std::vector<SceneObject>{};
    for(const auto& o:objects)for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end()||!m->world->RuntimeDefinition(o.id))continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"fixedUpdate");}}
std::vector<ScriptStateRecord> ScriptSystem::Capture()const{m->CheckThread();std::vector<ScriptStateRecord> result;
    for(const auto& entry:m->instances){const auto& i=entry.second;if(i.fault||(m->world&&!m->world->RuntimeDefinition(i.entity)))continue;auto value=JS_GetPropertyStr(m->ctx,i.value,"state");std::string text,error;
        if(m->Json(value,text,error))result.push_back({i.entity,i.slot.id,text});else {result.push_back({i.entity,i.slot.id,""});std::fprintf(stderr,"script state %llu/%llu: %s\n",(unsigned long long)i.entity,(unsigned long long)i.slot.id,error.c_str());}JS_FreeValue(m->ctx,value);}
    return result;}
bool ScriptSystem::Restore(const std::vector<ScriptStateRecord>& records,std::string& error){m->CheckThread();for(const auto& r:records)if(!ValidateJson(r.json,error))return false;
    for(const auto& r:records){auto key=std::make_pair(r.entity,r.slot);m->restored[key]=r.json;if(auto it=m->instances.find(key);it!=m->instances.end())JS_SetPropertyStr(m->ctx,it->second.value,"state",JS_ParseJSON(m->ctx,r.json.data(),r.json.size(),"saved state"));}return true;}
bool ScriptSystem::ValidateJson(const std::string& text,std::string& error){if(text.size()>65536){error="JSON exceeds 64KiB";return false;}Impl vm(nullptr,nullptr);vm.polls=0;auto value=JS_ParseJSON(vm.ctx,text.data(),text.size(),"JSON");if(JS_IsException(value)){error=Exception(vm.ctx);return false;}std::string canonical;bool ok=JS_IsObject(value)&&vm.Json(value,canonical,error);if(!ok&&error.empty())error="JSON root must be object";JS_FreeValue(vm.ctx,value);return ok;}
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

void ScriptSystem::UIFrame(const InputSystem* input,float dt){m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end())continue;auto& i=it->second;if(!i.started){i.started=true;m->Callback(i,"start");}m->Callback(i,"uiUpdate");}
}
void ScriptSystem::UIEvents(const InputSystem* input,float dt){m->CheckThread();m->input=input;m->delta=dt;m->fixed=false;
    if(!m->world->UIIfLoaded())return;
    auto events=m->world->UI().TakeEvents();
    for(const auto& event:events)for(const auto& o:m->world->ScriptObjects())for(const auto& slot:o.scripts){auto it=m->instances.find({o.id,slot.id});if(it==m->instances.end()||it->second.fault||!m->world->RuntimeDefinition(o.id))continue;
        auto& i=it->second;m->polls=0;m->currentOwner=i.entity;m->currentSlot=i.slot.id;auto fn=JS_GetPropertyStr(m->ctx,i.value,"onUI");if(JS_IsException(fn)){m->Error(i,"onUI");JS_FreeValue(m->ctx,fn);continue;}if(JS_IsFunction(m->ctx,fn)){
            auto e=JS_NewObject(m->ctx);JS_SetPropertyStr(m->ctx,e,"document",JS_NewString(m->ctx,event.document.c_str()));JS_SetPropertyStr(m->ctx,e,"element",JS_NewString(m->ctx,event.element.c_str()));JS_SetPropertyStr(m->ctx,e,"type",JS_NewString(m->ctx,event.type.c_str()));JS_SetPropertyStr(m->ctx,e,"value",JS_NewFloat64(m->ctx,event.value));
            auto result=JS_Call(m->ctx,fn,i.value,1,&e);if(JS_IsException(result))m->Error(i,"onUI");else if(JS_PromiseState(m->ctx,result)!=JS_PROMISE_NOT_A_PROMISE){JS_ThrowTypeError(m->ctx,"async UI callback unsupported");m->Error(i,"onUI");}JS_FreeValue(m->ctx,result);JS_FreeValue(m->ctx,e);
        }JS_FreeValue(m->ctx,fn);
    }
}
