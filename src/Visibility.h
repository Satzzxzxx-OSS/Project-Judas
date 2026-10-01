#pragma once
#include <array>
#include <limits>
#include <glm/glm.hpp>

// Presentation bounds only. Invalid/unknown bounds fail open.
struct VisualBounds {
    glm::vec3 min{std::numeric_limits<float>::infinity()};
    glm::vec3 max{-std::numeric_limits<float>::infinity()};
    void Include(glm::vec3 p) { min=glm::min(min,p); max=glm::max(max,p); }
    bool Valid() const { return min.x<=max.x && min.y<=max.y && min.z<=max.z; }
};
inline VisualBounds TransformBounds(const VisualBounds& b,const glm::mat4& m) {
    VisualBounds out;
    if(!b.Valid())return out;
    for(int i=0;i<8;++i)out.Include(glm::vec3(m*glm::vec4(glm::vec3(i&1?b.max.x:b.min.x,i&2?b.max.y:b.min.y,i&4?b.max.z:b.min.z),1)));
    return out;
}
class Frustum {
public:
    explicit Frustum(const glm::mat4& clip=glm::mat4(1)) {
        const auto t=glm::transpose(clip);
        for(int i=0;i<3;++i){planes[2*i]=t[3]+t[i];planes[2*i+1]=t[3]-t[i];}
        for(auto& p:planes){float n=glm::length(glm::vec3(p));if(n>0)p/=n;}
    }
    bool IsVisible(const VisualBounds& b)const {
        if(!b.Valid())return true;
        for(auto p:planes){glm::vec3 v(p.x>=0?b.max.x:b.min.x,p.y>=0?b.max.y:b.min.y,p.z>=0?b.max.z:b.min.z);
            // Conservative float margin; not a scene-dependent threshold.
            float margin=32*std::numeric_limits<float>::epsilon()*(1+glm::dot(glm::abs(glm::vec3(p)),glm::abs(v))+std::abs(p.w));
            if(glm::dot(glm::vec3(p),v)+p.w < -margin)return false;}
        return true;
    }
private: std::array<glm::vec4,6> planes;
};
