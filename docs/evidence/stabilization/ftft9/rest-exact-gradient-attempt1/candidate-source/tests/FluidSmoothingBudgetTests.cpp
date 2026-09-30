// Isolates ordinary configurable velocity smoothing: no boundaries, gravity,
// or density-position iterations. Independent momentum/energy sum oracle.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <glm/gtc/quaternion.hpp>
#include "FluidWorld.h"
#include "GravityField.h"
struct ZeroGravity : GravityField { glm::vec3 Sample(const glm::vec3&) const override {return glm::vec3(0);} };
// Independent density function and centred numerical derivative. The one-pair
// oracle differentiates the actual density constraint, without copying the
// candidate production gradient formula.
int DensityGradientCandidateChecks(const ZeroGravity& gravity) {
 int failed=0;
 FluidSettings settings;settings.substeps=1;settings.densityIterations=1;settings.velocitySmoothing=0;
 const double h=settings.smoothingRadius,pi=std::acos(-1.0);
 const auto kernel=[&](double r) {const double t=h*h-r*r;return t>0?315.0/(64*pi*std::pow(h,9))*t*t*t:0;};
 const glm::quat rotation=glm::angleAxis(.81f,glm::normalize(glm::vec3(1,-2,3)));
 for(float distance:{.02f,.045f,.08f}) {
  const float mass=float(settings.restDensity*1.005/(kernel(0)+kernel(distance)));
  const double epsilon=h*1e-5;
  const double derivative=(kernel(double(distance)+epsilon)-kernel(double(distance)-epsilon))/(2*epsilon);
  const double gradient=double(mass)/settings.restDensity*derivative;
  const double constraint=double(mass)/settings.restDensity*(kernel(0)+kernel(distance))-1;
  const double lambda=-constraint/(2*gradient*gradient+1e-5);
  const double expectedHalf=double(distance)*.5+2*lambda*gradient;
  FluidWorld fluid(settings),rotated(settings);
  for(float sign:{-1.f,1.f}) {fluid.AddParticle({sign*distance*.5f,0,0},{0,0,0},mass);rotated.AddParticle(rotation*glm::vec3(sign*distance*.5f,0,0),{0,0,0},mass);}
  fluid.Step(.001f,gravity,{});rotated.Step(.001f,gravity,{});
  const double actualHalf=fluid.Particles()[1].position.x;
  const double error=std::abs(actualHalf-expectedHalf);
  const auto momentum=fluid.GetDiagnostics().totalMomentum;
  double rotationError=0;
  for(std::size_t i=0;i<2;++i)rotationError=std::max(rotationError,double(glm::length(rotated.Particles()[i].position-rotation*fluid.Particles()[i].position)));
  const bool projection=error<2e-7,closed=glm::length(momentum)<1e-6f,covariant=rotationError<2e-7;
  std::cout<<std::setprecision(17)<<"DENSITY_GRADIENT distance="<<distance<<" mass="<<mass<<" finite_difference_derivative="<<derivative<<" expected_half="<<expectedHalf<<" actual_half="<<actualHalf<<" projection_error="<<error<<" momentum_residual="<<glm::length(momentum)<<" rotation_error="<<rotationError<<" pass="<<(projection&&closed&&covariant)<<'\n';
  if(!projection||!closed||!covariant)++failed;
 }
 return failed;
}
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
 failed+=DensityGradientCandidateChecks(gravity);
 std::cout<<"SUMMARY cases=2 density_gradient_cases=3 invalid_config_checks="<<invalidRejected<<" failures="<<failed<<'\n';return failed?1:0;
}
