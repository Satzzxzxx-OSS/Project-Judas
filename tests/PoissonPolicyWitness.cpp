// FTFT4B stop-condition probe. Actual Judas geometry/mass/inertia; exact
// two-row full-rank specialization of the selected normal PLUS rounds.
// This is NOT an implemented PhysicsWorld event handler or production pass.
#include "PhysicsWorld.h"
#include "Narrowphase.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <stdexcept>
struct State {glm::dvec3 p,v,w;double m;glm::dmat3 I;};
void vec(glm::dvec3 v){std::cout<<'['<<v.x<<','<<v.y<<','<<v.z<<']';}
double energy(const std::array<State,3>&s){double k=0;for(auto&b:s)k+=.5*(b.m*glm::dot(b.v,b.v)+glm::dot(b.w,b.I*b.w));return k;}
glm::dvec3 momentum(const std::array<State,3>&s){glm::dvec3 p(0);for(auto&b:s)p+=b.m*b.v;return p;}
glm::dvec3 angular(const std::array<State,3>&s){glm::dvec3 p(0);for(auto&b:s)p+=b.I*b.w+glm::cross(b.p,b.m*b.v);return p;}
int main(){
 std::cout<<std::setprecision(17);PhysicsWorld world;world.Init();
 std::vector<CompoundBox> parts={{{0,0,0},{.125f,.125f,.125f}},{{2,-.12109375f,0},{.00390625f,.00390625f,.00390625f}},{{-2,.12109375f,0},{.00390625f,.00390625f,.00390625f}},{{-.25f,-.12109375f,0},{.00390625f,.00390625f,.00390625f}},{{.25f,.12109375f,0},{.00390625f,.00390625f,.00390625f}}};
 std::array<BodyHandle,3> h={world.CreateDynamicCompoundBoxes({0,0,0},parts,100,0,0),world.CreateDynamicSphere({2,-.15625f,0},.03125f,1e6f,0,1),world.CreateDynamicSphere({-.25f,-.15625f,0},.03125f,1e6f,0,.5f)};
 world.SetLinearVelocity(h[0],{0,-19.f/9,0});world.SetAngularVelocity(h[0],{0,0,-4.f/9});
 std::array<State,3>s;std::array<Shape,3> shapes;std::array<RigidBody,3> poses;
 for(int i=0;i<3;++i){BodyTransform p;world.GetBodyShape(h[i],shapes[i],p);poses[i].position=p.position;poses[i].orientation=p.rotation;s[i]={glm::dvec3(p.position),glm::dvec3(world.GetLinearVelocity(h[i])),glm::dvec3(world.GetAngularVelocity(h[i])),world.GetMass(h[i]),glm::dmat3(world.GetInertiaWorld(h[i]))};}
 const auto before=s;std::array<glm::dvec3,2> n,ra,rb;int totalContacts=0;
 for(int k=0;k<2;++k){int count=0;
  for(int j=0;j<PrimitiveCount(shapes[0]);++j){auto manifold=ComputeContacts(PrimitiveAt(shapes[0],poses[0],j),PrimitiveAt(shapes[k+1],poses[k+1],0));
   for(int p=0;p<manifold.count;++p){const auto&c=manifold.points[p];++count;++totalContacts;n[k]=c.preciseNormal;ra[k]=ContactRotation(poses[0].orientation)*c.localAnchorA;rb[k]=ContactRotation(poses[k+1].orientation)*c.localAnchorB;
    std::cout<<"{\"contact\":"<<k<<",\"primitive\":"<<j<<",\"signed_separation\":"<<c.signedSeparation<<",\"normal\":";vec(n[k]);std::cout<<",\"rA\":";vec(ra[k]);std::cout<<",\"rB\":";vec(rb[k]);std::cout<<"}\n";
   }}if(count!=1)throw std::runtime_error("fixture must have exactly one actual contact per sphere");}
 const auto invI=glm::inverse(s[0].I);double A[2][2];
 for(int i=0;i<2;++i)for(int j=0;j<2;++j){A[i][j]=glm::dot(n[i],n[j])/s[0].m+glm::dot(glm::cross(ra[i],n[i]),invI*glm::cross(ra[j],n[j]));if(i==j)A[i][j]+=1/s[i+1].m+glm::dot(glm::cross(rb[i],n[i]),glm::inverse(s[i+1].I)*glm::cross(rb[i],n[i]));}
 auto velocity=[&](int k){return glm::dot(n[k],s[0].v+glm::cross(s[0].w,ra[k])-s[k+1].v-glm::cross(s[k+1].w,rb[k]));};
 std::cout<<"{\"A\":[["<<A[0][0]<<','<<A[0][1]<<"],["<<A[1][0]<<','<<A[1][1]<<"]],\"initial_u\":["<<velocity(0)<<','<<velocity(1)<<"],\"masses\":["<<s[0].m<<','<<s[1].m<<','<<s[2].m<<"],\"inertia_body\":[";
 for(int i=0;i<3;++i){if(i)std::cout<<',';vec(s[0].I[i]);}std::cout<<"],\"determinant\":"<<A[0][0]*A[1][1]-A[0][1]*A[1][0]<<"}\n";
 double pending[2]={0,0},e[2]={1,.5};int round=0;
 // This specialization has two independent normal rows, friction zero and
 // no active-set violations. Solve the unique full-rank system directly;
 // it is also the minimum-norm solution. No general rank-deficient claim.
 for(;round<16;++round){bool exp[2],comp[2];double u[2]={velocity(0),velocity(1)},p[2]={pending[0],pending[1]};int count=0;
  for(int k=0;k<2;++k){exp[k]=pending[k]>0;comp[k]=!exp[k]&&u[k]<-1e-10;count+=exp[k]||comp[k];}
  if(!count)break;
  if(comp[0]&&comp[1]){double det=A[0][0]*A[1][1]-A[0][1]*A[1][0];if(!(det>0))throw std::runtime_error("rank violation");p[0]=(-u[0]*A[1][1]+u[1]*A[0][1])/det;p[1]=(-u[1]*A[0][0]+u[0]*A[1][0])/det;}
  else for(int k=0;k<2;++k)if(comp[k])p[k]=(-u[k]-A[k][1-k]*p[1-k])/A[k][k];
  double residual=0;for(int k=0;k<2;++k){if(p[k]<0)throw std::runtime_error("attractive impulse outside this specialization");if(comp[k])residual=std::max(residual,std::abs(u[k]+A[k][0]*p[0]+A[k][1]*p[1]));pending[k]=comp[k]&&-u[k]>.5?e[k]*p[k]:0;}
  for(int k=0;k<2;++k){auto impulse=p[k]*n[k];s[0].v+=impulse/s[0].m;s[0].w+=invI*glm::cross(ra[k],impulse);s[k+1].v-=impulse/s[k+1].m;s[k+1].w-=glm::inverse(s[k+1].I)*glm::cross(rb[k],impulse);}
  std::cout<<"{\"round\":"<<round<<",\"compressing\":["<<comp[0]<<','<<comp[1]<<"],\"expanding\":["<<exp[0]<<','<<exp[1]<<"],\"before_u\":["<<u[0]<<','<<u[1]<<"],\"impulse\":["<<p[0]<<','<<p[1]<<"],\"after_u\":["<<velocity(0)<<','<<velocity(1)<<"],\"energy\":"<<energy(s)<<",\"compression_residual\":"<<residual<<"}\n";
 }
 if(round==16)throw std::runtime_error("budget exhausted");
 const double gain=energy(s)-energy(before);const auto dp=momentum(s)-momentum(before),dl=angular(s)-angular(before);
 for(int i=0;i<3;++i){std::cout<<"{\"body\":"<<i<<",\"before_v\":";vec(before[i].v);std::cout<<",\"before_w\":";vec(before[i].w);std::cout<<",\"after_v\":";vec(s[i].v);std::cout<<",\"after_w\":";vec(s[i].w);std::cout<<"}\n";}
 std::cout<<"{\"initial_energy\":"<<energy(before)<<",\"final_energy\":"<<energy(s)<<",\"gain\":"<<gain<<",\"momentum_residual\":"<<glm::length(dp)<<",\"angular_momentum_residual\":"<<glm::length(dl)<<",\"contacts\":"<<totalContacts<<",\"rounds\":"<<round<<",\"energy_failure\":"<<(gain>1e-4?"true":"false")<<"}\n";
 return gain>1e-4?1:0;
}
