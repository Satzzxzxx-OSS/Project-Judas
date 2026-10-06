#pragma once
#include "SkeletalAnimation.h"
#include <map>
// Stable hierarchical keys use authored glTF node names, not node-array order.
std::string SkeletonJointKey(const Skeleton&,int node);
int FindSkeletonJoint(const Skeleton&,const std::string& key);
bool ResolveJointMask(const Skeleton&,const std::vector<std::string>& keys,std::vector<int>& mask,std::string& error);
bool ValidPose(const Skeleton&,const SkeletalPose&,std::string& error);
struct PoseContribution {
 SkeletalPose pose,reference;float weight=1;bool enabled=true,additive=false;std::vector<int> mask;int order=0;
};
SkeletalPose BlendPoses(const SkeletalPose&,const SkeletalPose&,float weight,const std::vector<int>& mask={});
SkeletalPose ResolvePose(const Skeleton&,const SkeletalPose& base,const std::vector<PoseContribution>& contributions);
struct AnimationLayerSettings {
 std::string id="Layer",clip,referenceClip;bool enabled=true,additive=false;float weight=1,speed=1,time=0,referenceTime=0;
 std::vector<std::string> mask;
 bool operator==(const AnimationLayerSettings& b)const;
};
bool ValidAnimationLayers(const std::vector<AnimationLayerSettings>&,std::string& error);
struct PoseMixer {
 struct Source {AnimationPlayback playback;float weight=1;};
 std::vector<Source> outgoing;float elapsed=0,duration=0;bool paused=false;
 bool Transitioning()const{return !outgoing.empty();}
 float Fraction()const;
 void CrossFade(AnimationPlayback& target,const std::string& clip,float seconds);
 void Clear(){outgoing.clear();elapsed=duration=0;}
 SkeletalPose Sample(const SkeletalAsset&,AnimationPlayback& target,float dt);
};

// Targets and poles are model-space here. RuntimeWorld converts authored world
// intent before resolving. Direct three-node chains; bone lengths never change.
struct LimbIKSettings {
 std::string id="Limb",root,middle,end;
 glm::vec3 target{0},pole{0,0,1};
 float weight=1;bool enabled=true;int order=100;
};
bool ValidLimbIK(const LimbIKSettings&,std::string& error);
bool SolveLimbIK(const Skeleton&,const SkeletalPose&,const LimbIKSettings&,SkeletalPose& output,float& error,std::string& diagnostic);
