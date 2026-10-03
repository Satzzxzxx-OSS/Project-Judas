#include "CharacterMotor.h"
#include "StepClimb.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>

namespace {
glm::quat Align(glm::vec3 a,glm::vec3 b){
    float d=std::clamp(glm::dot(a,b),-1.f,1.f);auto c=glm::cross(a,b);float n=glm::length(c);
    if(d<-.9999f){auto axis=glm::cross(a,std::abs(a.x)<.9f?glm::vec3(1,0,0):glm::vec3(0,1,0));return glm::angleAxis(glm::pi<float>(),glm::normalize(axis));}
    return n<1e-7f?glm::quat(1,0,0,0):glm::angleAxis(std::atan2(n,d),c/n);
}
}
void ResolveCharacterSlide(PhysicsWorld& physics,glm::vec3& center,const glm::quat& rotation,
    glm::vec3 remaining,glm::vec3& velocity,const CharacterMotorSettings& s,
    const PhysicsQueryFilter* filter,bool movingGeometry,bool legacyPush,bool& collided){
    float bodyMotionTime=0;
    float pushBudget=s.maxPushImpulse;
    for(int i=0;i<4;++i){
        float length=glm::length(remaining);if(length<1e-6f)break;
        auto hit=legacyPush?physics.SweepPlayerShape(center,rotation,remaining,movingGeometry,bodyMotionTime,1):
            physics.SweepCapsuleMotion(s.radius,s.halfHeight,center,rotation,remaining,movingGeometry,bodyMotionTime,1,filter,s.collisionLayer,s.collisionMask);
        if(!hit.hit){center+=remaining;break;}collided=true;
        if(physics.IsDynamicBody(hit.hitBody)){
            auto direction=-hit.normal;auto objectVelocity=physics.GetLinearVelocity(hit.hitBody);
            float speed=legacyPush ? glm::dot(velocity,direction)-glm::dot(objectVelocity,direction)
                                   : glm::dot(velocity-objectVelocity,direction);
            if(speed>0&&(!legacyPush||glm::dot(velocity,direction)>0)){if(legacyPush)physics.SetLinearVelocity(hit.hitBody,physics.GetLinearVelocity(hit.hitBody)+direction*speed);
                else {float mass=physics.GetMass(hit.hitBody);float impulse= s.interactionMass>0 ? speed/(1/s.interactionMass+1/mass):0;
                    const float applied=std::min(impulse,pushBudget);pushBudget-=applied;
                    physics.ApplyLinearImpulse(hit.hitBody,direction*applied);}}
        }
        float fraction=0;
        if(hit.distance<s.skin)center+=hit.normal*(s.skin-hit.distance);
        else {fraction=(hit.distance-s.skin)/length;center+=remaining*fraction;}
        bodyMotionTime+=(1-bodyMotionTime)*fraction;
        auto leftover=remaining*(1-fraction);float inward=glm::dot(leftover,hit.normal);
        if(inward<0)leftover-=hit.normal*inward;
        remaining=leftover;
        if(!legacyPush){auto surface=physics.GetLinearVelocity(hit.hitBody);float into=glm::dot(velocity-surface,hit.normal);if(into<0)velocity-=hit.normal*into;}
    }
}
void CharacterMotor::Reset(glm::vec3 p,glm::quat q){position=p;orientation=glm::normalize(q);velocity=acceleration=glm::vec3(0);result={};followingSupport=false;}
void CharacterMotor::Step(PhysicsWorld& physics,const GravityField& gravity,float dt){
    if(!settings.enabled){followingSupport=false;result={};acceleration={0,0,0};return;}
    auto oldPosition=position;auto previous=result;result={};result.gravity=gravity.Sample(position);
    const auto intentUp=orientation*glm::vec3(0,1,0);
    auto up=intentUp;if(glm::length(result.gravity)>1e-6f)up=-glm::normalize(result.gravity);
    auto delta=Align(orientation*glm::vec3(0,1,0),up);float angle=glm::angle(delta),limit=glm::radians(settings.reorientationDegreesPerSecond)*dt;
    if(angle>limit)delta=glm::angleAxis(limit,glm::axis(delta));
    orientation=glm::normalize(delta*orientation);result.up=up;
    auto center=position+orientation*settings.offset;
    filter.includeSensors=false;
    filter.requiredTags=settings.requiredTags;filter.excludedTags=settings.excludedTags;
    auto sweep=[&](glm::vec3 p,glm::vec3 d,bool interpolate=false,float start=0,float end=1){return physics.SweepCapsuleMotion(settings.radius,settings.halfHeight,p,orientation,d,interpolate,start,end,&filter,settings.collisionLayer,settings.collisionMask);};
    // Recover using signed actual geometry, not a skin-only displacement.
    for(int i=0;i<8;++i){auto hit=sweep(center,{0,0,0},true,0,0);if(!hit.hit||hit.penetration<=1e-5f)break;center+=hit.normal*(hit.penetration+settings.skin);result.recovered=true;}
    bool departing=glm::dot(velocity-previous.supportVelocity,intentUp)>1e-4f;
    if(departing||glm::dot(supportOrigin-center,up)>settings.stepHeight)followingSupport=false;
    auto ground=sweep(center,-up*settings.supportDistance,true,0,0);
    const float slope=std::cos(glm::radians(settings.maxSlopeDegrees));
    bool supported=!departing&&ground.hit&&glm::dot(ground.normal,up)>slope;
    auto shape=Shape::Capsule(settings.radius,settings.halfHeight);
    if(!supported&&!departing&&(previous.supported||followingSupport)){glm::vec3 p,n;BodyHandle body;
        if(TryStepDown(physics,center,orientation,up,settings.stepHeight,slope,settings.skin,p,n,body,&shape,&filter,settings.collisionLayer,settings.collisionMask)){
            center=p;ground.hit=true;ground.hitBody=body;ground.normal=n;ground.distance=settings.skin;supported=true;result.stepped=true;}}
    glm::vec3 carryVelocity(0);
    if(supported){followingSupport=true;supportOrigin=center;center+=up*(settings.skin-ground.distance);
        if(physics.IsDynamicBody(ground.hitBody)){
            auto a=physics.GetPreviousTransform(ground.hitBody),b=physics.GetTransform(ground.hitBody);
            auto target=b.position+b.rotation*(glm::inverse(a.rotation)*(center-a.position));
            // Sweep carried motion too: support cannot carry the capsule through a wall.
            auto carryFilter=filter;carryFilter.ignoredBodies.push_back(ground.hitBody);bool blocked=false;auto cv=velocity;
            ResolveCharacterSlide(physics,center,orientation,target-center,cv,settings,&carryFilter,false,false,blocked);result.collided|=blocked;
            carryVelocity=physics.GetLinearVelocity(ground.hitBody)+glm::cross(physics.GetAngularVelocity(ground.hitBody),center-b.position);
        }
        velocity+=carryVelocity-previous.supportVelocity;
        float inward=glm::dot(velocity-carryVelocity,ground.normal);if(inward<0)velocity-=ground.normal*inward;
    }
    velocity+=(result.gravity*settings.gravityScale+acceleration)*dt;acceleration={0,0,0};
    auto remaining=(velocity-carryVelocity)*dt;
    if(supported&&!departing){glm::vec3 stepped;auto tangent=remaining-up*glm::dot(remaining,up);
        if(TryStepMove(physics,center,orientation,up,tangent,settings.stepHeight,slope,settings.skin,stepped,&shape,&filter,settings.collisionLayer,settings.collisionMask)){center=stepped;remaining={0,0,0};result.stepped=true;}}
    ResolveCharacterSlide(physics,center,orientation,remaining,velocity,settings,&filter,!supported,false,result.collided);
    position=center-orientation*settings.offset;
    // End-of-step support reports the current pose, rather than yesterday's ground.
    auto final=sweep(center,-up*settings.supportDistance);
    result.supported=!departing&&final.hit&&glm::dot(final.normal,up)>slope;
    if(result.supported){result.support=final.hitBody;result.supportNormal=final.normal;result.supportVelocity=carryVelocity;}
    result.velocity=velocity;result.displacement=position-oldPosition;
}
