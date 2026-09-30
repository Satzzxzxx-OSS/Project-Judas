// Isolates ordinary configurable velocity smoothing: no boundaries, gravity,
// or density-position iterations. Independent momentum/energy sum oracle.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "FluidWorld.h"
#include "GravityField.h"
struct ZeroGravity : GravityField { glm::vec3 Sample(const glm::vec3&) const override {return glm::vec3(0);} };
int main(){
 int failed=0;ZeroGravity gravity;
 for(int kind=0;kind<2;++kind){
  FluidSettings settings;settings.substeps=1;settings.densityIterations=0;settings.velocitySmoothing=.15f;
  FluidWorld fluid(settings);
  if(kind==0){fluid.AddParticle({0,0,0},{1,0,0},1);fluid.AddParticle({.04f,0,0},{-.5f,0,0},2);}
  else {fluid.AddParticle({0,0,0},{1,0,0},1);fluid.AddParticle({.025f,0,0},{-.5f,0,0},1);fluid.AddParticle({.075f,0,0},{-.5f,0,0},1);}
  auto budget=[&](){glm::dvec3 p(0);double ke=0;for(const auto& x:fluid.Particles()){p+=static_cast<double>(x.mass)*glm::dvec3(x.velocity);ke+=.5*x.mass*glm::dot(glm::dvec3(x.velocity),glm::dvec3(x.velocity));}return std::pair<glm::dvec3,double>{p,ke};};
  const auto before=budget();fluid.Step(.001f,gravity,{});const auto after=budget();
  const bool momentum=glm::length(after.first-before.first)<=1e-6,energy=after.second<=before.second+1e-6;
  std::cout<<std::setprecision(17)<<"CASE "<<(kind==0?"unequal_mass_pair":"equal_mass_asymmetric_neighbourhood")<<" initial_P="<<before.first.x<<" final_P="<<after.first.x<<" delta_P="<<after.first.x-before.first.x<<" initial_KE="<<before.second<<" final_KE="<<after.second<<" momentum="<<momentum<<" dissipative="<<energy<<'\n';
  for(std::size_t i=0;i<fluid.Particles().size();++i)std::cout<<" particle="<<i<<" mass="<<fluid.Particles()[i].mass<<" final_vx="<<fluid.Particles()[i].velocity.x<<'\n';
  if(!momentum||!energy)++failed;
 }
 int invalidRejected=0;
 for(float value:{-.01f,1.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
  FluidSettings invalid;invalid.velocitySmoothing=value;bool rejected=false;
  try { FluidWorld unused(invalid); } catch(const std::invalid_argument&) { rejected=true; }
  std::cout<<"CONFIG smoothing="<<value<<" rejected="<<rejected<<'\n';
  if(rejected)++invalidRejected;else ++failed;
 }
 std::cout<<"SUMMARY cases=2 invalid_config_checks="<<invalidRejected<<" failures="<<failed<<'\n';return failed?1:0;
}
