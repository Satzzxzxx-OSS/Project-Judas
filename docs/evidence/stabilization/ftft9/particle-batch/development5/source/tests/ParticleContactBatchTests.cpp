// Independent finite-mass contact/support checks for the FTFT9 batch adapter.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "PhysicsWorld.h"
namespace {
int checks=0,failures=0;
void Check(bool c,const char* label){++checks;if(!c){++failures;std::cerr<<"FAIL "<<label<<'\n';}}
bool Near(glm::vec3 a,glm::vec3 b,float tolerance=2e-5f){return glm::length(a-b)<=tolerance;}
double Energy(const PhysicsWorld& world,BodyHandle body,const std::vector<PhysicsWorld::ContactParticle>& particles){
    const auto v=glm::dvec3(world.GetLinearVelocity(body)),w=glm::dvec3(world.GetAngularVelocity(body));
    double e=.5*world.GetMass(body)*glm::dot(v,v)+.5*glm::dot(w,glm::dmat3(world.GetInertiaWorld(body))*w);
    for(const auto& p:particles)e+=.5*double(p.mass)*glm::dot(glm::dvec3(p.velocity),glm::dvec3(p.velocity));
    return e;
}
void Free(float bodyMass,float particleMass,bool offCenter){
    PhysicsWorld world;world.Init();const auto body=world.CreateDynamicBox({0,0,0},glm::vec3(.5f),bodyMass,0,0);
    const float r=offCenter?.4f:0;
    std::vector<PhysicsWorld::ContactParticle> particles{{{r,1,0},{0,-2,0},particleMass}};
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows{{0,body,{r,.5f,0},{0,1,0},{0,0,0},true}};
    std::vector<glm::vec3> impulses;glm::vec3 support;
    const double before=Energy(world,body,particles);const auto pose=world.GetTransform(body);
    world.ApplyForce(body,{3,0,0});
    world.SolveParticleContacts(particles,rows,impulses,&support);
    const double inverseInertia=6.0/bodyMass; // unit-sided uniform box Izz=m/6
    const double impulse=2.0/(1.0/particleMass+1.0/bodyMass+r*r*inverseInertia);
    Check(Near(impulses[0],{0,float(impulse),0}),"free contact impulse equals analytical reduced-mass value");
    Check(Near(particles[0].velocity,{0,float(-2+impulse/particleMass),0}),"particle velocity follows same impulse");
    Check(Near(world.GetLinearVelocity(body),{0,float(-impulse/bodyMass),0}),"body linear reaction is equal/opposite");
    Check(Near(world.GetAngularVelocity(body),{0,0,float(-r*impulse*inverseInertia)}),"off-center reaction matches independent torque/inertia");
    Check(Energy(world,body,particles)<=before+1e-5,"inelastic finite-mass contact cannot create energy");
    Check(support==glm::vec3(0),"free body has no invented static support");
    Check(world.GetTransform(body).position==pose.position && world.GetTransform(body).rotation==pose.rotation,"batch leaves authoritative pose unchanged");
    const auto beforeVelocity=world.GetLinearVelocity(body);world.Step(1.f/60);
    Check(std::abs(world.GetLinearVelocity(body).x-beforeVelocity.x-3.f/bodyMass/60)<2e-6,"batch preserves force accumulator for ordinary step");
}
void Supported(bool dynamicPlatform){
    PhysicsWorld world;world.Init();world.CreateStaticBox({0,-.1f,0},{1,0,0,0},{3,.1f,3},0,0);
    BodyHandle platform;
    const float bottom=dynamicPlatform?1.f:0;
    if(dynamicPlatform)platform=world.CreateDynamicBox({0,.5f,0},{1,.5f,1},400,0,0);
    const auto body=world.CreateDynamicBox({0,bottom+.5f,0},glm::vec3(.5f),200,0,0);
    std::vector<PhysicsWorld::ContactParticle> particles;
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows;
    for(int x=-2;x<=2;++x)for(int z=-2;z<=2;++z){const glm::vec3 point(x*.15f,bottom+1,z*.15f);
        particles.push_back({point+glm::vec3(0,.05f,0),{0,-1,0},4});rows.push_back({particles.size()-1,body,point,{0,1,0},{0,0,0},true});}
    std::vector<glm::vec3> impulses;glm::vec3 support;world.SolveParticleContacts(particles,rows,impulses,&support);
    glm::dvec3 total=double(world.GetMass(body))*glm::dvec3(world.GetLinearVelocity(body));
    if(dynamicPlatform)total+=double(world.GetMass(platform))*glm::dvec3(world.GetLinearVelocity(platform));
    double worstNormal=0;
    for(const auto& p:particles){total+=double(p.mass)*glm::dvec3(p.velocity);worstNormal=std::max(worstNormal,std::abs(double(p.velocity.y)));}
    Check(glm::length(total-glm::dvec3(0,-100,0)-glm::dvec3(support))<2e-4,"supported batch momentum closes with measured external support");
    const auto stats=world.GetParticleContactStats();
    Check(!stats.iterationCapReached && stats.maximumClosingSpeed<=1e-5f*stats.speedScale,"auxiliary solve meets declared normal convergence criterion");
    Check(worstNormal<.002,"supported finite-mass batch converges to near-stationary normal particle velocity");
    Check(glm::length(world.GetLinearVelocity(body))<.002,"supported body remains near stationary in same solve");
    std::cout<<"SUPPORTED dynamic_platform="<<dynamicPlatform<<" particle_normal_max="<<worstNormal<<" support_y="<<support.y<<" body_vy="<<world.GetLinearVelocity(body).y<<" iterations="<<stats.iterations<<" closing_residual="<<stats.maximumClosingSpeed<<" scale="<<stats.speedScale<<" cap="<<stats.iterationCapReached<<'\n';
}
void PersistentGap(bool cached){
    PhysicsWorld world;world.Init();const float dt=1.f/60;
    world.CreateStaticBox({0,-.1f,0},{2,.1f,2},0,0);
    const auto body=world.CreateDynamicBox({0,cached?.5f:.501f,0},glm::vec3(.5f),1,0,0);
    if(cached){world.SetLinearVelocity(body,{0,.06f,0});world.Step(dt);}
    const auto before=world.GetTransform(body);const double gap=double(before.position.y)-.5;
    std::vector<PhysicsWorld::ContactParticle> particles{{{0,before.position.y+1,0},{0,-2,0},100}};
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows{{0,body,{0,before.position.y+.5f,0},{0,1,0},{0,0,0},true}};
    std::vector<glm::vec3> impulses;world.SolveParticleContacts(particles,rows,impulses,nullptr,dt);
    const auto stats=world.GetParticleContactStats();const auto velocity=world.GetLinearVelocity(body);
    Check(gap>0,"positive support-gap fixture is genuinely separated");
    Check(world.GetTransform(body).position==before.position,"gap support solve never advances or snaps pose");
    if(cached){
        Check(stats.persistentGapRows>0,"previous support identity retained across genuine positive gap");
        Check(velocity.y<-.001f && std::abs(velocity.y+gap/dt)<.0001,"persistent support permits physical gap closing without pinning velocity");
    }else{
        Check(stats.persistentGapRows==0 && stats.rigidContactRows==0,"new separated contact never becomes batch support");
        Check(velocity.y<-1,"uncached separated body retains ordinary free finite-mass response");
    }
    std::cout<<"GAP cached="<<cached<<" gap="<<gap<<" vy="<<velocity.y<<" rows="<<stats.persistentGapRows<<" residual="<<stats.maximumClosingSpeed<<'\n';
}
void Prescribed(){
    PhysicsWorld world;world.Init();const auto body=world.CreateDynamicBox({0,0,0},glm::vec3(.5f),1,0,0);world.SetLinearVelocity(body,{4,5,6});
    std::vector<PhysicsWorld::ContactParticle> particles{{{0,1,0},{0,-1,0},100}};
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows{{0,body,{0,.5f,0},{0,1,0},{0,.25f,0},false}};
    std::vector<glm::vec3> impulses;world.SolveParticleContacts(particles,rows,impulses);
    Check(Near(particles[0].velocity,{0,.25f,0}),"one-way wall uses its prescribed velocity");
    Check(world.GetLinearVelocity(body)==glm::vec3(4,5,6),"one-way wall never changes actual rigid velocity");
    world.DestroyBody(body);const auto replacement=world.CreateDynamicBox({0,0,0},glm::vec3(.5f),1,0,0);
    rows[0].twoWay=true;const auto before=particles[0].velocity;bool rejected=false;
    try{world.SolveParticleContacts(particles,rows,impulses);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected&&particles[0].velocity==before&&world.GetLinearVelocity(replacement)==glm::vec3(0),"stale owner rejected without particle/new-slot mutation");
}
}
int main(){Free(1,100,false);Free(100,1,false);Free(2,3,true);Supported(false);Supported(true);PersistentGap(false);PersistentGap(true);Prescribed();std::cout<<"FTFT9 PARTICLE BATCH checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;}
