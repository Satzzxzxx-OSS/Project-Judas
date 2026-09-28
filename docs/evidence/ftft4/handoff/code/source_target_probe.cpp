// Generated from the uploaded source. This executes only the scalar target branch,
// not PhysicsWorld, GLM, contact generation, or the full contact solver.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
constexpr float kRestitutionVelocityThreshold = 0.5f;
float SourceTarget(float closingSpeed,float restitution,float penetration,float fixedDeltaTime){
float target=0;
        const bool bounces = -closingSpeed > kRestitutionVelocityThreshold;
        const float gap = std::max(-penetration, 0.0f);
        if (gap <= 0.0f) {
            target = bounces ? -restitution * closingSpeed : 0.0f;
        } else {
            // Speculative contact (Milestone 32): the pair is `gap` apart.
            // It may close exactly that gap this step (v_n >= -gap/dt) but
            // not penetrate. Restitution applies only if this step's
            // approach actually reaches the surface.
            const bool reaches = fixedDeltaTime > 0.0f && -closingSpeed * fixedDeltaTime > gap;
            if (reaches && bounces) {
                target = -restitution * closingSpeed;
            } else {
                target = fixedDeltaTime > 0.0f ? -gap / fixedDeltaTime : 0.0f;
            }
        }
return target;
}
int main(){
std::cout<<std::setprecision(17);
std::cout<<"gap,speed,e,dt,source_target,source_end_gap,physical_end_speed,physical_end_gap\n";
for(float gap:{0.001f,0.05f,0.1f}) for(float e:{0.0f,0.5f,1.0f}){
float q=gap==0.001f?1.0f:6.38f;float dt=1.0f/60.0f;
float target=SourceTarget(-q,e,-gap,dt);
double tau=double(gap)/double(q);double v=e*q;
std::cout<<gap<<','<<q<<','<<e<<','<<dt<<','<<target<<','<<double(gap)+target*dt<<','<<v<<','<<v*(double(dt)-tau)<<'\n';
}
// Genuine separated contacts below speed threshold retain a negative gap-closing target.
float gap=0.001f,q=0.1f,dt=1.0f/60.0f;
float target=SourceTarget(-q,0,-gap,dt);
std::cout<<gap<<','<<q<<",0,"<<dt<<','<<target<<','<<double(gap)+target*dt<<",0,0\n";
}
