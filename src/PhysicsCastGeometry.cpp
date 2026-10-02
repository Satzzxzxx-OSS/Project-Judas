#include "PhysicsCastGeometry.h"
#include "RadialTerrain.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
constexpr double tolerance=1e-5; // metres, query convergence only; no simulation skin
struct Distance {double gap=0;glm::dvec3 normal{0},point{0};};

// Exact segment/AABB closest pair: distance squared is a piecewise quadratic.
// Evaluate interval endpoints and each quadratic minimum. Unlike the player's
// eight alternating projections, this supports long public casts without an
// overestimated separation stepping past a thin collider. Player code is untouched.
Distance SegmentBox(glm::dvec3 a,glm::dvec3 b,double radius,glm::dvec3 h) {
    const auto v=b-a;
    std::vector<double> cuts{0,1};
    for(int k=0;k<3;++k)if(v[k]!=0)for(double face:{-h[k],h[k]}) {
        const double t=(face-a[k])/v[k];if(t>0&&t<1)cuts.push_back(t);
    }
    std::sort(cuts.begin(),cuts.end());
    double best=std::numeric_limits<double>::infinity();glm::dvec3 p{0},q{0};
    auto evaluate=[&](double t){auto x=a+t*v,y=glm::clamp(x,-h,h);double d=glm::dot(x-y,x-y);if(d<best){best=d;p=x;q=y;}};
    for(size_t i=1;i<cuts.size();++i){double lo=cuts[i-1],hi=cuts[i],mid=(lo+hi)*.5,A=0,B=0;
        for(int k=0;k<3;++k){double x=a[k]+mid*v[k];if(x<-h[k]||x>h[k]){double c=a[k]-(x<0?-h[k]:h[k]);A+=v[k]*v[k];B+=v[k]*c;}}
        evaluate(lo);evaluate(hi);if(A>0)evaluate(std::clamp(-B/A,lo,hi));
    }
    if(best>0){double d=std::sqrt(best);return {d-radius,(p-q)/d,q};}
    auto depths=h-glm::abs(p);int k=0;if(depths.y<depths[k])k=1;if(depths.z<depths[k])k=2;
    glm::dvec3 n{0};n[k]=p[k]>=0?1:-1;q=p;q[k]=n[k]*h[k];
    return {-depths[k]-radius,n,q};
}
Distance SegmentSphere(glm::dvec3 a,glm::dvec3 b,double radius,double targetRadius) {
    auto v=b-a;double length2=glm::dot(v,v),t=length2>0?std::clamp(-glm::dot(a,v)/length2,0.,1.):0;
    auto p=a+t*v;double d=glm::length(p);glm::dvec3 n=d>0?p/d:glm::dvec3(1,0,0);
    return {d-radius-targetRadius,n,n*targetRadius};
}
PrimitiveCastHit MakeHit(double t,const Distance& d,bool overlap) {
    return {true,overlap,t,glm::vec3(d.point),glm::vec3(d.normal)};
}
}

