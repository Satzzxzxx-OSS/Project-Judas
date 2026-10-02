#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include "InputSystem.h"
#include <cmath>
#include <limits>

const SceneObject* RuntimeWorld::RuntimeDefinition(EntityId id) const {
    if(const auto* e=FindEntity(id))if(e->lifecycle==EntityLifecycle::Destroyed)return nullptr;
    auto it=m_scriptDefinitions.find(id);return it==m_scriptDefinitions.end()?nullptr:&it->second;
}
BodyHandle RuntimeWorld::RuntimeBody(EntityId id) const {
    if(!RuntimeDefinition(id))return {};
    if(const auto* e=FindEntity(id))if(e->slot!=std::numeric_limits<size_t>::max())return m_dynamicBodies[e->slot].Handle();
    auto it=m_entityCategories.find(id);return it==m_entityCategories.end()?BodyHandle{}:it->second.body;
}
std::vector<SceneObject> RuntimeWorld::ScriptObjects() const {
    std::vector<SceneObject> result;for(const auto& entry:m_scriptDefinitions)if(RuntimeDefinition(entry.first))result.push_back(entry.second);return result;
}
bool RuntimeWorld::SetRuntimeTransform(EntityId id,const SceneTransform& t){
    if(!RuntimeDefinition(id))return false;
    if(!std::isfinite(glm::dot(t.position,t.position))||!std::isfinite(glm::dot(t.scale,t.scale))||
       !std::isfinite(glm::dot(t.rotation,t.rotation))||glm::dot(t.rotation,t.rotation)<1e-12f)return false;
    auto transform=t;transform.rotation=glm::normalize(t.rotation);
    auto& definition=m_scriptDefinitions.at(id);definition.transform=transform;
    // Explicit transform writes are teleports, not continuous kinematic motion.
    auto body=RuntimeBody(id);if(body.IsValid())m_physics.ResetBody(body,transform.position,transform.rotation);
    if(auto* e=FindEntity(id)){e->state.position=transform.position;e->state.rotation=transform.rotation;
        if(e->slot!=std::numeric_limits<size_t>::max()){m_dynamicBodies[e->slot].SetPoseFromState(transform.position,transform.rotation);m_dynamicBodies[e->slot].SnapPresentation();m_dynamicVisuals[e->slot].scale=transform.scale;}}
    if(auto* o=m_hierarchy.Find(id)){
        auto local=transform;if(o->parent){const auto* p=RuntimeDefinition(o->parent);if(p){auto parent=PresentedTransform(p->id,p->transform,1);
            local.position=glm::inverse(parent.rotation)*(transform.position-parent.position)/parent.scale;
            local.rotation=glm::inverse(parent.rotation)*transform.rotation;local.scale=transform.scale/parent.scale;}}
        o->transform=local;
    }
    for(auto& r:m_staticRenderables)if(r.id==id){r.position=transform.position;r.rotation=transform.rotation;r.scale=transform.scale;}
    for(auto& l:m_staticLights)if(l.id==id){l.position=transform.position;l.direction=transform.rotation*glm::vec3(0,0,-1);}
    for(auto& e:m_audioEmitters)if(e.id==id)e.transform=transform;
    for(auto& e:m_particleEmitters)if(e.id==id)e.transform=transform;
    for(auto& c:m_renderCameras)if(c.id==id)c.transform=transform;
    return true;
}
void RuntimeWorld::UpdateScripts(const InputSystem* input,float dt){
    if(!m_scripts){if(!m_hasScripts)return;
        m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);}
    m_scripts->Synchronize(ScriptObjects());m_scripts->Frame(input,dt);
}
void RuntimeWorld::FixedScripts(const InputSystem* input,float dt){
    if(!m_scripts){if(!m_hasScripts)return;
        m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);}
    if(m_scripts){m_scripts->Synchronize(ScriptObjects());m_scripts->Fixed(input,dt);}
}

bool RuntimeWorld::RestoreScriptState(const std::vector<ScriptStateRecord>& records,std::string& error){
    if(records.empty())return true;
    if(!m_scripts)m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);
    return m_scripts->Restore(records,error);
}

void RuntimeWorld::UpdateUIScripts(InputSystem* input,float dt){
    if(!m_scripts){if(!m_hasScripts)return;m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);}
    bool wasPaused=m_ui&&m_ui->Paused();m_scripts->Synchronize(ScriptObjects());m_scripts->UIFrame(input,dt);
    if(input&&!wasPaused&&m_ui&&m_ui->Paused())input->ConsumeBindings({"pause"});
}
void RuntimeWorld::DispatchUIEvents(const InputSystem* input,float dt){if(m_scripts)m_scripts->UIEvents(input,dt);}
