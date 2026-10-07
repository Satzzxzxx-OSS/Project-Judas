#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include "ScriptSystem.h"
#include "WorldStreaming.h"
#include "SceneSerialization.h"
#include "JobSystem.h"
#include <thread>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace{unsigned checks=0;void require(bool v,const char* label){++checks;if(!v)throw std::runtime_error(label);}}
int main(){try{
 namespace fs=std::filesystem;const auto root=fs::absolute("build/post-consumer-metadata-fixture");fs::remove_all(root);fs::create_directories(root/"Assets");
 std::ofstream(root/"Assets/ref.js")<<R"JS(export const properties={ref:{type:'entity',default:null},label:{type:'string',default:'2'},number:{type:'number',default:2}};export default class{constructor({properties}){this.state={ticks:0,value:properties.ref?.id??null};}fixedUpdate(){this.state.ticks++;}})JS";
 AssetDatabase db;db.Scan(root.string(),(root/"Assets").string());AssetRecord asset;std::string error;require(db.Track((root/"Assets/ref.js").string(),asset,error),"registered schema");ResourceManager resources(nullptr,&db);
 Scene scene;auto& owner=scene.CreateObject("owner");auto id=owner.id;owner.scripts={{1,asset.id,true,"{\"ref\":{\"entity\":\"2\"}}"},{2,asset.id,true,"{}"}};
 for(int i=0;i<4000;++i)scene.CreateObject("inert");RuntimeWorld world;require(world.Build(scene,&resources,error),"indexed world build");world.FixedScripts(nullptr,.016f);
 require(world.ScriptObjects().size()==4001&&world.ScriptObjects(true).size()==1,"all-object enumeration preserved; maintained script membership");
 auto slot=scene.Find(id)->scripts[0];auto refs=world.Scripts()->DeclaredReferences(id,slot);require(refs&&*refs==std::vector<SceneObjectId>{2},"only schema-declared entity field counts");
 refs=world.Scripts()->DeclaredReferences(id,scene.Find(id)->scripts[1]);require(refs&&refs->empty(),"ordinary numeric/string defaults are not references");
 auto before=world.MetadataStats();for(int i=0;i<100;++i)world.FixedScripts(nullptr,.016f);auto after=world.MetadataStats();require(after.definitionsCopied-before.definitionsCopied<=500,"inert scenery not copied into script phases");require(after.indexRebuilds==before.indexRebuilds,"stable indexed lookups do not rebuild");
 auto objects=world.ScriptObjects(true);objects[0].scripts[0].properties="{\"ref\":{\"entity\":\"3\"}}";world.Scripts()->Synchronize(objects);refs=world.Scripts()->DeclaredReferences(id,objects[0].scripts[0]);require(refs&&*refs==std::vector<SceneObjectId>{3},"slot replacement refreshes validated references");world.Scripts()->Fixed(nullptr,.016f);auto records=world.Scripts()->Capture();require(records[0].json.find("\"value\":\"3\"")!=std::string::npos,"replacement gets independent new instance data");
 objects[0].scripts[0].enabled=false;world.Scripts()->Synchronize(objects);require(world.Scripts()->Capture().size()==1,"disabled slot retired once");
 require(world.DestroyEntity(2)&&!world.RuntimeDefinition(2),"destroyed target invalidated despite index");
 ScriptSystem::PropertyEntities("{}");auto construction=ScriptSystem::MetadataStats().parserConstructions;
 bool referencesCorrect=true;for(int i=0;i<1000;++i){referencesCorrect &= ScriptSystem::PropertyEntities("{\"ref\":{\"entity\":\"3\"},\"label\":\"3\",\"n\":3}")==std::vector<SceneObjectId>{3};}require(referencesCorrect,"tagged JSON references only");
 require(ScriptSystem::MetadataStats().parserConstructions==construction,"repeated inspection constructs zero VM/context");require(ScriptSystem::PropertyEntities("bad").empty()&&ScriptSystem::PropertyEntities("{}").empty(),"invalid JSON does not poison reusable parser");
 auto remapped=ScriptSystem::RemapPropertyEntities("{\"ref\":{\"entity\":\"3\"}}",{{3,4}});require(ScriptSystem::PropertyEntities(remapped)==std::vector<SceneObjectId>{4},"prefab/duplication remapping uses same tagged contract");

 // The normal residency coordinator must retain typed references and release
 // them after supported definition/lifetime changes, not just parse JSON.
 Project project;require(Project::CreateNew(root.string(),"Reference churn",project,error),"ordinary reference project");
 Scene region;region.CreateObject("target");region.CreateObject("retained target");require(SaveSceneToFile(region,(root/"Scenes/region.judas").string(),error),"region source");
 JobSystem jobs(2);ResourceManager regionResources(nullptr,&db,&jobs);RuntimeWorld resident;Scene empty;require(resident.Build(empty,&regionResources,error),"reference residency world");
 WorldManifest manifest;WorldRegion part;part.id="a";part.scene="Scenes/region.judas";manifest.regions={{"a",part}};WorldRegion second=part;second.id="b";manifest.regions["b"]=second;WorldStreaming stream(resident,regionResources,project,manifest);
 auto pump=[&](const auto& done){for(int i=0;i<3000;++i){resident.FixedScripts(nullptr,.016f);stream.Advance(false);if(done())return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false;};
 auto demand=stream.Request("a",false,error);auto other=stream.Request("b",false,error);require(pump([&]{return stream.Status(demand)->state=="active"&&stream.Status(other)->state=="active";}),"two regions published");
 auto target=stream.Resolve("a",1);SceneObject external;external.scripts={{1,asset.id,true,"{\"ref\":{\"entity\":\""+std::to_string(target)+"\"}}"},{2,asset.id,true,"{}"}};auto ownerId=resident.CreateEntity(external,nullptr,&error);require(ownerId!=0,"ordinary external multi-slot owner");resident.FixedScripts(nullptr,.016f);stream.Release(demand);stream.Advance(false);
 auto status=stream.Regions()[0];require(status.state=="active"&&std::find(status.pins.begin(),status.pins.end(),"external script entity property")!=status.pins.end(),"real declared reference pins and reports its reason");
 auto vmBefore=ScriptSystem::MetadataStats().parserConstructions;for(int i=0;i<100;++i)stream.Advance(false);require(ScriptSystem::MetadataStats().parserConstructions==vmBefore,"steady multi-region pins construct no VM/context");
 require(stream.Adopt(target,"b",error),"target adoption uses existing owner identity");stream.Advance(false);require(pump([&]{return stream.Regions()[0].state=="unloaded";}),"pin follows target adoption, old owner releases");stream.Release(other);stream.Advance(false);require(stream.Regions()[1].state=="active","new target owner remains pinned");
 require(stream.Adopt(target,"root",error),"adopted traveller explicitly returned to root");auto bTarget=stream.Resolve("b",1);auto* definition=const_cast<SceneObject*>(resident.RuntimeDefinition(ownerId));definition->scripts[0].properties="{\"ref\":{\"entity\":\""+std::to_string(bTarget)+"\"}}";resident.FixedScripts(nullptr,.016f);stream.Advance(false);require(stream.Regions()[1].state=="active","replacement references actual second-region target");definition->scripts[0].properties="{}";resident.FixedScripts(nullptr,.016f);require(pump([&]{return stream.Regions()[1].state=="unloaded";}),"authored property replacement releases typed demand");
 demand=stream.Request("a",false,error);require(pump([&]{return stream.Status(demand)->state=="active";}),"reference region revisits");auto returned=stream.Resolve("a",2);require(returned&&returned!=target,"revisit does not alias retired target identity");
 definition=const_cast<SceneObject*>(resident.RuntimeDefinition(ownerId));definition->scripts[0].properties="{\"ref\":{\"entity\":\""+std::to_string(returned)+"\"}}";resident.FixedScripts(nullptr,.016f);stream.Release(demand);stream.Advance(false);require(stream.Regions()[0].state=="active","replacement pins fresh target");resident.DestroyEntity(returned);require(pump([&]{return stream.Regions()[0].state=="unloaded";}),"destroyed target removes dependency safely");resident.DestroyEntity(ownerId);
 world.Destroy();resources.Shutdown();fs::remove_all(root);std::cout<<"Metadata regressions "<<checks<<" checks PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
