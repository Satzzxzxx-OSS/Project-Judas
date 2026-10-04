#include "PerformanceProfiler.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>

namespace {
bool Pose(const RuntimeWorld& world,SceneObjectId id,const SceneTransform& authored,BodyHandle body,float alpha,glm::vec3& position,glm::quat& rotation){
    position=authored.position;rotation=glm::normalize(authored.rotation);
    if(const auto* entity=world.FindEntity(id)){
        if(entity->lifecycle!=EntityLifecycle::Active)return false;
        const auto t=world.PresentedTransform(id,authored,alpha);position=t.position;rotation=t.rotation;
    }else if(body.IsValid()){
        const auto transform=world.Physics().GetTransform(body);position=transform.position;rotation=transform.rotation;
    }
    const auto t=world.PresentedTransform(id,SceneTransform{position,rotation,authored.scale},alpha);
    position=t.position;rotation=t.rotation;
    return true;
}
}
void RuntimeWorld::BeginAudio(){EndAudio();m_audioRunning=true;for(auto& e:m_audioEmitters){e.wantPlay=e.settings.playOnStart;e.error.clear();}}
void RuntimeWorld::EndAudio(){
    for(auto& e:m_audioEmitters){if(m_audioSystem)m_audioSystem->DestroyVoice(e.voice);e.voice={};}
    m_audioRunning=false;
}
void RuntimeWorld::UpdateAudio(const glm::mat4& activeView,float alpha){
    JUDAS_PROFILE_SCOPE("Audio main update");
    if(!m_audioRunning||!m_audioSystem||!m_assets||m_audioEmitters.empty())return;
    glm::vec3 listenerPosition{0};glm::quat listenerOrientation{1,0,0,0};bool listenerActive=false;
    if(m_audioListener&&m_audioListener->settings.enabled){
        const auto& l=*m_audioListener;
        listenerActive=Pose(*this,l.id,l.transform,l.staticBody,alpha,listenerPosition,listenerOrientation);
        if(listenerActive&&l.settings.followActiveView){
            const glm::mat4 transform=glm::inverse(activeView);
            listenerPosition=glm::vec3(transform[3]);listenerOrientation=glm::normalize(glm::quat_cast(glm::mat3(transform)));
        }
    }
    for(auto& e:m_audioEmitters){
        glm::vec3 position;glm::quat rotation;
        const bool active=e.settings.enabled&&Pose(*this,e.id,e.transform,e.staticBody,alpha,position,rotation)&&(!e.settings.spatial||listenerActive);
        if(!active){
            if(e.settings.enabled&&e.settings.spatial&&!listenerActive&&e.error!="no active audio listener"){
                e.error="no active audio listener";
                std::fprintf(stderr,"audio emitter %llu: %s\n",static_cast<unsigned long long>(e.id),e.error.c_str());
            }
            m_audioSystem->DestroyVoice(e.voice);e.voice={};continue;
        }
        AudioVoiceSnapshot existing;
        if(e.voice.IsValid()&&!m_audioSystem->Snapshot(e.voice,existing))e.voice={};
        if(!e.voice.IsValid()){
            std::string error;const auto clip=m_assets->GetAudio(e.settings.asset,error);
            if(clip.IsValid()){
                if(m_audioSystem->Init(error)){
                    if(listenerActive)m_audioSystem->SetListener(listenerPosition,listenerOrientation);
                    e.voice=m_audioSystem->CreateVoice(clip,e.settings,error);
                    if(e.voice.IsValid()){
                        m_audioSystem->SetPosition(e.voice,position);
                        if(e.wantPlay)m_audioSystem->Play(e.voice);
                    }
                }
            }
            if(!error.empty()&&error!="loading"&&error!=e.error)std::fprintf(stderr,"audio emitter %llu: %s\n",static_cast<unsigned long long>(e.id),error.c_str());
            e.error=error;
        }
        if(e.voice.IsValid()){m_audioSystem->SetSettings(e.voice,e.settings);m_audioSystem->SetPosition(e.voice,position);}
    }
    if(listenerActive)m_audioSystem->SetListener(listenerPosition,listenerOrientation);
    m_audioSystem->Update();
}
bool RuntimeWorld::PlayAudio(SceneObjectId id){for(auto& e:m_audioEmitters)if(e.id==id){e.wantPlay=true;return !e.voice.IsValid()||(m_audioSystem&&m_audioSystem->Play(e.voice));}return false;}
bool RuntimeWorld::StopAudio(SceneObjectId id){for(auto& e:m_audioEmitters)if(e.id==id){e.wantPlay=false;return !e.voice.IsValid()||(m_audioSystem&&m_audioSystem->Stop(e.voice));}return false;}
bool RuntimeWorld::PauseAudio(SceneObjectId id){for(auto& e:m_audioEmitters)if(e.id==id)return m_audioSystem&&m_audioSystem->Pause(e.voice);return false;}
bool RuntimeWorld::ResumeAudio(SceneObjectId id){for(auto& e:m_audioEmitters)if(e.id==id)return m_audioSystem&&m_audioSystem->Resume(e.voice);return false;}
bool RuntimeWorld::SetAudioEnabled(SceneObjectId id,bool enabled){for(auto& e:m_audioEmitters)if(e.id==id){e.settings.enabled=enabled;if(!enabled&&m_audioSystem){m_audioSystem->DestroyVoice(e.voice);e.voice={};}return true;}return false;}
