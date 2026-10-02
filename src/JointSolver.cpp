#include "JointSolver.h"
#include <algorithm>
#include <limits>
void JointSolver::Apply(Row& r,float p){r.a->linearVelocity+=r.linearA*(p*r.a->inverseMass);r.b->linearVelocity+=r.linearB*(p*r.b->inverseMass);
 r.a->angularVelocity+=r.inertiaA*r.angularA*p;r.b->angularVelocity+=r.inertiaB*r.angularB*p;}
void JointSolver::Prepare(const std::vector<JointInput>& inputs,float dt,bool warm,bool impact){
 m_rows.clear();m_bodies.clear();const float inf=std::numeric_limits<float>::infinity();
 for(auto& input:inputs){auto& s=input.state->settings;auto& a=*input.a;auto& b=*input.b;if(!impact)input.state->motorImpulse=0;
  for(auto* body:{&a,&b})if(std::find(m_bodies.begin(),m_bodies.end(),body)==m_bodies.end())m_bodies.push_back(body);
  glm::quat qa=glm::normalize(a.orientation),qb=glm::normalize(b.orientation),fa=qa*glm::normalize(s.frameA),fb=qb*glm::normalize(s.frameB);
  auto basis=glm::mat3_cast(fa);auto ra=qa*s.anchorA,rb=qb*s.anchorB,delta=(b.position-a.position)+rb-ra;
  auto ia=a.InverseInertiaWorld(),ib=b.InverseInertiaWorld();int index=0;
  auto row=[&](glm::vec3 la,glm::vec3 lb,glm::vec3 aa,glm::vec3 ab,float error,float low=-std::numeric_limits<float>::infinity(),float high=std::numeric_limits<float>::infinity(),bool soft=false,bool motor=false){
   float gamma=0,bias=impact?0:.2f*error/dt;
   if(soft){float denominator=dt*(s.damping+dt*s.stiffness);if(denominator<=0)return;gamma=1/denominator;bias=error*dt*s.stiffness*gamma;}
   if(motor)bias=-s.speed;
   float k=a.inverseMass*glm::dot(la,la)+b.inverseMass*glm::dot(lb,lb)+glm::dot(aa,ia*aa)+glm::dot(ab,ib*ab);
   if(k+gamma<=0)return;
   float* cache=impact?nullptr:&(*input.warm)[index];float lambda=warm&&cache?std::clamp(*cache,low,high):0;
   m_rows.push_back({&a,&b,la,lb,aa,ab,ia,ib,1/(k+gamma),bias,gamma,low,high,lambda,cache,motor?&input.state->motorImpulse:nullptr});++index;
   if(lambda!=0)Apply(m_rows.back(),lambda);
  };
  auto linear=[&](glm::vec3 n,float error,float low=-std::numeric_limits<float>::infinity(),float high=std::numeric_limits<float>::infinity(),bool soft=false,bool motor=false){row(-n,n,-glm::cross(ra,n)+glm::cross(n,delta),glm::cross(rb,n),error,low,high,soft,motor);};
  auto angular=[&](glm::vec3 n,float error,float low=-std::numeric_limits<float>::infinity(),float high=std::numeric_limits<float>::infinity(),bool soft=false,bool motor=false){row({0,0,0},{0,0,0},-n,n,error,low,high,soft,motor);};
  if(s.type==JointType::Slider){for(int k=1;k<3;++k)linear(basis[k],glm::dot(delta,basis[k]));}
  else for(int k=0;k<3;++k)linear(basis[k],glm::dot(delta,basis[k]));
  if(s.type==JointType::Fixed||s.type==JointType::Slider){auto q=glm::normalize(fb*glm::inverse(fa));if(q.w<0)q=-q;glm::vec3 v(q.x,q.y,q.z);float length=glm::length(v);auto error=length>1e-7f?v*(2*std::atan2(length,q.w)/length):2.f*v;
   for(int k=0;k<3;++k)angular(basis[k],glm::dot(error,basis[k]));}
  if(s.type==JointType::Hinge){auto axisB=fb*glm::vec3(1,0,0);auto error=glm::cross(basis[0],axisB);
   for(int k=1;k<3;++k)angular(basis[k],glm::dot(error,basis[k]));}
  if(s.type==JointType::Hinge||s.type==JointType::Slider){
   float coordinate=s.type==JointType::Slider?glm::dot(delta,basis[0]):std::atan2(glm::dot(basis[0],glm::cross(basis[1],fb*glm::vec3(0,1,0))),glm::dot(basis[1],fb*glm::vec3(0,1,0)));
   input.state->coordinate=coordinate;
   auto axisRow=[&](float error,float low,float high,bool soft=false,bool motor=false){if(s.type==JointType::Slider)linear(basis[0],error,low,high,soft,motor);else angular(basis[0],error,low,high,soft,motor);};
   // Predict limit crossing from current velocity, so limits act before overshoot.
   float velocity=s.type==JointType::Slider?glm::dot(b.linearVelocity+glm::cross(b.angularVelocity,rb)-a.linearVelocity-glm::cross(a.angularVelocity,ra),basis[0]):glm::dot(b.angularVelocity-a.angularVelocity,basis[0]);
   index=7;if(!impact&&s.spring)axisRow(coordinate-s.rest,-inf,inf,true);
   index=8;if(!impact&&s.motor)axisRow(0,-s.maxForce*dt,s.maxForce*dt,false,true);
   index=6;
   if(s.limits){if(coordinate<=s.lower||(!impact&&coordinate+velocity*dt<s.lower))axisRow(coordinate<s.lower?coordinate-s.lower:5*(coordinate-s.lower),0,inf);
    else if(coordinate>=s.upper||(!impact&&coordinate+velocity*dt>s.upper))axisRow(coordinate>s.upper?coordinate-s.upper:5*(coordinate-s.upper),-inf,0);}

  }
 }
}
void JointSolver::SolveIteration(){for(auto& r:m_rows){float velocity=glm::dot(r.linearA,r.a->linearVelocity)+glm::dot(r.linearB,r.b->linearVelocity)+glm::dot(r.angularA,r.a->angularVelocity)+glm::dot(r.angularB,r.b->angularVelocity);
 float total=std::clamp(r.impulse-(velocity+r.bias+r.gamma*r.impulse)*r.mass,r.low,r.high),change=total-r.impulse;r.impulse=total;Apply(r,change);if(r.cache)*r.cache=total;if(r.motor)*r.motor=total;}}
void JointSolver::ResetImpulses(){for(auto& r:m_rows)r.impulse=0;}
