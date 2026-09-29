// UNEXECUTED IN THE RESEARCH ENVIRONMENT (GLM unavailable).
// Calls the actual ContactSolver, not the broadphase/world detector.
// Deliberately supplies one physical contact; no geometry/step-timing claim.
#include "ContactSolver.h"
#include "RigidBody.h"
#include <cmath>
#include <iomanip>
#include <iostream>
long double energy(const RigidBody& b) {
    const long double x=b.linearVelocity.x,y=b.linearVelocity.y,z=b.linearVelocity.z;
    const long double wx=b.angularVelocity.x,wy=b.angularVelocity.y,wz=b.angularVelocity.z;
    return (x*x+y*y+z*z)/2 + (wx*wx+wy*wy)/2 + wz*wz/3;
}
int main(){
    RigidBody a,b;
    a.inverseMass=1; a.inverseInertiaLocal=glm::mat3(1.0f); a.inverseInertiaLocal[2][2]=1.5f;
    a.position=glm::vec3(0,1,0); a.linearVelocity=glm::vec3(-.1f,-1,0);
    b.position=glm::vec3(0); // immovable plane reference, reaction external
    Contact c; c.hit=true;c.normal=glm::vec3(0,1,0);c.preciseNormal=glm::dvec3(0,1,0);
    c.point=glm::vec3(1,0,0);c.hasLocalAnchors=true;c.localAnchorA=glm::dvec3(1,-1,0);
    c.localAnchorB=glm::dvec3(1,0,0);c.signedSeparation=0;c.penetration=0;
    c.separationState=SeparationState::Touching;
    const auto e0=energy(a);
    ContactSolver s;s.AddContact(a,b,c,.8f,1.0f);s.Prepare(1.0f/60);s.SolveVelocities();
    const auto e1=energy(a);
    std::cout<<std::setprecision(20)<<"initial_energy "<<e0<<"\nfinal_energy "<<e1
       <<"\nenergy_change "<<e1-e0<<"\n";
    return e1>e0+1e-6L?1:0;
}
