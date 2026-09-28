// Source-specific integration witness for the real Judas PhysicsWorld.
// NOT executed by ChatGPT: GLM is unavailable in its container.
// No test-only replacement of collision, solver, or integration.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include "PhysicsWorld.h"

int main() {
    std::cout << std::setprecision(17);
    const float h = 1.0f / 60.0f;
    for (float e : {0.0f, 0.5f}) {
        PhysicsWorld w; w.Init();
        w.CreateStaticBox(glm::vec3(0,-0.5f,0), glm::vec3(10,0.5f,10), 0, 0);
        const BodyHandle b = w.CreateDynamicSphere(glm::vec3(0,0.501f,0),0.5f,1,0,e);
        w.SetLinearVelocity(b,glm::vec3(0,-1,0));
        // Derive the oracle from the ACTUAL represented initial center.
        const double gap = double(w.GetTransform(b).position.y) - 0.5;
        const double expectedGap = double(e) * (double(h) - gap);
        for (int step=1;step<=4;++step) {
            w.Step(h);  // No gravity: detects indefinite, not merely one-frame stopping.
            auto pose=w.GetTransform(b); auto vel=w.GetLinearVelocity(b);
            std::cout << "impact," << e << ',' << step << ',' << gap << ','
                      << double(pose.position.y)-0.5 << ',' << vel.y << ','
                      << w.LastStepContactCount() << ',' << expectedGap << '\n';
        }
    }
    // Positive represented gap, a tangent velocity large enough for candidate
    // generation, but the normal velocity closes only 1/4 of the gap this step.
    for (float T : {0.0f,137.0f,1024.0f}) {
        PhysicsWorld w; w.Init();
        w.CreateStaticBox(glm::vec3(T,0,0),glm::vec3(.5f,10,10),0,0);
        float x=std::nextafter(std::nextafter(T+1.0f,
            std::numeric_limits<float>::infinity()),std::numeric_limits<float>::infinity());
        const double gap=double(x)-double(T)-1;
        const float q=float(gap/(4.0*double(h)));
        auto b=w.CreateDynamicSphere(glm::vec3(x,0,0),.5f,1,0,0);
        w.SetLinearVelocity(b,glm::vec3(-q,1,0));
        w.Step(h);
        auto v=w.GetLinearVelocity(b);
        std::cout << "gap," << T << ',' << gap << ',' << -q << ',' << v.x << ','
                  << w.LastStepContactCount();
        for (const auto &c:w.LastStepContacts()) std::cout << ',' << c.penetration;
        std::cout << '\n';
    }
    // Two impacts in one frame. Initial bounds/velocities must not suppress B-C.
    {
        PhysicsWorld w; w.Init();
        auto a=w.CreateDynamicSphere(glm::vec3(0,0,0),.5f,1,0,1);
        auto b=w.CreateDynamicSphere(glm::vec3(1.8f,0,0),.5f,1,0,1);
        auto c=w.CreateDynamicSphere(glm::vec3(3.6f,0,0),.5f,1,0,1);
        w.SetLinearVelocity(a,glm::vec3(10,0,0)); w.Step(.2f);
        for(auto x:{a,b,c}) std::cout << "chain," << w.GetTransform(x).position.x
                                     << ',' << w.GetLinearVelocity(x).x << '\n';
    }
}
