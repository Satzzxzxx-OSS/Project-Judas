#pragma once
#include "Scene.h"
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
    void Frame(const InputSystem* input,float dt);
    void Fixed(const InputSystem* input,float dt);
    void Stop();
    std::vector<ScriptStateRecord> Capture() const;
    bool Restore(const std::vector<ScriptStateRecord>& records,std::string& error);
    const std::vector<ScriptDiagnostic>& Diagnostics() const;
    void SetBudget(unsigned interruptPolls);
    static bool ValidateJson(const std::string& text,std::string& error);
    static bool ReadProperties(const std::string& schema,const std::string& values,std::vector<ScriptProperty>& out,std::string& error);
    static std::string WriteProperties(const std::vector<ScriptProperty>& values);
    static bool SourceFingerprint(const AssetDatabase& assets,const Scene& scene,std::string& digest,std::string& error,bool strict=true);
    static bool Inspect(const AssetDatabase& assets,const std::string& asset,std::string& schema,std::string& error);
private:
    struct Impl;std::unique_ptr<Impl> m;
};
