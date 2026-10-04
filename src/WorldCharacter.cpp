#include "PerformanceProfiler.h"
#include "RuntimeWorld.h"
#include <glm/gtc/matrix_transform.hpp>
CharacterMotor* RuntimeWorld::RuntimeCharacter(EntityId id){
    auto* d=RuntimeDefinition(id);if(!d||!d->characterMotor)return nullptr;
    auto [it,added]=m_characters.try_emplace(id);
    auto& c=it->second;if(added){c.motor.Reset(d->transform.position,d->transform.rotation);c.previous=d->transform;}
    c.motor.settings=*d->characterMotor;return &c.motor;
}
void RuntimeWorld::ClearCharacters(){m_characters.clear();view.reset();}
void RuntimeWorld::UpdateCharacters(float dt){
    JUDAS_PROFILE_SCOPE("Character motors");
 JUDAS_PROFILE_COUNTER("Character motor instances",double(m_characters.size()),ProfileCounterMode::Latest);
    // Stable entity ordering, independent state; no input/camera/clip ownership.
    for(const auto& [id,d]:m_scriptDefinitions)if(d.characterMotor&&RuntimeDefinition(id))RuntimeCharacter(id);
    for(auto it=m_characters.begin();it!=m_characters.end();){auto id=it->first;auto* d=RuntimeDefinition(id);
        if(!d||!d->characterMotor){it=m_characters.erase(it);continue;}
        auto& c=it->second;auto& motor=c.motor;c.previous=d->transform;
        motor.settings=*d->characterMotor;
        motor.Step(m_physics,m_gravityMap,dt);
        // Internal resolved motion writes bypass the explicit-teleport/reset API.
        auto& definition=m_scriptDefinitions.at(id);definition.transform.position=motor.position;definition.transform.rotation=motor.orientation;
        if(auto* e=FindEntity(id)){e->state.position=motor.position;e->state.rotation=motor.orientation;e->state.linearVelocity=motor.velocity;}
        if(auto* h=m_hierarchy.Find(id)){auto t=definition.transform;if(h->parent){auto* p=RuntimeDefinition(h->parent);if(p){auto parent=PresentedTransform(p->id,p->transform,1);t.position=glm::inverse(parent.rotation)*(t.position-parent.position)/parent.scale;t.rotation=glm::inverse(parent.rotation)*t.rotation;}}h->transform=t;}
        ++it;
    }
}
bool RuntimeWorld::SetRuntimeView(const SceneTransform& pose,float fov){
    if(!std::isfinite(glm::dot(pose.position,pose.position))||!std::isfinite(glm::dot(pose.rotation,pose.rotation))||glm::dot(pose.rotation,pose.rotation)<1e-12f||!std::isfinite(fov)||fov<=1||fov>=179)return false;
    view=RuntimeView{pose,fov};view->pose.rotation=glm::normalize(pose.rotation);return true;
}

bool RuntimeWorld::SetCharacterSettings(EntityId id,const CharacterMotorSettings& settings){
    auto* m=RuntimeCharacter(id);std::string error;if(!m||!ValidCharacterMotor(settings,error))return false;
    m_scriptDefinitions.at(id).characterMotor=settings;m->settings=settings;
    if(!settings.enabled){m->result={};m->acceleration={0,0,0};}return true;
}
