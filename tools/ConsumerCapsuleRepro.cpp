// Review evidence, not a new physics implementation. Independent analytic
// expectations expose M65's query-target capsule defect without repairing it.
#include "PhysicsWorld.h"
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <memory>
int main(){auto owned=std::make_unique<PhysicsWorld>();auto& world=*owned;if(!world.Init())return 2;auto body=world.CreateQueryCapsule(.3f,.6f,{{0,0,0},{1,0,0,0}});
    for(auto origin:{glm::vec3(-2,0,0),glm::vec3(-2,.4f,0),glm::vec3(0,2,0)}){
        const auto direction=origin.x<0?glm::vec3(1,0,0):glm::vec3(0,-1,0);
        auto hit=world.Raycast(origin,direction,4);const float expected=origin.x<0?1.7f:1.1f;
        std::cout<<"origin "<<origin.x<<' '<<origin.y<<' '<<origin.z<<" expected-distance "<<expected<<" actual-hit "<<hit.hit<<" distance "<<hit.distance<<" body "<<hit.body.id<<" capsule "<<body.id<<'\n';
    }
    auto overlap=world.Raycast({.2f,.4f,0},{1,0,0},1);
    std::cout<<"interior expected-initial-overlap 1 actual-hit "<<overlap.hit<<" initial-overlap "<<overlap.initialOverlap<<'\n';
    return 0; // A reproduction reports observations; it is not a passing fix test.
}