PrimitiveCastHit CastAgainstPrimitive(const Shape& cast,const BodyTransform& pose,
    const glm::vec3& direction,double maximum,const PrimitivePose& target) {
    const auto targetR=ContactRotation(target.parentOrientation);
    const glm::dvec3 targetCenter=glm::dvec3(target.parentPosition)+targetR*glm::dvec3(target.parentLocalCenter);
    const auto queryR=ContactRotation(pose.rotation);
    const glm::dvec3 origin=glm::dvec3(pose.position)-targetCenter,dir=glm::normalize(glm::dvec3(direction));
    auto distance=[&](double t) {
        glm::dvec3 center=origin+dir*t;
        const double half=cast.type==ShapeType::Capsule?cast.halfHeight:0;
        glm::dvec3 a=center-queryR[1]*half,b=center+queryR[1]*half;
        Distance d;
        if(cast.type!=ShapeType::Box) {
            if(target.shape.type==ShapeType::Sphere)d=SegmentSphere(a,b,cast.radius,target.shape.radius);
            else {d=SegmentBox(glm::transpose(targetR)*a,glm::transpose(targetR)*b,cast.radius,glm::dvec3(target.shape.halfExtents));d.normal=targetR*d.normal;d.point=targetR*d.point;}
        } else if(target.shape.type==ShapeType::Sphere) {
            // Reverse sphere/box distance and convert its target witness back to
            // the sphere surface. This is the same physical sphere/OBB geometry.
            auto sphereLocal=glm::transpose(queryR)*(-center);
            auto reverse=SegmentBox(sphereLocal,sphereLocal,target.shape.radius,glm::dvec3(cast.halfExtents));
            d.gap=reverse.gap;d.normal=-queryR*reverse.normal;d.point=d.normal*double(target.shape.radius);
        } else {
            // Existing robust SAT/contact geometry supplies a separating plane
            // (a lower bound on distance) and real target-local surface witness.
            ContactPose aPose{glm::vec3(center),pose.rotation,{0,0,0}};
            ContactPose bPose{{0,0,0},target.parentOrientation,target.parentLocalCenter};
            // Use parent-relative centres without world float rounding.
            aPose.position=glm::vec3(glm::dvec3(pose.position)+dir*t-glm::dvec3(target.parentPosition));
            auto manifold=PrimitiveContacts(cast,aPose,target.shape,bPose,std::numeric_limits<float>::max());
            if(!manifold.count)throw std::runtime_error("box cast could not evaluate contact geometry");
            const auto& contact=manifold.points[0];d.gap=contact.signedSeparation;d.normal=contact.preciseNormal;
            d.point=targetR*(contact.localWitnessB-glm::dvec3(target.parentLocalCenter));
        }
        d.point+=targetCenter;return d;
    };
    if(target.shape.type==ShapeType::Terrain) {
        if(cast.type==ShapeType::Box)throw std::invalid_argument("box casts against radial terrain are unsupported; use sphere/capsule casts");
        // Same authoritative radial surface and nine core samples as the player
        // query. Terrain has approximate distance, no supplied Lipschitz bound:
        // bracket crossings at a declared finite resolution, not fake exact CCD.
        auto sample=[&](double t){Distance best;best.gap=std::numeric_limits<double>::infinity();
            for(int i=0;i<9;++i){const auto local=glm::transpose(targetR)*(origin+dir*t+queryR[1]*double(cast.halfHeight)*(i/4.-1));
                auto s=target.shape.terrain->Sample(glm::vec3(local));double gap=s.signedDistance-cast.radius;
                if(gap<best.gap)best={gap,targetR*glm::dvec3(s.outwardNormal),targetCenter+targetR*glm::dvec3(s.surfacePoint)};}
            return best;};
        auto first=sample(0);if(first.gap<=0)return MakeHit(0,first,true);
        // Restrict sampling to the target's actual conservative bounding sphere.
        double radius=target.shape.terrain->BoundRadius()+cast.radius+cast.halfHeight;
        double projection=glm::dot(origin,dir),disc=projection*projection-glm::dot(origin,origin)+radius*radius;
        if(disc<0)return {};
        double lo=std::max(0.,-projection-std::sqrt(disc)),end=std::min(maximum,-projection+std::sqrt(disc));
        if(end<lo)return {};
        for(int i=0;i<=512;++i){double t=lo+(end-lo)*i/512.;auto d=sample(t);
            if(d.gap<=0){double lower=i?lo+(end-lo)*(i-1)/512.:0,upper=t;
                for(int j=0;j<32;++j){double mid=(lower+upper)*.5;if(sample(mid).gap<=0)upper=mid;else lower=mid;}
                return MakeHit(upper,sample(upper),false);}}
        return {};
    }
    // A ray is a radius-zero point. Analytic primitives avoid grazing convergence
    // and detect arbitrarily thin boxes regardless of cast length.
    if(cast.type==ShapeType::Sphere&&cast.radius==0) {
        if(target.shape.type==ShapeType::Sphere) {
            auto start=distance(0);if(start.gap<=0)return MakeHit(0,start,true);
            double projection=glm::dot(origin,dir),disc=projection*projection-glm::dot(origin,origin)+double(target.shape.radius)*target.shape.radius;
            if(disc<0)return {};
            double t=-projection-std::sqrt(disc);if(t<0||t>maximum)return {};return MakeHit(t,distance(t),false);
        }
        auto o=glm::transpose(targetR)*origin,v=glm::transpose(targetR)*dir,h=glm::dvec3(target.shape.halfExtents);
        auto start=distance(0);if(start.gap<=0)return MakeHit(0,start,true);
        double entry=0,exit=maximum;
        for(int k=0;k<3;++k){if(v[k]==0){if(o[k]<-h[k]||o[k]>h[k])return {};continue;}
            double t0=(-h[k]-o[k])/v[k],t1=(h[k]-o[k])/v[k];if(t0>t1)std::swap(t0,t1);entry=std::max(entry,t0);exit=std::min(exit,t1);if(entry>exit)return {};}
        return MakeHit(entry,distance(entry),false);
    }
    double t=0;
    for(int i=0;i<256;++i){auto d=distance(t);if(d.gap<=tolerance)return MakeHit(t,d,t==0&&d.gap<=0);
        // Fixed orientations: the current separating plane remains valid until
        // translation closes its gap. Never step beyond that plane's crossing.
        double closing=-glm::dot(dir,d.normal);if(closing<=0)return {};
        double next=t+d.gap/closing;if(next>maximum+tolerance)return {};t=std::min(next,maximum);
    }
    throw std::runtime_error("shape cast did not converge within 256 iterations");
}
