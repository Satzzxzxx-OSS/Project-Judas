// Supporting gprof diagnostic only; acceptance remains the unchanged full suite.
#include "PhysicsWorld.h"
#include <vector>
int main() {
    for (int repetition=0; repetition<5; ++repetition) {
        PhysicsWorld world; world.Init();
        world.CreateStaticBox(glm::vec3(0,-1,0),glm::vec3(100,1,100),0.8f,0.05f);
        std::vector<BodyHandle> crates;
        for(int i=0;i<1500;++i){
            const float x=static_cast<float>(i%50)*1.5f-37.0f;
            const float z=static_cast<float>(i/50)*1.5f-22.0f;
            crates.push_back(world.CreateDynamicBox(glm::vec3(x,0.5f,z),glm::vec3(0.5f),5.0f,0.6f,0.15f));
        }
        for(int step=0;step<30;++step){
            for(auto h:crates)world.ApplyLinearAcceleration(h,glm::vec3(0,-9.81f,0),1.0f/60.0f);
            world.Step(1.0f/60.0f);
        }
    }
}
