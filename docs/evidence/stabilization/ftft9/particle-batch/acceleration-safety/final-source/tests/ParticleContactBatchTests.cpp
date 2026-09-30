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
    double finalEnergy=Energy(world,body,particles);
    if(dynamicPlatform) {
        const auto v=glm::dvec3(world.GetLinearVelocity(platform)),w=glm::dvec3(world.GetAngularVelocity(platform));
        finalEnergy+=.5*world.GetMass(platform)*glm::dot(v,v)+.5*glm::dot(w,glm::dmat3(world.GetInertiaWorld(platform))*w);
    }
    Check(finalEnergy<=50.00001,"fixed support cannot create finite-island kinetic energy");
    glm::dvec3 total=double(world.GetMass(body))*glm::dvec3(world.GetLinearVelocity(body));
    if(dynamicPlatform)total+=double(world.GetMass(platform))*glm::dvec3(world.GetLinearVelocity(platform));
    double worstNormal=0;
    for(const auto& p:particles){total+=double(p.mass)*glm::dvec3(p.velocity);worstNormal=std::max(worstNormal,std::abs(double(p.velocity.y)));}
    Check(glm::length(total-glm::dvec3(0,-100,0)-glm::dvec3(support))<2e-4,"supported batch momentum closes with measured external support");
    const auto stats=world.GetParticleContactStats();
    Check(!stats.iterationCapReached && stats.maximumClosingSpeed<=1e-5f*stats.speedScale,"auxiliary solve meets declared normal convergence criterion");
    Check(worstNormal<.002,"supported finite-mass batch converges to near-stationary normal particle velocity");
    Check(glm::length(world.GetLinearVelocity(body))<.002,"supported body remains near stationary in same solve");
    std::cout<<"SUPPORTED dynamic_platform="<<dynamicPlatform<<" particle_normal_max="<<worstNormal<<" support_y="<<support.y<<" body_vy="<<world.GetLinearVelocity(body).y<<" iterations="<<stats.iterations<<" closing_residual="<<stats.maximumClosingSpeed<<" scale="<<stats.speedScale<<" cap="<<stats.iterationCapReached<<" acceleration="<<stats.normalAccelerations<<" products="<<stats.normalMatrixProducts<<" breakdowns="<<stats.normalAccelerationBreakdowns<<'\n';
}
void PersistentGap(bool cached){
    PhysicsWorld world;world.Init();const float dt=1.f/60;
    world.CreateStaticBox({0,-.1f,0},{2,.1f,2},0,0);
    const auto body=world.CreateDynamicBox({0,cached?.5f:.501f,0},glm::vec3(.5f),1,0,0);
    if(cached){world.SetLinearVelocity(body,{0,.06f,0});world.Step(dt);}
    const auto before=world.GetTransform(body);const double gap=double(before.position.y)-.5;
    std::vector<PhysicsWorld::ContactParticle> particles{{{0,before.position.y+1,0},{0,-2,0},100}};
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows{{0,body,{0,before.position.y+.5f,0},{0,1,0},{0,0,0},true}};
    const double initialEnergy=Energy(world,body,particles);
    const glm::dvec3 initialMomentum=glm::dvec3(world.GetLinearVelocity(body))+100.0*glm::dvec3(particles[0].velocity);
    const float initialBodyVy=world.GetLinearVelocity(body).y;
    std::vector<glm::vec3> impulses;glm::vec3 support;
    world.SolveParticleContacts(particles,rows,impulses,&support,dt);
    const glm::dvec3 finalMomentum=glm::dvec3(world.GetLinearVelocity(body))+100.0*glm::dvec3(particles[0].velocity);
    Check(glm::length(finalMomentum-initialMomentum-glm::dvec3(support))<.002,"gap witness momentum closes with actual external support impulse");
    Check(Energy(world,body,particles)<=initialEnergy+1e-4,"gap witness has no unexplained kinetic energy creation");
    const auto stats=world.GetParticleContactStats();const auto velocity=world.GetLinearVelocity(body);
    Check(gap>0,"positive support-gap fixture is genuinely separated");
    Check(world.GetTransform(body).position==before.position,"gap support solve never advances or snaps pose");
    if(cached){
        Check(stats.persistentGapRows>0,"previous support identity retained across genuine positive gap");
        const double expectedSpeed=-gap/dt;
        const double expectedParticleImpulse=100*(expectedSpeed+2);
        const double expectedSupportImpulse=expectedParticleImpulse+expectedSpeed-initialBodyVy;
        Check(std::abs(impulses[0].y-expectedParticleImpulse)<.002,"stiff contact impulse matches independent two-mass constrained mechanics");
        Check(std::abs(support.y-expectedSupportImpulse)<.002,"stiff support load matches independent total momentum change");
        Check(stats.normalAccelerations>0 && !stats.iterationCapReached,"stiff witness uses bounded algebraic acceleration and converges");
        Check(velocity.y<-.001f && std::abs(velocity.y+gap/dt)<.0001,"persistent support permits physical gap closing without pinning velocity");
    }else{
        Check(stats.persistentGapRows==0 && stats.rigidContactRows==0,"new separated contact never becomes batch support");
        Check(velocity.y<-1,"uncached separated body retains ordinary free finite-mass response");
    }
    std::cout<<"GAP cached="<<cached<<" gap="<<gap<<" vy="<<velocity.y<<" rows="<<stats.persistentGapRows<<" residual="<<stats.maximumClosingSpeed<<" iterations="<<stats.iterations<<" acceleration="<<stats.normalAccelerations<<" products="<<stats.normalMatrixProducts<<" breakdowns="<<stats.normalAccelerationBreakdowns<<" initial_energy="<<initialEnergy<<" final_energy="<<Energy(world,body,particles)<<'\n';
}
void OpposingPrescribedWalls(){
    PhysicsWorld world;world.Init();const auto owner=world.CreateStaticBox({0,0,0},{1,1,1},0,0);
    std::vector<PhysicsWorld::ContactParticle> particles{{{0,0,0},{0,-.1635f,0},8}};
    std::vector<PhysicsWorld::ParticleBoundaryContact> rows{
        {0,owner,{0,0,0},{0,1,0},{0,0,0},false},
        {0,owner,{0,0,0},{0,-1,0},{0,-1,0},false}};
    std::vector<glm::vec3> impulses;world.SolveParticleContacts(particles,rows,impulses);
    const auto v=particles[0].velocity;const auto stats=world.GetParticleContactStats();
    // No velocity can satisfy v_y>=0 and v_y<=-1 simultaneously.
    // This prescribed-boundary fixture has external work; the oracle is
    // bounded finite iterative progress with an honestly reported residual.
    Check(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"incompatible prescribed normal rows remain finite");
    Check(glm::length(v)<=1.00001f,"one-particle opposing-wall sweep stays within prescribed wall speed range");
    Check(stats.iterationCapReached&&stats.maximumClosingSpeed>.9f,"incompatible wall constraints remain explicitly unsolved at bounded iteration cap");
    Check(stats.normalAccelerationBoundaryDeclines>0,"moving prescribed normal endpoint declines optional accelerator");
    Check(impulses.size()==2&&std::isfinite(impulses[0].y)&&std::isfinite(impulses[1].y),"incompatible boundary impulse diagnostics stay finite");
    std::cout<<"OPPOSING vy="<<v.y<<" energy="<<4.0*glm::dot(glm::dvec3(v),glm::dvec3(v))<<" residual="<<stats.maximumClosingSpeed<<" cap="<<stats.iterationCapReached<<" acceleration="<<stats.normalAccelerations<<" breakdowns="<<stats.normalAccelerationBreakdowns<<" rejected="<<stats.normalAccelerationRejections<<" declined="<<stats.normalAccelerationBoundaryDeclines<<'\n';
}
void NearOpposingPrescribedWalls(){
    for(float tilt:{1e-6f,1e-5f,.0001f,.001f,.01f}){
        PhysicsWorld world;world.Init();const auto owner=world.CreateStaticBox({0,0,0},{1,1,1},0,0);
        std::vector<PhysicsWorld::ContactParticle> particles{{{0,0,0},{0,-.1635f,0},8}};
        std::vector<PhysicsWorld::ParticleBoundaryContact> rows{
            {0,owner,{0,0,0},{0,1,0},{0,0,0},false},
            {0,owner,{0,0,0},glm::normalize(glm::vec3(tilt,-1,0)),{0,-1,0},false}};
        std::vector<glm::vec3> impulses;world.SolveParticleContacts(particles,rows,impulses);
        const auto v=particles[0].velocity;const auto stats=world.GetParticleContactStats();
        // Independent closed form for the existing sequential normal sweeps:
        // floor sets vy=0; tilted upper-wall projection gives
        // vx_(k+1)=cos^2(theta)*vx_k+sin(theta)*cos(theta).
        const double sin=double(tilt)/std::sqrt(1.0+double(tilt)*tilt),cos=1/std::sqrt(1.0+double(tilt)*tilt);
        double x=0,y=0;
        for(int k=0;k<64;++k){const double impulse=std::max(0.0,cos-sin*x);x+=sin*impulse;y=-cos*impulse;}
        Check(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"near-opposing prescribed walls remain finite");
        Check(std::abs(double(v.x)-x)<2e-5&&std::abs(double(v.y)-y)<2e-5,"declined accelerator retains unchanged bounded sequential projection endpoint");
        Check(stats.normalAccelerations==0&&stats.normalAccelerationBoundaryDeclines>0,"all moving prescribed-wall accelerator proposals are declined");
        Check(stats.iterationCapReached&&stats.maximumClosingSpeed>.9f,"near-opposing coarse geometry exposes unresolved normal residual");
        std::cout<<"NEAR_OPPOSING tilt="<<tilt<<" vx="<<v.x<<" vy="<<v.y<<" energy="<<4.0*glm::dot(glm::dvec3(v),glm::dvec3(v))<<" residual="<<stats.maximumClosingSpeed<<" cap="<<stats.iterationCapReached<<" declined="<<stats.normalAccelerationBoundaryDeclines<<'\n';
    }
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
int main(){Free(1,100,false);Free(100,1,false);Free(2,3,true);Supported(false);Supported(true);PersistentGap(false);PersistentGap(true);Prescribed();OpposingPrescribedWalls();NearOpposingPrescribedWalls();std::cout<<"FTFT9 PARTICLE BATCH checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;}
