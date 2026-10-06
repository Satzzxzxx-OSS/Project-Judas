#include <map>
#pragma once
#include "Scene.h"
#include "PhysicsWorld.h"
#include <memory>
#include <string>
#include <vector>
class RuntimeWorld;
class AssetDatabase;
class InputSystem;
struct ScriptStateRecord { SceneObjectId entity=0;std::uint64_t slot=0;std::string json="{}"; };
struct ScriptProperty {std::string name,type,text;double number=0;bool boolean=false;};
struct ScriptDiagnostic {SceneObjectId entity=0;std::uint64_t slot=0;std::string asset,callback,message;};
// One main-thread session per world. No backend objects escape this boundary.
class ScriptSystem {
public:
    explicit ScriptSystem(RuntimeWorld* world,const AssetDatabase* assets);
    ~ScriptSystem();
    ScriptSystem(const ScriptSystem&)=delete;
    void Synchronize(const std::vector<SceneObject>& objects);
    // Latest active presentation camera, for project-authored pointing UI.
    void SetView(const glm::mat4& view);
    void Frame(const InputSystem* input,float dt);
    void Fixed(const InputSystem* input,float dt);
    void UIFrame(const InputSystem* input,float dt);
    void Presentation(const InputSystem* input,float dt,float alpha);
    void FractureEvent(SceneObjectId,uint64_t,const std::vector<std::string>&,const std::vector<unsigned>&);
    void PhysicsEvent(SceneObjectId self,SceneObjectId other,const PhysicsWorld::TouchEvent& event,bool reverse);
    void UIEvents(const InputSystem* input,float dt);
    void Stop();
    void RemoveEntities(const std::vector<SceneObjectId>&);
    std::vector<ScriptStateRecord> Capture(bool required=false) const;
    bool Restore(const std::vector<ScriptStateRecord>& records,std::string& error,bool resume=false);
    const std::vector<ScriptDiagnostic>& Diagnostics() const;
    void SetBudget(unsigned interruptPolls);
    static bool ValidateJson(const std::string& text,std::string& error,bool requireObject=true);
    static std::vector<SceneObjectId> PropertyEntities(const std::string& values);
    static std::string RemapPropertyEntities(const std::string& values,const std::map<SceneObjectId,SceneObjectId>& ids);
    static bool SetPropertyEntity(std::string& values,const std::string& name,SceneObjectId id);
    static bool ReadProperties(const std::string& schema,const std::string& values,std::vector<ScriptProperty>& out,std::string& error);
    static std::string WriteProperties(const std::vector<ScriptProperty>& values);
    static bool SourceFingerprint(const AssetDatabase& assets,const Scene& scene,std::string& digest,std::string& error,bool strict=true);
    static bool Inspect(const AssetDatabase& assets,const std::string& asset,std::string& schema,std::string& error);
private:
    struct Impl;std::unique_ptr<Impl> m;
};
