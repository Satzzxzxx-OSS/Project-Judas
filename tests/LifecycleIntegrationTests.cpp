// FTFT6: one ordinary serialized non-planetary scene, real RuntimeWorld/session
// stepping, mixed lifecycle history, generation reuse and the production load
// helper shared by standalone and editor. No alternate simulation loop.
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "GameSession.h"
#include "RuntimeWorld.h"
#include "SceneSerialization.h"
#include "Simulation.h"
#include "SimulationTiming.h"
#include "Window.h"
#include "WorldState.h"
namespace {
namespace fs=std::filesystem;
int checks=0,failures=0,steps=0;
void Check(bool value,const std::string& label) {++checks;std::cout<<(value?"PASS ":"FAIL ")<<label<<'\n';failures+=!value;}
bool Same(const EntityPhysicalState& a,const EntityPhysicalState& b) {
    return a.position==b.position&&a.rotation==b.rotation&&a.linearVelocity==b.linearVelocity&&a.angularVelocity==b.angularVelocity;
}
bool Near(const EntityPhysicalState& a,const EntityPhysicalState& b,float tolerance=2e-5f) {
    return glm::length(a.position-b.position)<=tolerance&&glm::length(a.linearVelocity-b.linearVelocity)<=tolerance&&
           glm::length(a.angularVelocity-b.angularVelocity)<=tolerance&&glm::length(a.rotation-b.rotation)<=tolerance;
}
std::string Read(const fs::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void Write(const fs::path& p,const std::string& text){std::ofstream out(p,std::ios::binary);out<<text;Check(bool(out),"write "+p.filename().string());}
struct Played {
    RuntimeWorld world;GameSession session;Window window;
    bool Begin(const Scene& scene){std::string error;window.SetTestInputMode(true);const bool ok=world.Build(scene,nullptr,error)&&session.Begin(world,error);Check(ok,"ordinary runtime/session construction: "+error);return ok;}
    void Advance(int count){for(int i=0;i<count;++i){StepPlayedWorld(session,window,SimulationTiming::kFixedTimestep);++steps;}}
};
BodyHandle Handle(RuntimeWorld& w,EntityId id){const auto* e=w.FindEntity(id);return e?w.DynamicBodies()[e->slot].Handle():BodyHandle{};}
EntityPhysicalState State(RuntimeWorld& w,EntityId id){EntityPhysicalState s;Check(w.GetEntityState(id,s),"read stable entity "+std::to_string(id));return s;}
std::string Snapshot(const RuntimeWorld& w){
    std::ostringstream out;out<<std::hexfloat<<w.EntityVersion()<<':'<<w.NextRuntimeEntityId()<<':'<<w.SimulationTimeSeconds()<<'\n';
    std::string delta;SaveWorldStateToString(CaptureWorldState(w),delta);out<<delta;
    for(const auto& e:w.Entities()){
        out<<e.id<<':'<<int(e.lifecycle)<<':'<<int(e.fidelity)<<':'<<e.slot<<':'<<e.reconstructions<<':'<<e.coarseStepsSimulated<<':'<<e.dormantSinceSeconds<<':'<<w.DynamicBodies()[e.slot].Handle().id<<'\n';
    }
    for(auto h:w.Physics().AliveBodies()){
        const auto t=w.Physics().GetTransform(h);const auto v=w.Physics().GetLinearVelocity(h),a=w.Physics().GetAngularVelocity(h);
        out<<h.id<<':'<<t.position.x<<','<<t.position.y<<','<<t.position.z<<','<<t.rotation.w<<','<<t.rotation.x<<','<<t.rotation.y<<','<<t.rotation.z<<','<<v.x<<','<<v.y<<','<<v.z<<','<<a.x<<','<<a.y<<','<<a.z<<'\n';
    }
    return out.str();
}
Scene TinyScene(std::array<EntityId,4>& ids,EntityId& doorId,EntityId& switchId){
    Scene scene;scene.Settings().name="FTFT6 ordinary mixed lifecycle";
    auto& start=scene.CreateObject("Start");start.transform.position={0,2,20};start.playerStart=ScenePlayerStartComponent{};
    for(int i=0;i<4;++i){auto& o=scene.CreateObject("Movable "+std::to_string(i));ids[i]=o.id;o.transform.position={float(4*i),3,0};
        o.body=SceneBodyComponent{};o.body->motion=SceneBodyMotion::Dynamic;o.body->shape=SceneShape::Sphere;o.body->radius=.25f;o.body->mass=2;o.body->friction=0;o.body->restitution=0;o.body->managed=true;o.body->initialLinearVelocity={.25f,.1f,.125f};}
    auto& door=scene.CreateObject("Door");doorId=door.id;door.transform.position={30,3,0};door.render=SceneRenderComponent{};door.door=SceneDoorComponent{};
    auto& lever=scene.CreateObject("Switch");switchId=lever.id;lever.transform.position={40,3,0};lever.render=SceneRenderComponent{};lever.lightSwitch=SceneLightSwitchComponent{};
    return scene;
}
void Exercise(const fs::path& directory){
    std::array<EntityId,4> ids{};EntityId doorId=0,switchId=0;Scene authored=TinyScene(ids,doorId,switchId);std::string error;
    const auto scenePath=directory/"tiny.judas",savePath=directory/"mixed.judasstate";
    Check(SaveSceneToFile(authored,scenePath.string(),error),"serialize ordinary authored scene: "+error);const std::string authoredBytes=Read(scenePath);
    Scene loaded;Check(LoadSceneFromFile(scenePath.string(),loaded,error),"reload ordinary authored scene: "+error);
    Played mixed,reference;if(!mixed.Begin(loaded)||!reference.Begin(loaded))return;
    Check(mixed.world.GetFidelityPolicy()==nullptr,"tiny game installs no distance/planet policy");
    Check(mixed.world.PointMassSources().empty(),"tiny game has no celestial sources");
    for(Played* played:{&mixed,&reference})for(auto id:ids)played->world.Physics().SetAngularVelocity(Handle(played->world,id),{.2f,-.3f,.4f});
    mixed.Advance(12);reference.Advance(12);
    const BodyHandle first=Handle(mixed.world,ids[0]);const EntityPhysicalState beforeCoarse=State(mixed.world,ids[0]);
    Check(mixed.world.SetEntityFidelity(ids[0],SimulationFidelity::Coarse,&error),"moving Full -> Coarse: "+error);
    Check(mixed.world.FindEntity(ids[0])->lifecycle==EntityLifecycle::Active&&mixed.world.FindEntity(ids[0])->coarseMotion==CoarseMotion::Inertial,"moving Coarse is Active and inertial");
    Check(!mixed.world.Physics().IsDynamicBody(first),"demoted body's old physics generation is invalid");
    mixed.Advance(30);reference.Advance(30);
    const auto coarse=State(mixed.world,ids[0]);
    const glm::dvec3 oracle=glm::dvec3(beforeCoarse.position)+glm::dvec3(beforeCoarse.linearVelocity)*(30.*double(SimulationTiming::kFixedTimestep));
    Check(glm::length(glm::dvec3(coarse.position)-oracle)<2e-5,"coarse translation matches independent zero-gravity v*t oracle");
    Check(Near(coarse,State(reference.world,ids[0])),"moving Coarse agrees with never-demoted full world");
    const auto sleeping=State(mixed.world,ids[1]);const BodyHandle stale=Handle(mixed.world,ids[1]);
    Check(mixed.world.SetEntityFidelity(ids[1],SimulationFidelity::Dormant,&error),"moving Full -> Dormant: "+error);
    Check(mixed.world.FindEntity(ids[1])->lifecycle==EntityLifecycle::Unloaded,"Dormant is Unloaded, not destroyed");
    // Force actual free-slot reuse through the ordinary runtime creator. No
    // cached pointers are kept across entity/body-vector growth.
    SceneObject definition;definition.name="Created survivor";definition.body=SceneBodyComponent{};definition.body->motion=SceneBodyMotion::Dynamic;definition.body->shape=SceneShape::Sphere;definition.body->radius=.25f;definition.body->mass=3;
    EntityPhysicalState createdState;createdState.position={20,4,0};createdState.linearVelocity={-.2f,.05f,.1f};createdState.angularVelocity={-.1f,.2f,.3f};
    const EntityId created=mixed.world.CreateEntity(definition,&createdState,&error);Check(created>=kRuntimeEntityIdBase,"runtime creation obtains non-authored ID: "+error);
    const BodyHandle fresh=Handle(mixed.world,created);
    // This test-only storage assertion mirrors the existing broadphase reuse
    // oracle; engine clients continue treating handles as opaque.
    Check((fresh.id&0xFFFFFu)==(stale.id&0xFFFFFu)&&fresh.id!=stale.id,"fixture genuinely reuses a physics slot with a new generation");
    const auto freshBefore=State(mixed.world,created);
    mixed.world.Physics().ApplyLinearImpulse(stale,{100,200,300});mixed.world.Physics().SetAngularVelocity(stale,{7,8,9});
    mixed.world.Physics().ResetBody(stale,{99,99,99},glm::quat(1,0,0,0));mixed.world.Physics().DestroyBody(stale);
    Check(Same(State(mixed.world,created),freshBefore)&&mixed.world.Physics().IsDynamicBody(fresh),"stale mutation/reset/destruction cannot reach the reused occupant");
    Check(mixed.world.EntityIdOfBody(stale)==0&&mixed.world.EntityIdOfBody(fresh)==created,"body-to-entity lookup follows live generation only");
    mixed.Advance(15);reference.Advance(15);
    Check(Same(State(mixed.world,ids[1]),sleeping),"dormant moving state remains bit-exact while others advance");
    const auto beforePromotion=State(mixed.world,ids[0]);
    Check(mixed.world.SetEntityFidelity(ids[0],SimulationFidelity::Full,&error),"Coarse -> Full reconstructs: "+error);
    Check(Same(State(mixed.world,ids[0]),beforePromotion),"reconstruction preserves moving pose and both velocities exactly");
    Check(Handle(mixed.world,ids[0]).id!=first.id&&mixed.world.FindEntity(ids[0])->reconstructions==1,"same entity receives one new live incarnation");
    Check(Near(State(mixed.world,ids[0]),State(reference.world,ids[0])),"mixed fidelity state matches independent full trajectory");
    Check(mixed.world.SetEntityFidelity(ids[0],SimulationFidelity::Coarse,&error),"leave moving entity Coarse for persistence");
    Check(mixed.world.SetEntityFidelity(created,SimulationFidelity::Dormant,&error),"unload runtime-created survivor");
    Check(mixed.world.DestroyEntity(ids[2],&error),"permanently destroy authored body");
    const EntityId transient=mixed.world.CreateEntity(definition,&createdState,&error);Check(transient>created,"second runtime identity is monotonic");
    Check(mixed.world.DestroyEntity(transient,&error),"destroy second runtime-created entity");
    Check(!mixed.world.SetEntityFidelity(ids[2],SimulationFidelity::Full,&error)&&!error.empty(),"permanent destruction rejects reconstruction");
    mixed.world.FindDoor(doorId)->SetOpen(true);mixed.world.FindLightSwitch(switchId)->SetLampOn(true);
    const auto counts=mixed.world.CountLifecycle();Check(counts.full==1&&counts.coarse==1&&counts.dormant==2&&counts.destroyed==2,"save fixture simultaneously contains all fidelity/lifecycle states");
    const std::array<EntityId,4> surviving{{ids[0],ids[1],ids[3],created}};std::array<EntityPhysicalState,4> saved{};
    for(std::size_t i=0;i<surviving.size();++i)saved[i]=State(mixed.world,surviving[i]);
    Check(SaveWorldStateToFile(CaptureWorldState(mixed.world),savePath.string(),error),"save mixed lifecycle deltas through production serializer: "+error);
    const std::string saveBytes=Read(savePath);Check(Read(scenePath)==authoredBytes,"runtime transitions never rewrite authored scene file");
    Played restored;if(!restored.Begin(loaded))return;bool applied=false;
    Check(ApplyWorldStateFileIfPresent(restored.world,savePath.string(),applied,error)&&applied,"standalone/editor shared load helper restores mixed history: "+error);
    for(std::size_t i=0;i<surviving.size();++i){Check(Near(State(restored.world,surviving[i]),saved[i],1e-6f),"reloaded entity preserves retained pose/velocities "+std::to_string(surviving[i]));
        Check(restored.world.FindEntity(surviving[i])->lifecycle==EntityLifecycle::Active&&restored.world.FindEntity(surviving[i])->fidelity==SimulationFidelity::Full,"fidelity is reselected by ordinary no-policy construction, not persisted");}
    Check(restored.world.FindEntity(ids[2])->lifecycle==EntityLifecycle::Destroyed,"authored destruction survives reload");
    Check(restored.world.FindEntity(transient)==nullptr&&restored.world.NextRuntimeEntityId()>transient,"destroyed runtime creation stays absent without ID reuse");
    Check(!restored.world.SetEntityFidelity(ids[2],SimulationFidelity::Full,&error),"destroyed authored identity remains non-reconstructible after reload");
    Check(restored.world.FindDoor(doorId)->IsOpen()&&restored.world.FindLightSwitch(switchId)->IsLampOn(),"door/switch deltas survive the same mixed reload");
    Check(Read(savePath)==saveBytes,"successful application does not rewrite existing save");
    const auto next=restored.world.CreateEntity(definition,&createdState,&error);Check(next>transient,"new runtime entity after mixed reload cannot recycle any consumed identity");
    // A populated world with its own reductions/runtime creation must remain
    // exactly unchanged when the same-named incompatible baseline rejects a save.
    Scene changed=loaded;changed.Find(ids[0])->body->mass=4;
    Check(SaveSceneToFile(changed,(directory/"incompatible-same-name.judas").string(),error),"write same-name changed-component authored fixture");
    Played incompatible;if(!incompatible.Begin(changed))return;
    incompatible.Advance(7);incompatible.world.SetEntityFidelity(ids[1],SimulationFidelity::Dormant);incompatible.world.SetEntityFidelity(ids[0],SimulationFidelity::Coarse);
    Check(incompatible.world.CreateEntity(definition,&createdState,&error)>=kRuntimeEntityIdBase,"incompatible destination already contains a runtime creation");
    const std::string beforeReject=Snapshot(incompatible.world);applied=true;
    const bool incompatibleApplied=ApplyWorldStateFileIfPresent(incompatible.world,savePath.string(),applied,error);
    Check(!incompatibleApplied&&error.find("baseline fingerprint mismatch")!=std::string::npos,"same-name changed baseline rejects mixed save usefully: "+error);
    Check(Snapshot(incompatible.world)==beforeReject,"failed load leaves handles/live state/reductions/identities/counters unchanged");
    Check(Read(savePath)==saveBytes,"failed load leaves original save bytes unchanged");
    Check(Read(scenePath)==authoredBytes,"all replay/rejection work leaves authored baseline file unchanged");
    Write(directory/"restored.snapshot.txt",Snapshot(restored.world));Write(directory/"rejected-before.snapshot.txt",beforeReject);Write(directory/"rejected-after.snapshot.txt",Snapshot(incompatible.world));
}
}
int main(int argc,char**argv){
    const auto begin=std::chrono::steady_clock::now();fs::path directory;
    if(argc==3&&std::string(argv[1])=="--output")directory=argv[2];
    else if(argc==1)directory=fs::temp_directory_path()/("judas-ftft6-lifecycle-"+std::to_string(begin.time_since_epoch().count()));
    else {std::cerr<<"usage: judas_lifecycle_integration_tests [--output fresh-directory]\n";return 2;}
    if(fs::exists(directory)&&!fs::is_empty(directory)){std::cerr<<"Refusing to overwrite lifecycle evidence\n";return 2;}
    fs::create_directories(directory);Exercise(directory);
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::ostringstream result;result<<std::setprecision(17)<<"{\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"fixed_steps\":"<<steps<<",\"seconds\":"<<elapsed<<",\"pass\":"<<(failures?"false":"true")<<"}\n";
    std::ofstream(directory/"results.json")<<result.str();std::cout<<result.str();return failures?1:0;
}
