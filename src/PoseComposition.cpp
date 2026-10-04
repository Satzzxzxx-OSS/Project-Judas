#include "PerformanceProfiler.h"
#include "PoseComposition.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
std::string SkeletonJointKey(const Skeleton& s,int node){
 if(node<0||size_t(node)>=s.names.size())return {};
 std::string name=s.names[node]; // escape delimiter so imported names remain unambiguous
 size_t p=0;while((p=name.find('%',p))!=std::string::npos){name.replace(p,1,"%25");p+=3;}
 p=0;while((p=name.find('/',p))!=std::string::npos){name.replace(p,1,"%2F");p+=3;}
 return s.parents[node]<0?name:SkeletonJointKey(s,s.parents[node])+"/"+name;
}
int FindSkeletonJoint(const Skeleton& s,const std::string& key){int found=-1;
 for(size_t i=0;i<s.names.size();++i)if(SkeletonJointKey(s,int(i))==key){if(found>=0)return -1;found=int(i);}
 if(found>=0)return found;
 for(size_t i=0;i<s.names.size();++i)if(s.names[i]==key){if(found>=0)return -1;found=int(i);}
 return found;
}
bool ResolveJointMask(const Skeleton& s,const std::vector<std::string>& keys,std::vector<int>& mask,std::string& error){
 std::vector<int> result;for(const auto& key:keys){int node=FindSkeletonJoint(s,key);if(node<0){error="missing or ambiguous skeleton joint: "+key;return false;}if(std::find(result.begin(),result.end(),node)!=result.end()){error="duplicate masked joint";return false;}result.push_back(node);}mask=std::move(result);return true;
}
bool ValidPose(const Skeleton& s,const SkeletalPose& p,std::string& error){
 if(p.local.size()!=s.rest.local.size()){error="pose does not match skeleton";return false;}
 for(const auto& v:p.local){for(int k=0;k<3;++k)if(!std::isfinite(v.translation[k])||!std::isfinite(v.scale[k])){error="nonfinite pose";return false;}
 if(!std::isfinite(glm::dot(v.rotation,v.rotation))||glm::dot(v.rotation,v.rotation)<1e-12f){error="invalid pose quaternion";return false;}}return true;
}
SkeletalPose BlendPoses(const SkeletalPose& a,const SkeletalPose& b,float weight,const std::vector<int>& mask){
 if(a.local.size()!=b.local.size())throw std::invalid_argument("incompatible poses");
 SkeletalPose result=a;
 auto blend=[&](int node){auto& v=result.local.at(node);const auto& source=b.local.at(node);v.translation=glm::mix(v.translation,source.translation,weight);v.scale=glm::mix(v.scale,source.scale,weight);v.rotation=glm::normalize(glm::slerp(glm::normalize(v.rotation),glm::normalize(source.rotation),weight));};
 if(mask.empty())for(size_t i=0;i<a.local.size();++i)blend(int(i));else for(int node:mask)blend(node);return result;
}
SkeletalPose ResolvePose(const Skeleton& s,const SkeletalPose& base,const std::vector<PoseContribution>& contributions){
 SkeletalPose result=base; // inputs remain independent, never mutate a producer's pose
 for(const auto& c:contributions){if(!c.enabled||c.weight<=0)continue;
  if(!c.additive){result=BlendPoses(result,c.pose,c.weight,c.mask);continue;}
  const auto& reference=c.reference.local.empty()?s.rest:c.reference;
  auto apply=[&](int node){auto& v=result.local.at(node);const auto& from=reference.local.at(node);const auto& to=c.pose.local.at(node);
   v.translation+=c.weight*(to.translation-from.translation);
   auto delta=glm::normalize(glm::inverse(glm::normalize(from.rotation))*glm::normalize(to.rotation));
   v.rotation=glm::normalize(v.rotation*glm::slerp(glm::quat(1,0,0,0),delta,c.weight));
   for(int k=0;k<3;++k){if(std::abs(from.scale[k])<1e-8f)throw std::invalid_argument("additive reference scale is zero");v.scale[k]*=glm::mix(1.f,to.scale[k]/from.scale[k],c.weight);}
  };
  if(c.mask.empty())for(size_t i=0;i<result.local.size();++i)apply(int(i));else for(int node:c.mask)apply(node);
 }return result;
}
bool AnimationLayerSettings::operator==(const AnimationLayerSettings& b)const{return id==b.id&&clip==b.clip&&referenceClip==b.referenceClip&&enabled==b.enabled&&additive==b.additive&&weight==b.weight&&speed==b.speed&&time==b.time&&referenceTime==b.referenceTime&&mask==b.mask;}
bool ValidAnimationLayers(const std::vector<AnimationLayerSettings>& layers,std::string& error){
 if(layers.size()>16){error="at most 16 animation layers";return false;}std::set<std::string> ids;
 for(const auto& l:layers)if(l.id.empty()||!ids.insert(l.id).second||!std::isfinite(l.weight)||l.weight<0||l.weight>1||!std::isfinite(l.speed)||!std::isfinite(l.time)||l.time<0||!std::isfinite(l.referenceTime)||l.referenceTime<0||l.mask.size()>128){error="invalid animation layer";return false;}
 return true;
}
float PoseMixer::Fraction()const{return duration>0?std::clamp(elapsed/duration,0.f,1.f):1;}
void PoseMixer::CrossFade(AnimationPlayback& target,const std::string& clip,float seconds){
 if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("invalid fade duration");
 if(seconds>0 && outgoing.size()>=16)throw std::invalid_argument("too many interrupted fade contributors");
 if(seconds==0){Clear();}else {float fraction=Fraction();std::vector<Source> sources;
  if(Transitioning()){for(auto source:outgoing){source.weight*=1-fraction;if(source.weight>0)sources.push_back(std::move(source));}if(fraction>0)sources.push_back({target,fraction});}
  else sources.push_back({target,1});
  outgoing=std::move(sources);duration=seconds;elapsed=0;
 }
 target.clip=clip;target.time=0;target.playing=true;target.stopped=false;paused=false;
}
SkeletalPose PoseMixer::Sample(const SkeletalAsset& a,AnimationPlayback& target,float dt){
 JUDAS_PROFILE_SCOPE("Pose clip mixing");
 float advance=paused?0:dt;auto pose=target.Evaluate(a,advance);
 if(!Transitioning())return pose;
 elapsed=std::min(duration,elapsed+advance);float fraction=Fraction();SkeletalPose combined=a.skeleton.rest;float total=0;
 for(auto& source:outgoing){auto sampled=source.playback.Evaluate(a,advance);float weight=source.weight*(1-fraction);if(weight>0){combined=BlendPoses(combined,sampled,weight/(total+weight));total+=weight;}}
 if(fraction>0)combined=BlendPoses(combined,pose,fraction/(total+fraction));
 if(fraction>=1)Clear();
 return combined;
}
