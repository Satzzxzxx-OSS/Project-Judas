#include "PerformanceProfiler.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
namespace {
bool PrepareLayer(const SkeletalAsset& asset,const AnimationLayerSettings& settings,RuntimeWorld::AnimationLayer& layer,std::string& error){
 if(!ValidAnimationLayers({settings},error))return false;
 auto clip=std::find_if(asset.clips.begin(),asset.clips.end(),[&](const auto& c){return c.name==settings.clip;});
 if(clip==asset.clips.end()){error="unknown layer clip: "+settings.clip;return false;}
 layer.settings=settings;layer.playback.clip=settings.clip;layer.playback.time=settings.time;layer.playback.speed=settings.speed;
 if(!ResolveJointMask(asset.skeleton,settings.mask,layer.mask,error))return false;
 layer.reference=asset.skeleton.rest;
 if(!settings.referenceClip.empty()){auto ref=std::find_if(asset.clips.begin(),asset.clips.end(),[&](const auto& c){return c.name==settings.referenceClip;});if(ref==asset.clips.end()){error="unknown additive reference clip";return false;}layer.reference=SampleClip(asset.skeleton,*ref,settings.referenceTime);}
 if(settings.additive)for(const auto& p:layer.reference.local)for(int k=0;k<3;++k)if(std::abs(p.scale[k])<1e-8f){error="additive reference scale is zero";return false;}
 return true;
}
}
RuntimeWorld::AnimationInstance* RuntimeWorld::RuntimeAnimation(EntityId id){
 const auto* definition=RuntimeDefinition(id);if(!definition||!definition->animation||!definition->render)return nullptr;
 auto [it,fresh]=m_animationInstances.try_emplace(id);auto& instance=it->second;
 if(fresh){const auto& a=*definition->animation;instance.playback.clip=a.clip;instance.playback.playing=a.playOnStart;instance.playback.loop=a.loop;instance.playback.speed=a.speed;instance.playback.time=a.time;instance.mixer.paused=!a.playOnStart;}
 if(m_assets){auto asset=m_assets->TryGetSkeletal(definition->render->meshAsset);if(asset!=instance.asset){instance.asset=asset;instance.external.clear();instance.mixer.Clear();instance.layers.clear();instance.error.clear();instance.previousWorld.clear();instance.recentWorld.clear();instance.motionDt=0;
  if(asset){for(const auto& setting:definition->animation->layers){AnimationLayer layer;if(PrepareLayer(*asset,setting,layer,instance.error))instance.layers.push_back(std::move(layer));}ResolveAnimationPose(instance,0);}else instance.skin.clear();}}
 return &instance;
}
const std::vector<glm::mat4>* RuntimeWorld::AnimationSkin(EntityId id)const{
 auto it=m_animationInstances.find(id);return it!=m_animationInstances.end()?&it->second.skin:nullptr;
}
void RuntimeWorld::ResolveAnimationPose(AnimationInstance& instance,float dt){
    JUDAS_PROFILE_SCOPE("Final pose resolution");
 if(!instance.asset)return;
 instance.sourcePose=instance.mixer.Sample(*instance.asset,instance.playback,dt);std::vector<PoseContribution> contributions;
 for(auto& layer:instance.layers){if(!layer.settings.enabled)continue;auto pose=layer.playback.Evaluate(*instance.asset,instance.mixer.paused?0:dt);PoseContribution c;c.pose=std::move(pose);c.reference=layer.reference;c.weight=layer.settings.weight;c.mask=layer.mask;c.additive=layer.settings.additive;contributions.push_back(std::move(c));}
 std::vector<std::pair<std::string,const PoseContribution*>> external;for(const auto& item:instance.external)external.push_back({item.first,&item.second});
 std::stable_sort(external.begin(),external.end(),[](const auto& a,const auto& b){return a.second->order<b.second->order;});for(const auto& item:external)contributions.push_back(*item.second);
 instance.finalPose=ResolvePose(instance.asset->skeleton,instance.sourcePose,contributions);instance.skin=ResolveSkinMatrices(instance.asset->skeleton,instance.finalPose);
}
void RuntimeWorld::UpdateAnimations(float dt){
    JUDAS_PROFILE_SCOPE("Animation instances");
 for(auto id:m_animationOwners){const auto* definition=RuntimeDefinition(id);if(!definition){m_animationInstances.erase(id);continue;}
  auto* instance=RuntimeAnimation(id);if(instance&&instance->asset&&definition->animation->enabled){ResolveAnimationPose(*instance,dt);
   if(dt>0){instance->previousWorld=instance->recentWorld;auto transform=PresentedTransform(id,definition->transform,1);auto model=glm::translate(glm::mat4(1),transform.position)*glm::mat4_cast(transform.rotation)*glm::scale(glm::mat4(1),transform.scale);instance->recentWorld=PoseGlobalMatrices(instance->asset->skeleton,instance->finalPose);for(auto& m:instance->recentWorld)m=model*m;instance->motionDt=dt;}
  }
 }
}
bool RuntimeWorld::SetPoseContribution(EntityId id,const std::string& name,const PoseContribution& contribution,std::string& error){
 auto* instance=RuntimeAnimation(id);if(!instance||!instance->asset){error="animation asset is not ready";return false;}
 const auto& skeleton=instance->asset->skeleton;
 if(name.empty()||(!instance->external.count(name)&&instance->external.size()>=16)||!std::isfinite(contribution.weight)||contribution.weight<0||contribution.weight>1){error="invalid pose contribution";return false;}
 if(!ValidPose(skeleton,contribution.pose,error)||(!contribution.reference.local.empty()&&!ValidPose(skeleton,contribution.reference,error)))return false;
 for(int node:contribution.mask)if(node<0||size_t(node)>=skeleton.rest.local.size()){error="invalid pose mask";return false;}
 if(contribution.additive){const auto& ref=contribution.reference.local.empty()?skeleton.rest:contribution.reference;for(const auto& p:ref.local)for(int k=0;k<3;++k)if(std::abs(p.scale[k])<1e-8f){error="additive reference scale is zero";return false;}}
 auto copy=contribution;for(auto& p:copy.pose.local)p.rotation=glm::normalize(p.rotation);for(auto& p:copy.reference.local)p.rotation=glm::normalize(p.rotation);
 instance->external[name]=std::move(copy);ResolveAnimationPose(*instance,0);return true;
}
void RuntimeWorld::RemovePoseContribution(EntityId id,const std::string& name){auto* instance=RuntimeAnimation(id);if(instance){instance->external.erase(name);ResolveAnimationPose(*instance,0);}}
bool RuntimeWorld::SetFinalPose(EntityId id,const SkeletalPose& pose,std::string& error){PoseContribution c;c.pose=pose;return SetPoseContribution(id,"externalOverride",c,error);}
bool RuntimeWorld::SetAnimationLayer(EntityId id,const AnimationLayerSettings& settings,bool remove,std::string& error){
 auto* instance=RuntimeAnimation(id);if(!instance||!instance->asset){error="animation asset is not ready";return false;}
 auto it=std::find_if(instance->layers.begin(),instance->layers.end(),[&](const auto& l){return l.settings.id==settings.id;});
 if(remove){if(it!=instance->layers.end())instance->layers.erase(it);ResolveAnimationPose(*instance,0);return true;}
 if(it==instance->layers.end()&&instance->layers.size()>=16){error="too many layers";return false;}AnimationLayer layer;if(!PrepareLayer(*instance->asset,settings,layer,error))return false;
 if(it!=instance->layers.end()){if(it->settings.clip==settings.clip){layer.playback=it->playback;layer.playback.speed=settings.speed;}*it=std::move(layer);}else instance->layers.push_back(std::move(layer));ResolveAnimationPose(*instance,0);return true;
}
