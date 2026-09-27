// FTFT1: production persistence compatibility, complete preflight, and failure atomicity.
// Every application-path load uses the same ApplyWorldStateFileIfPresent helper as
// Application::Run and EditorApplication::StartPlay. No alternate persistence path.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneSerialization.h"
#include "WorldState.h"

namespace {
int g_checks = 0, g_failures = 0;
void Check(bool condition, const std::string& label) {
    ++g_checks;
    std::printf("  %s %s\n", condition ? "PASS" : "FAIL", label.c_str());
    if (!condition) ++g_failures;
}
Scene Baseline() {
    Scene scene;
    scene.Settings().name = "FTFT1 same scene name";
    for (int i = 0; i < 3; ++i) {
        auto& o = scene.CreateObject("Body " + std::to_string(i));
        o.transform.position = glm::vec3(float(i * 3), 2, 0);
        o.body = SceneBodyComponent{};
        o.body->motion = SceneBodyMotion::Dynamic;
        o.body->managed = true;
        if (i == 2) o.vehicle = SceneVehicleComponent{};
    }
    auto& door = scene.CreateObject("Door");
    door.transform.position = glm::vec3(12, 2, 0);
    door.render = SceneRenderComponent{};
    door.door = SceneDoorComponent{};
    auto& lever = scene.CreateObject("Switch");
    lever.transform.position = glm::vec3(15, 2, 0);
    lever.render = SceneRenderComponent{};
    lever.lightSwitch = SceneLightSwitchComponent{};
    auto& staticObject = scene.CreateObject("Static authored object");
    staticObject.body = SceneBodyComponent{};
    staticObject.transform.position = glm::vec3(0, -2, 0);
    return scene;
}
bool Build(RuntimeWorld& world, const Scene& scene) {
    std::string error;
    const bool result = world.Build(scene, nullptr, error);
    Check(result, "build runtime world: " + error);
    return result;
}
void Vec(std::ostream& out, const glm::vec3& v) { out << v.x << ',' << v.y << ',' << v.z << ';'; }
void State(std::ostream& out, const EntityPhysicalState& s) {
    Vec(out, s.position);
    out << s.rotation.w << ',' << s.rotation.x << ',' << s.rotation.y << ',' << s.rotation.z << ';';
    Vec(out, s.linearVelocity);
    Vec(out, s.angularVelocity);
}
// Exact before/after observation includes live state, handle identity, lifecycle,
// reconstruction bookkeeping, definition, counters, version and interactables.
std::string Snapshot(const RuntimeWorld& world) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::hexfloat << world.NextRuntimeEntityId() << ':' << world.EntityVersion() << ':'
        << world.SimulationTimeSeconds() << ':' << world.Physics().AliveBodyCount() << ':'
        << world.Physics().DynamicBodyCount() << '\n';
    const auto counts = world.CountLifecycle();
    out << counts.full << ',' << counts.coarse << ',' << counts.dormant << ',' << counts.destroyed << ','
        << counts.physicsBodies << ',' << counts.dynamicPhysicsBodies << ',' << counts.transitionsThisStep << '\n';
    for (const auto& e : world.Entities()) {
        out << e.id << ':' << e.name << ':' << e.authored << ':' << int(e.lifecycle) << ':' << int(e.fidelity)
            << ':' << int(e.coarseMotion) << ':' << e.slot << ':' << e.reconstructions << ':'
            << e.coarseStepsSimulated << ':' << e.dormantSinceSeconds << ':';
        if (e.forcedFidelity) out << int(*e.forcedFidelity);
        out << ':' << world.DynamicBodies()[e.slot].Handle().id << ':' << world.DynamicBodies()[e.slot].IsLive();
        EntityPhysicalState physical;
        out << ':' << world.GetEntityState(e.id, physical) << ':';
        State(out, physical);
        State(out, e.state);
        std::string definition;
        WriteSceneObjectBlock(e.definition, definition);
        out << definition;
    }
    for (const auto& door : world.Doors()) out << "door:" << door.IsOpen() << ':' << door.GetCurrentAngleRadians();
    for (const auto& lever : world.LightSwitches()) out << "switch:" << lever.IsLampOn() << ':' << lever.GetCurrentAngleRadians();
    return out.str();
}
WorldState EmptyDelta(RuntimeWorld& world) {
    WorldState delta = CaptureWorldState(world);
    delta.entities.clear();
    delta.interactables.clear();
    return delta;
}
WorldStateEntityChange Moved(RuntimeWorld& world, EntityId id = 1) {
    WorldStateEntityChange change;
    change.id = id;
    world.GetEntityState(id, change.state);
    change.state.position += glm::vec3(1, 2, 3);
    change.state.linearVelocity = glm::vec3(4, -5, 6);
    change.state.angularVelocity = glm::vec3(0.2f, -0.3f, 0.4f);
    return change;
}
WorldStateEntityChange Created(EntityId id = kRuntimeEntityIdBase) {
    WorldStateEntityChange change;
    change.created = true;
    change.id = change.definition.id = id;
    change.definition.name = "Runtime-created body";
    change.definition.body = SceneBodyComponent{};
    change.definition.body->motion = SceneBodyMotion::Dynamic;
    change.state.position = glm::vec3(21, 3, -1);
    return change;
}
void Reject(RuntimeWorld& world, const WorldState& delta, const std::string& label) {
    const std::string before = Snapshot(world);
    std::string error;
    const bool applied = ApplyWorldState(world, delta, error);
    Check(!applied && !error.empty(), label + " rejects usefully: " + error);
    Check(Snapshot(world) == before, label + " leaves exact observable live state unchanged");
}
std::string Read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}
void Write(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out << bytes;
    Check(bool(out), "write hostile save fixture");
}
std::string Replace(std::string text, const std::string& from, const std::string& to) {
    const auto at = text.find(from);
    Check(at != std::string::npos, "malformed fixture mutation finds " + from);
    if (at != std::string::npos) text.replace(at, from.size(), to);
    return text;
}
std::string ReplaceLine(std::string text, const std::string& prefix, const std::string& line) {
    const auto at = text.find(prefix);
    Check(at != std::string::npos, "fixture locates directive " + prefix);
    if (at != std::string::npos) text.replace(at, text.find('\n', at) - at, line);
    return text;
}
void Compatibility() {
    std::puts("A: exact authored baseline compatibility");
    const Scene original = Baseline();
    RuntimeWorld source;
    if (!Build(source, original)) return;
    WorldState delta = CaptureWorldState(source);
    delta.entities.push_back(Moved(source));
    std::string serialized, error;
    Check(SaveWorldStateToString(delta, serialized), "save compatible delta");
    WorldState parsed;
    Check(LoadWorldStateFromString(serialized, parsed, error), "load compatible delta: " + error);
    RuntimeWorld same;
    if (Build(same, original)) {
        Check(ApplyWorldState(same, parsed, error), "same authored baseline accepts delta: " + error);
        EntityPhysicalState result;
        same.GetEntityState(1, result);
        Check(result.position == delta.entities.front().state.position, "compatible delta restores moved transform");
    }
    std::string authored;
    SaveSceneToString(original, authored);
    Scene roundtrip;
    Check(LoadSceneFromString(authored, roundtrip, error), "authored scene round-trip loads: " + error);
    RuntimeWorld roundtripWorld;
    if (Build(roundtripWorld, roundtrip))
        Check(ApplyWorldState(roundtripWorld, parsed, error), "canonical authored round-trip remains compatible: " + error);
    const std::vector<std::pair<std::string, std::function<void(Scene&)>>> mutations = {
        {"same name; changed transform", [](Scene& s) { s.Find(1)->transform.position.x = 99; }},
        {"same name; changed stable object ID", [](Scene& s) { s.Find(1)->id = 101; s.SetNextId(102); }},
        {"same name; changed body component", [](Scene& s) { s.Find(1)->body->mass = 7; }},
        {"same name; added component", [](Scene& s) { s.Find(1)->render = SceneRenderComponent{}; }},
        {"same name; removed component", [](Scene& s) { s.Find(5)->lightSwitch.reset(); }},
        {"same name; added object", [](Scene& s) { s.CreateObject("additional authored object"); }},
        {"same name; removed object", [](Scene& s) { s.DestroyObject(2); }},
        {"same name; changed absolute origin", [](Scene& s) { s.Settings().worldOrigin.x = 1e12; }},
        {"same name; changed lighting setting", [](Scene& s) { s.Settings().ambientColor.x = 0.9f; }},
        {"same name; changed next authored ID", [](Scene& s) { s.SetNextId(100); }},
        {"same name; changed authored order", [](Scene& s) { s.MoveObject(2, -1); }}
    };
    for (const auto& mutation : mutations) {
        Scene changed = original;
        mutation.second(changed);
        RuntimeWorld destination;
        if (Build(destination, changed)) Reject(destination, parsed, mutation.first);
    }
    // Runtime mutation must not alter the cached authored baseline identity.
    source.SetEntityState(1, delta.entities.front().state);
    source.SetEntityFidelity(2, SimulationFidelity::Dormant);
    const auto afterRuntimeChanges = CaptureWorldState(source);
    RuntimeWorld reload;
    if (Build(reload, original))
        Check(ApplyWorldState(reload, afterRuntimeChanges, error), "runtime motion and fidelity do not enter baseline fingerprint: " + error);
    std::string unchanged;
    SaveSceneToString(original, unchanged);
    Check(unchanged == authored, "authoring scene untouched by capture/load/runtime mutation");
}
void Atomicity() {
    std::puts("B: complete preflight before any live mutation");
    const Scene scene = Baseline();
    using Mutation = std::function<void(WorldState&, RuntimeWorld&)>;
    const std::vector<std::pair<std::string, Mutation>> invalid = {
        {"unknown entity reference", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w); c.id=999; s.entities.push_back(c); }},
        {"invalid zero entity reference", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w); c.id=0; s.entities.push_back(c); }},
        {"duplicate entity reference", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w); s.entities={c,c}; }},
        {"mixed valid destroy then non-destroyable vehicle", [](WorldState& s, RuntimeWorld&) { WorldStateEntityChange c; c.destroyed=true; c.id=1; s.entities.push_back(c); c.id=3; s.entities.push_back(c); }},
        {"mixed valid destroy then unsupported created component", [](WorldState& s, RuntimeWorld&) { WorldStateEntityChange c; c.id=1;c.destroyed=true;s.entities.push_back(c);auto n=Created();n.definition.gravity=SceneGravityComponent{};s.entities.push_back(n);s.nextRuntimeId=n.id+1; }},
        {"unknown door reference", [](WorldState& s, RuntimeWorld&) { s.interactables.push_back({999,true,true}); }},
        {"invalid zero switch reference", [](WorldState& s, RuntimeWorld&) { s.interactables.push_back({0,false,true}); }},
        {"wrong interactable type", [](WorldState& s, RuntimeWorld&) { s.interactables.push_back({4,false,true}); }},
        {"duplicate interactable reference", [](WorldState& s, RuntimeWorld&) { s.interactables={{4,true,true},{4,true,false}}; }},
        {"created contradictory flags", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.destroyed=true;s.entities.push_back(c);s.nextRuntimeId=c.id+1; }},
        {"created ID outside runtime range", [](WorldState& s, RuntimeWorld&) { s.entities.push_back(Created(123)); }},
        {"created definition ID mismatch", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.definition.id++;s.entities.push_back(c);s.nextRuntimeId=c.id+2; }},
        {"created mesh body cannot instantiate", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.definition.body->shape=SceneShape::Mesh;s.entities.push_back(c);s.nextRuntimeId=c.id+1; }},
        {"created invalid body mass", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.definition.body->mass=0;s.entities.push_back(c);s.nextRuntimeId=c.id+1; }},
        {"created invalid box extents", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.definition.body->halfExtents.x=-1;s.entities.push_back(c);s.nextRuntimeId=c.id+1; }},
        {"created nonfinite authored component", [](WorldState& s, RuntimeWorld&) { auto c=Created();c.definition.body->friction=std::numeric_limits<float>::quiet_NaN();s.entities.push_back(c);s.nextRuntimeId=c.id+1; }},
        {"nonfinite state position", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w);c.state.position.x=std::numeric_limits<float>::infinity();s.entities.push_back(c); }},
        {"nonfinite state velocity", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w);c.state.angularVelocity.z=std::numeric_limits<float>::quiet_NaN();s.entities.push_back(c); }},
        {"zero quaternion", [](WorldState& s, RuntimeWorld& w) { auto c=Moved(w);c.state.rotation=glm::quat(0,0,0,0);s.entities.push_back(c); }},
        {"next ID outside runtime range", [](WorldState& s, RuntimeWorld&) { s.nextRuntimeId=17; }},
        {"next ID would recycle created identity", [](WorldState& s, RuntimeWorld&) { s.entities.push_back(Created());s.nextRuntimeId=kRuntimeEntityIdBase; }},
        {"exhausted next ID", [](WorldState& s, RuntimeWorld&) { s.nextRuntimeId=std::numeric_limits<EntityId>::max(); }},
        {"unsupported in-memory save version", [](WorldState& s, RuntimeWorld&) { s.compatibility.formatVersion=999; }},
        {"unsupported fingerprint schema", [](WorldState& s, RuntimeWorld&) { s.compatibility.fingerprintVersion=999; }},
        {"unknown fingerprint algorithm", [](WorldState& s, RuntimeWorld&) { s.compatibility.algorithm="std-hash"; }},
        {"fingerprint mismatch", [](WorldState& s, RuntimeWorld&) { s.compatibility.baselineFingerprint=std::string(64,'0'); }},
        {"missing fingerprint", [](WorldState& s, RuntimeWorld&) { s.compatibility.baselineFingerprint.clear(); }}
    };
    for (const auto& test : invalid) {
        RuntimeWorld world;
        if (!Build(world, scene)) continue;
        world.SetEntityFidelity(2, SimulationFidelity::Dormant);
        world.FindDoor(4)->SetOpen(true);
        world.FindLightSwitch(5)->SetLampOn(true);
        WorldState delta=EmptyDelta(world);
        test.second(delta,world);
        Reject(world,delta,test.first);
    }
    RuntimeWorld destroyed;
    if (Build(destroyed,scene)) {
        auto moved=Moved(destroyed);
        destroyed.DestroyEntity(1);
        auto delta=EmptyDelta(destroyed);
        delta.entities.push_back(moved);
        Reject(destroyed,delta,"moved reference to already destroyed live entity");
    }
}
void AssetPreflight() {
    std::puts("C: asset-backed creation preflight has no world or resource side effects");
    AssetDatabase assets;
    ResourceManager resources(nullptr, &assets);
    RuntimeWorld world;
    std::string error;
    if (!world.Build(Baseline(), &resources, error)) { Check(false, "build asset-backed world: " + error); return; }
    WorldState delta=EmptyDelta(world);
    WorldStateEntityChange destroy;
    destroy.id=1; destroy.destroyed=true;
    delta.entities.push_back(destroy);
    auto creation=Created();
    creation.definition.render=SceneRenderComponent{};
    creation.definition.render->shape=SceneShape::Mesh;
    creation.definition.render->meshAsset=std::string(32,'a');
    delta.entities.push_back(creation);
    delta.nextRuntimeId=creation.id+1;
    const auto before=resources.Stats();
    Reject(world,delta,"valid destroy followed by missing mesh asset");
    const auto after=resources.Stats();
    Check(after.misses==before.misses&&after.loading==before.loading&&after.failed==before.failed,
          "failed asset validation makes no resource load requests");
}
void DeltaRoundTrip(const std::filesystem::path& directory) {
    std::puts("C: valid created/destroyed/moved/door/switch persistence through application load helper");
    const Scene baseline=Baseline();
    RuntimeWorld source;
    if (!Build(source,baseline)) return;
    const auto moved=Moved(source);
    source.SetEntityState(1,moved.state);
    source.DestroyEntity(2);
    source.FindDoor(4)->SetOpen(true);
    source.FindLightSwitch(5)->SetLampOn(true);
    auto creation=Created(); creation.definition.id=0;
    std::string error;
    const EntityId createdId=source.CreateEntity(creation.definition,&creation.state,&error);
    Check(createdId>=kRuntimeEntityIdBase,"create runtime persistent entity: "+error);
    // A deleted runtime creation has no authored referent: retaining its ID
    // allocator counter is sufficient and must not serialize an invalid tombstone.
    const EntityId transient=source.CreateEntity(creation.definition,&creation.state,&error);
    Check(source.DestroyEntity(transient,&error),"destroy runtime-created entity: "+error);
    const auto captured=CaptureWorldState(source);
    bool hasCreated=false,hasTransient=false;
    for(const auto& c:captured.entities){hasCreated|=c.id==createdId&&c.created;hasTransient|=c.id==transient;}
    Check(hasCreated&&!hasTransient,"capture keeps surviving runtime creation and omits unreferenced creation tombstone");
    const auto path=directory/"valid.judasstate";
    Check(SaveWorldStateToFile(captured,path.string(),error),"write valid save: "+error);
    const auto fileBefore=Read(path);
    RuntimeWorld destination;
    if (!Build(destination,baseline)) return;
    bool applied=false;
    Check(ApplyWorldStateFileIfPresent(destination,path.string(),applied,error)&&applied,"real application load helper applies compatible save: "+error);
    EntityPhysicalState physical;
    Check(destination.GetEntityState(1,physical)&&physical.position==moved.state.position&&physical.linearVelocity==moved.state.linearVelocity&&physical.angularVelocity==moved.state.angularVelocity,"moved position and both velocities restored");
    Check(destination.FindEntity(2)->lifecycle==EntityLifecycle::Destroyed,"authored destroyed entity remains absent");
    Check(destination.FindEntity(createdId)&&!destination.FindEntity(createdId)->authored,"created identity and provenance restored");
    Check(destination.FindDoor(4)->IsOpen()&&destination.FindLightSwitch(5)->IsLampOn(),"door and switch changes restored");
    Check(destination.NextRuntimeEntityId()==source.NextRuntimeEntityId()&&destination.NextRuntimeEntityId()>transient,"counter retains destroyed runtime identity history");
    Check(Read(path)==fileBefore,"successful load does not rewrite source save");
    const auto next=destination.CreateEntity(creation.definition,&creation.state,&error);
    Check(next>transient,"next creation cannot reuse destroyed creation identity");
    // Reduced-fidelity state is still persisted independently of residency.
    RuntimeWorld dormant;
    if(Build(dormant,baseline)) {
        dormant.SetEntityFidelity(1,SimulationFidelity::Dormant);
        dormant.SetEntityState(1,moved.state);
        auto delta=CaptureWorldState(dormant);
        RuntimeWorld fresh;
        if(Build(fresh,baseline)) {
            Check(ApplyWorldState(fresh,delta,error),"dormant physical-state delta reloads: "+error);
            Check(fresh.GetEntityState(1,physical)&&physical.position==moved.state.position,"dormant entity retains moved state on reconstruction");
        }
    }
}
void ExhaustedIdentityRoundTrip() {
    std::puts("E: the final allocatable identity remains persistable without overflow");
    RuntimeWorld source;
    if (!Build(source, Baseline())) return;
    const EntityId exhausted = static_cast<EntityId>(std::numeric_limits<std::int64_t>::max());
    source.SetNextRuntimeEntityId(exhausted - 1);
    auto definition = Created().definition;
    definition.id = 0;
    std::string error, text;
    Check(source.CreateEntity(definition, nullptr, &error) == exhausted - 1, "last allocatable ID is created");
    Check(SaveWorldStateToString(CaptureWorldState(source), text), "exhausted next-ID sentinel can be serialized");
    WorldState parsed;
    Check(LoadWorldStateFromString(text, parsed, error), "exhausted counter save parses: " + error);
    RuntimeWorld destination;
    if (!Build(destination, Baseline())) return;
    Check(ApplyWorldState(destination, parsed, error), "last-ID creation reloads: " + error);
    const std::string before = Snapshot(destination);
    Check(destination.CreateEntity(definition, nullptr, &error) == kInvalidSceneObjectId && !error.empty(),
          "exhausted allocator rejects further creation without wrapping");
    Check(Snapshot(destination) == before, "allocator exhaustion leaves world unchanged");
}
void FileValidation(const std::filesystem::path& directory) {
    std::puts("D: malformed/unverifiable saves leave world and original bytes unchanged");
    RuntimeWorld world;
    if(!Build(world,Baseline()))return;
    auto valid=CaptureWorldState(world);
    valid.entities.push_back(Moved(world));
    std::string text,error;
    Check(SaveWorldStateToString(valid,text),"serialize valid hostile-fixture seed");
    std::vector<std::pair<std::string,std::string>> cases;
    cases.push_back({"empty file",""});
    cases.push_back({"malformed header","this is not a save\n"});
    cases.push_back({"unverifiable legacy v1","JudasWorldState 1\nbaseline \"FTFT1 same scene name\"\nnext-runtime-id 4611686018427387904\n"});
    cases.push_back({"unsupported format",ReplaceLine(text,"JudasWorldState ","JudasWorldState 99")});
    cases.push_back({"missing compatibility record",ReplaceLine(text,"compatibility ","")});
    cases.push_back({"duplicate compatibility record",text+"compatibility 1 sha256 "+valid.compatibility.baselineFingerprint+"\n"});
    cases.push_back({"unknown fingerprint algorithm",Replace(text,"compatibility 1 sha256 ","compatibility 1 md5 ")});
    cases.push_back({"unsupported fingerprint version",Replace(text,"compatibility 1 sha256 ","compatibility 99 sha256 ")});
    cases.push_back({"duplicate baseline",text+"baseline \"FTFT1 same scene name\"\n"});
    cases.push_back({"duplicate next ID",text+"next-runtime-id 4611686018427387904\n"});
    cases.push_back({"duplicate entity",text+"entity 1 destroyed\n"});
    cases.push_back({"unknown entity",Replace(text,"entity 1 moved","entity 999 moved")});
    cases.push_back({"negative entity ID",Replace(text,"entity 1 moved","entity -1 moved")});
    cases.push_back({"overflow entity ID",Replace(text,"entity 1 moved","entity 184467440737095516160 moved")});
    cases.push_back({"nonfinite physical state",ReplaceLine(text,"  position ","  position nan 2 3")});
    cases.push_back({"zero quaternion",ReplaceLine(text,"  rotation ","  rotation 0 0 0 0")});
    cases.push_back({"embedded NUL physical field",ReplaceLine(text,"  position ",std::string("  position 1")+char(0)+"junk 2 3")});
    cases.push_back({"duplicate physical field",Replace(text,"end\n","  position 1 2 3\nend\n")});
    cases.push_back({"extra end token",Replace(text,"end\n","end stray\n")});
    cases.push_back({"truncated entity block",text.substr(0,text.find("end\n"))});
    cases.push_back({"unknown directive",text+"future-magic 99\n"});
    cases.push_back({"duplicate interactable",text+"door 4 true\ndoor 4 false\n"});
    cases.push_back({"zero interactable",text+"light-switch 0 true\n"});
    cases.push_back({"unknown interactable",text+"door 999 true\n"});
    cases.push_back({"counter overflow",ReplaceLine(text,"next-runtime-id ","next-runtime-id 184467440737095516160")});
    // Preserve valid syntax while corrupting the actual content fingerprint.
    cases.push_back({"fingerprint mismatch",Replace(text,valid.compatibility.baselineFingerprint,std::string(64,'0'))});
    auto createdState=EmptyDelta(world);
    createdState.entities.push_back(Created());
    createdState.nextRuntimeId=kRuntimeEntityIdBase+1;
    std::string createdText;
    Check(SaveWorldStateToString(createdState,createdText),"serialize valid created entity for nested-parser check");
    cases.push_back({"extra nested object end token",Replace(createdText,"end\n","end stray\n")});
    for (std::size_t i=0;i<cases.size();++i) {
        const auto path=directory/("invalid_"+std::to_string(i)+".judasstate");
        Write(path,cases[i].second);
        const auto bytesBefore=Read(path),worldBefore=Snapshot(world);
        bool applied=true;
        error.clear();
        const bool result=ApplyWorldStateFileIfPresent(world,path.string(),applied,error);
        Check(!result&&!applied&&!error.empty(),cases[i].first+" rejects through shared application helper: "+error);
        Check(Snapshot(world)==worldBefore,cases[i].first+" leaves live state/handles/identities unchanged");
        Check(Read(path)==bytesBefore,cases[i].first+" leaves existing save bytes unchanged");
    }
    bool applied=true;
    error="stale prior error";
    Check(ApplyWorldStateFileIfPresent(world,(directory/"absent.judasstate").string(),applied,error)&&!applied,"genuinely missing file means no saved state");
    applied=true;
    Check(!ApplyWorldStateFileIfPresent(world,directory.string(),applied,error)&&!applied,"existing directory is an error, not a missing save");
    // A failed parse must not replace the caller's already valid output object.
    WorldState parsed=valid;
    std::string before,after;
    SaveWorldStateToString(parsed,before);
    Check(!LoadWorldStateFromString("JudasWorldState 99\n",parsed,error),"invalid parse fails");
    SaveWorldStateToString(parsed,after);
    Check(after==before,"parse failure leaves prior output record unchanged");
    error="stale prior error";
    Check(LoadWorldStateFromString(text,parsed,error)&&error.empty(),"successful parse clears stale caller error");
    // Validation failure while saving must likewise not truncate an existing file.
    const auto existing=directory/"retained.judasstate";
    Write(existing,text);
    auto bad=valid;
    bad.entities.front().state.position.x=std::numeric_limits<float>::quiet_NaN();
    Check(!SaveWorldStateToFile(bad,existing.string(),error),"invalid state refuses to serialize: "+error);
    Check(Read(existing)==text,"invalid save request does not truncate existing valid save");
    for(const auto& badName:std::vector<std::string>{"bad\nname",std::string("bad")+char(0)+"name"}) {
        auto badString=valid;
        badString.baselineName=badName;
        Check(!SaveWorldStateToFile(badString,existing.string(),error),"control characters in saved name reject: "+error);
        Check(Read(existing)==text,"invalid string cannot truncate existing save");
    }
    const auto legacy=directory/"protected-legacy.judasstate";
    const std::string legacyBytes="JudasWorldState 1\nbaseline \"FTFT1 same scene name\"\nnext-runtime-id 4611686018427387904\n";
    Write(legacy,legacyBytes);
    Check(!SaveWorldStateToFile(valid,legacy.string(),error),"saving cannot silently overwrite unverifiable legacy file: "+error);
    Check(Read(legacy)==legacyBytes,"legacy file remains byte-identical on attempted overwrite");
    Scene changed=Baseline(); changed.Find(1)->transform.position.x=123;
    RuntimeWorld differentBaseline;
    if(Build(differentBaseline,changed)) {
        const auto other=CaptureWorldState(differentBaseline);
        Check(!SaveWorldStateToFile(other,existing.string(),error),"save from incompatible baseline cannot overwrite existing deltas: "+error);
        Check(Read(existing)==text,"incompatible save overwrite preserves existing file");
    }
}
} // namespace
int main() {
    const auto begin=std::chrono::steady_clock::now();
    const auto directory=std::filesystem::temp_directory_path()/("judas-ftft1-"+std::to_string(begin.time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    Compatibility();
    Atomicity();
    AssetPreflight();
    DeltaRoundTrip(directory);
    FileValidation(directory);
    ExhaustedIdentityRoundTrip();
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::printf("FTFT1: %d checks, %d failures, %.6f seconds\n",g_checks,g_failures,seconds);
    if(g_failures)std::printf("Failed fixture files preserved at %s\n",directory.c_str());
    else std::filesystem::remove_all(directory);
    return g_failures?1:0;
}
