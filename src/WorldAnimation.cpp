#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include <cmath>
RuntimeWorld::AnimationInstance* RuntimeWorld::RuntimeAnimation(EntityId id){
 const auto* definition=RuntimeDefinition(id);if(!definition||!definition->animation||!definition->render)return nullptr;
 auto [it,fresh]=m_animationInstances.try_emplace(id);auto& instance=it->second;
 if(fresh){const auto& a=*definition->animation;instance.playback.clip=a.clip;instance.playback.playing=a.playOnStart;instance.playback.loop=a.loop;instance.playback.speed=a.speed;instance.playback.time=a.time;}
 if(m_assets){auto asset=m_assets->TryGetSkeletal(definition->render->meshAsset);if(asset!=instance.asset){instance.asset=asset;if(asset){instance.finalPose=instance.playback.Evaluate(*asset,0);instance.skin=ResolveSkinMatrices(asset->skeleton,instance.finalPose);}else instance.skin.clear();}}
 return &instance;
}
const std::vector<glm::mat4>* RuntimeWorld::AnimationSkin(EntityId id)const{
 auto it=m_animationInstances.find(id);return it!=m_animationInstances.end()?&it->second.skin:nullptr;
}
void RuntimeWorld::UpdateAnimations(float dt){
 for(auto id:m_animationOwners){const auto* definition=RuntimeDefinition(id);if(!definition){m_animationInstances.erase(id);continue;}
  auto* instance=RuntimeAnimation(id);if(!instance||!instance->asset||!definition->animation->enabled)continue;
  // M46 resolves one source. This is the pose-resolution boundary, not an
  // ownership right held by the clip player. Other producers can supply SkeletalPose.
  instance->finalPose=instance->playback.Evaluate(*instance->asset,dt);
  instance->skin=ResolveSkinMatrices(instance->asset->skeleton,instance->finalPose);
 }
}
bool RuntimeWorld::SetFinalPose(EntityId id,const SkeletalPose& pose,std::string& error){
 auto* instance=RuntimeAnimation(id);if(!instance||!instance->asset){error="animation asset is not ready";return false;}
 if(pose.local.size()!=instance->asset->skeleton.rest.local.size()){error="pose does not match skeleton";return false;}
 for(const auto& p:pose.local){for(int i=0;i<3;++i)if(!std::isfinite(p.translation[i])||!std::isfinite(p.scale[i])){error="nonfinite pose";return false;}
  if(!std::isfinite(glm::dot(p.rotation,p.rotation))||glm::dot(p.rotation,p.rotation)<1e-12f){error="invalid pose rotation";return false;}}
 instance->finalPose=pose;for(auto& p:instance->finalPose.local)p.rotation=glm::normalize(p.rotation);
 instance->skin=ResolveSkinMatrices(instance->asset->skeleton,instance->finalPose);return true;
}
