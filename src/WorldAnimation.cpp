#include "PerformanceProfiler.h"
#include "RuntimeWorld.h"
#include "ResourceManager.h"
#include <set>
#include "Ragdoll.h"
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
 auto [it,fresh]=m_animationInstances.try_emplace(id);auto& instance=it->second;instance.owner=id;
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
 // Resolve producers deterministically by (order, stable id), then skin the
 // same final pose queried by scripts. Physical contribution keeps order 1000.
 instance.finalPose=ResolvePose(instance.asset->skeleton,instance.sourcePose,contributions);
 struct Producer {int order;std::string id;const PoseContribution* external;const LimbIKSettings* limb;};
 std::vector<Producer> sources;for(const auto& [key,c]:instance.external)sources.push_back({c.order,key,&c,nullptr});
 // Runtime-authored IK settings live in the ordinary scene definition.
 if(const auto* d=RuntimeDefinition(instance.owner);d&&d->animation)for(const auto& k:d->animation->limbs)if(k.enabled)sources.push_back({k.order,k.id,nullptr,&k});
 std::stable_sort(sources.begin(),sources.end(),[](const auto& a,const auto& b){return std::tie(a.order,a.id)<std::tie(b.order,b.id);});
 for(const auto& source:sources){if(source.external)instance.finalPose=ResolvePose(instance.asset->skeleton,instance.finalPose,{*source.external});
  else {auto k=*source.limb;EntityId owner=instance.owner;
   const auto* d=RuntimeDefinition(owner);if(!d)continue;auto transform=PresentedTransform(owner,d->transform,1);auto model=glm::translate(glm::mat4(1),transform.position)*glm::mat4_cast(transform.rotation)*glm::scale(glm::mat4(1),transform.scale);auto inverse=glm::inverse(model);k.target=glm::vec3(inverse*glm::vec4(k.target,1));k.pole=glm::vec3(inverse*glm::vec4(k.pole,1));
   SkeletalPose pose;float error=0;std::string diagnostic;if(SolveLimbIK(instance.asset->skeleton,instance.finalPose,k,pose,error,diagnostic))instance.finalPose=std::move(pose);else instance.error=diagnostic;
  }
 }
 instance.skin=ResolveSkinMatrices(instance.asset->skeleton,instance.finalPose);
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

bool RuntimeWorld::JointPose(EntityId id,const std::string& key,const std::string& space,float alpha,SceneTransform& out){
 auto* a=RuntimeAnimation(id);if(!a||!a->asset)return false;const auto& s=a->asset->skeleton;int n=FindSkeletonJoint(s,key);if(n<0||size_t(n)>=a->finalPose.local.size())return false;
 if(space=="local"){const auto& t=a->finalPose.local[n];out={t.translation,t.rotation,t.scale};return true;}
 if(space!="model"&&space!="world")return false;
 auto matrix=PoseGlobalMatrices(s,a->finalPose)[n];
 if(space=="world"){auto* d=RuntimeDefinition(id);auto t=PresentedTransform(id,d->transform,alpha);matrix=glm::translate(glm::mat4(1),t.position)*glm::mat4_cast(t.rotation)*glm::scale(glm::mat4(1),t.scale)*matrix;}
 JointTransform t;std::string error;if(!DecomposeRigidPose(matrix,t,error))return false;out={t.translation,t.rotation,t.scale};return true;
}
bool RuntimeWorld::SetLimbIK(EntityId id,const LimbIKSettings& k,bool remove,std::string& error){
 auto* d=RuntimeDefinition(id);if(!d||!d->animation){error="entity has no animation";return false;}auto* a=RuntimeAnimation(id);if(!a||!a->asset){error="skeleton is not ready";return false;}
 auto settings=*d->animation;auto it=std::find_if(settings.limbs.begin(),settings.limbs.end(),[&](const auto& v){return v.id==k.id;});
 if(remove){if(it!=settings.limbs.end())settings.limbs.erase(it);}else{SkeletalPose tested;float residual=0;if(!SolveLimbIK(a->asset->skeleton,a->finalPose,k,tested,residual,error))return false;if(it==settings.limbs.end()){if(settings.limbs.size()>=16){error="16 limb contributor limit";return false;}settings.limbs.push_back(k);}else *it=k;}
 m_scriptDefinitions.at(id).animation=settings;ResolveAnimationPose(*a,0);return true;
}
bool RuntimeWorld::SetSocket(EntityId id,const SceneSocketComponent& s,bool remove,std::string& error){
 auto* d=RuntimeDefinition(id);if(!d){error="stale socket owner";return false;}if(remove){m_scriptDefinitions.at(id).socket.reset();return true;}
 if(!s.target||s.joint.empty()||d->body.has_value()||d->characterMotor||d->ragdoll||d->deformable){error="visual socket cannot own physical motion";return false;}
 auto* skeleton=RuntimeAnimation(s.target);if(!skeleton||!skeleton->asset||FindSkeletonJoint(skeleton->asset->skeleton,s.joint)<0){error="socket target skeleton/joint is unavailable";return false;}
 std::set<EntityId> seen{id};EntityId target=s.target;while(target){if(!seen.insert(target).second){error="socket attachment cycle";return false;}auto* t=RuntimeDefinition(target);if(!t){error="missing socket target";return false;}target=t->socket?t->socket->target:t->parent;}
 auto finite=[](glm::vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};if(!finite(s.offset.position)||!finite(s.offset.scale)){error="invalid socket transform";return false;}
 auto q=glm::dot(s.offset.rotation,s.offset.rotation);if(!std::isfinite(q)||q<1e-12f){error="invalid socket orientation";return false;}
 m_scriptDefinitions.at(id).socket=s;return true;
}
void RuntimeWorld::UpdateSockets(){
 std::set<EntityId> done,visiting;std::function<void(EntityId)> visit=[&](EntityId id){if(done.count(id)||!visiting.insert(id).second)return;const auto* d=RuntimeDefinition(id);if(d&&d->socket&&d->socket->enabled){const auto s=*d->socket;visit(s.target);SceneTransform joint;if(JointPose(s.target,s.joint,"world",1,joint)){SceneTransform pose;pose.position=joint.position+joint.rotation*(joint.scale*s.offset.position);pose.rotation=glm::normalize(joint.rotation*s.offset.rotation);pose.scale=joint.scale*s.offset.scale;SetRuntimeTransform(id,pose);}}visiting.erase(id);done.insert(id);};
 for(const auto& [id,d]:m_scriptDefinitions)if(d.socket)visit(id);
}
