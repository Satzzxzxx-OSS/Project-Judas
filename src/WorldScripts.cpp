#include "SceneSession.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include "InputSystem.h"
#include <cmath>
#include <limits>

const SceneObject* RuntimeWorld::RuntimeDefinition(EntityId id) const {
    if(!IsPublished(id))return nullptr;
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
    const auto* authored=RuntimeDefinition(id);if(!authored)return false;
    // Baked static capacity data cannot silently follow a runtime teleport.
    if(authored->liquidBasin&&(t.position!=authored->transform.position||t.rotation!=authored->transform.rotation||t.scale!=authored->transform.scale))return false;
    if(authored->liquidContainer&&t.scale!=glm::vec3(1))return false;
    if(!std::isfinite(glm::dot(t.position,t.position))||!std::isfinite(glm::dot(t.scale,t.scale))||
       !std::isfinite(glm::dot(t.rotation,t.rotation))||glm::dot(t.rotation,t.rotation)<1e-12f)return false;
    if(auto it=m_characters.find(id);it!=m_characters.end()){
        it->second.motor.Reset(t.position,t.rotation);it->second.previous=t;
    }
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
    SynchronizeJoints();
    if(!m_scripts){if(!m_hasScripts)return;
        m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);}
    if(m_scripts){m_scripts->Synchronize(ScriptObjects());m_scripts->Fixed(input,dt);}
}

bool RuntimeWorld::RestoreScriptState(const std::vector<ScriptStateRecord>& records,std::string& error,bool resume){
    if(records.empty())return true;
    if(!m_scripts)m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);
    return m_scripts->Restore(records,error,resume);
}

void RuntimeWorld::UpdateUIScripts(InputSystem* input,float dt){
    if(!m_scripts){if(!m_hasScripts)return;m_scripts=std::make_unique<ScriptSystem>(this,m_assets?m_assets->Assets():nullptr);}
    bool wasPaused=m_ui&&m_ui->Paused();m_scripts->Synchronize(ScriptObjects());m_scripts->UIFrame(input,dt);
    if(input&&!wasPaused&&m_ui&&m_ui->Paused())input->ConsumeBindings({"pause"});
}
void RuntimeWorld::DispatchUIEvents(const InputSystem* input,float dt){if(m_scripts)m_scripts->UIEvents(input,dt);}

void RuntimeWorld::DispatchPhysicsEvents(const InputSystem* input,float dt){
    if(!m_scripts)return; // No script consumers: do not scan/copy entity definitions.
    // Freeze pair->entity identities before any callback can destroy/spawn bodies.
    for(const auto& o:ScriptObjects()){auto h=RuntimeBody(o.id);if(h.IsValid())m_touchEntityHistory[h.id]=o.id;}
    struct Delivery {EntityId a,b;PhysicsWorld::TouchEvent event;};
    std::vector<Delivery> deliveries;
    std::map<unsigned,EntityId> next;
    for(const auto& event:m_physics.LastStepTouchEvents()){
        auto a=m_touchEntityHistory.find(event.a.id),b=m_touchEntityHistory.find(event.b.id);
        if(a==m_touchEntityHistory.end()||b==m_touchEntityHistory.end())continue;
        deliveries.push_back({a->second,b->second,event});
        if(event.phase!=PhysicsWorld::TouchPhase::Exit){next[event.a.id]=a->second;next[event.b.id]=b->second;}
    }
    if(m_scripts){m_scripts->Synchronize(ScriptObjects());
        for(const auto& d:deliveries){m_scripts->PhysicsEvent(d.a,d.b,d.event,false);m_scripts->PhysicsEvent(d.b,d.a,d.event,true);}}
    m_touchEntityHistory=std::move(next);(void)input;(void)dt;
}
bool RuntimeWorld::SetColliderEnabled(EntityId id,bool enabled){
    auto h=RuntimeBody(id);if(!h.IsValid()||!m_physics.SetBodyEnabled(h,enabled))return false;
    auto& d=m_scriptDefinitions.at(id);if(d.body)d.body->enabled=enabled;return true;
}

void RuntimeWorld::SynchronizeJoints(){
    for(auto owner:m_jointOwners){const auto* definition=RuntimeDefinition(owner);
        auto it=m_runtimeJoints.find(owner);JointState state;
        if(!definition||!definition->joint){if(it!=m_runtimeJoints.end()){m_physics.DestroyJoint(it->second);m_runtimeJoints.erase(it);}continue;}
        const auto& j=*definition->joint;auto a=RuntimeBody(j.bodyA),b=RuntimeBody(j.bodyB);
        if(it!=m_runtimeJoints.end()&&m_physics.GetJoint(it->second,state)&&state.settings.bodyA.id==a.id&&state.settings.bodyB.id==b.id)continue;
        if(it!=m_runtimeJoints.end()){m_physics.DestroyJoint(it->second);m_runtimeJoints.erase(it);}
        if(!a.IsValid()||(j.bodyB&&!b.IsValid()))continue;
        auto settings=j.settings;settings.bodyA=a;settings.bodyB=b;
        if(!j.bodyB){settings.anchorB=definition->transform.position+definition->transform.rotation*settings.anchorB;settings.frameB=definition->transform.rotation*settings.frameB;}
        auto handle=m_physics.CreateJoint(settings);if(handle.IsValid())m_runtimeJoints[owner]=handle;
    }
}
JointHandle RuntimeWorld::RuntimeJoint(EntityId owner){SynchronizeJoints();auto it=m_runtimeJoints.find(owner);return it==m_runtimeJoints.end()?JointHandle{}:it->second;}

void RuntimeWorld::PresentationScripts(const InputSystem* input,float dt,float alpha){
    // Downstream of fixed simulation; reuse the renderer's existing pose history.
    if(m_scripts)m_scripts->Presentation(input,dt,alpha);
}

LocalizationSession& RuntimeWorld::Localization(){if(auto scenes=SceneControl())return scenes->Localization(m_assets);if(!m_localization)m_localization=std::make_unique<LocalizationSession>();m_localization->Bind(m_assets);return *m_localization;}
