// FTFT9 actual particle-field -> ordinary played-world -> custom-player tests.
// No SetFluidSample calls, alternate fluid loop, or particle-force injection.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include "GameSession.h"
#include "ProductionFluidCoupling.h"
#include "RuntimeWorld.h"
#include "SceneSerialization.h"
#include "Simulation.h"
#include "SimulationTiming.h"
#include "Window.h"
namespace {
namespace fs=std::filesystem;int checks=0,failures=0,steps=0;constexpr float dt=SimulationTiming::kFixedTimestep;
void Check(bool value,const std::string& label){++checks;failures+=!value;if(!value)std::cerr<<"FAIL "<<label<<'\n';}
struct Frame {glm::quat q{1,0,0,0};glm::vec3 offset{0};glm::vec3 Point(glm::vec3 p)const{return offset+q*p;}};
struct Result{std::string name;int count=0,failures=0;float density=0,spacing=0,maxSpeed=0,maxImmersion=0,minImmersion=1,finalImmersion=0,relativeDrift=0;glm::vec3 initial{0},final{0},velocity{0};double seconds=0,maxParticleMs=0;unsigned long long particleSteps=0;};
std::vector<Result> results;
void Box(Scene& s,const Frame& frame,glm::vec3 p,glm::vec3 h){auto&o=s.CreateObject("Boundary");o.transform.position=frame.Point(p);o.transform.rotation=frame.q;o.body=SceneBodyComponent{};o.body->halfExtents=h;}
Scene Fixture(const Frame& frame,float spacing,float density,bool pool,float gravity,glm::vec3 player){Scene s;s.Settings().name="FTFT9 particle-field player";s.Settings().fluidScale=spacing/.05f;s.Settings().fluidUpdateRateHz=30;
if(pool){Box(s,frame,{0,-.2f,0},{1.8f,.2f,1.3f});Box(s,frame,{-1.8f,2,0},{.2f,2,1.5f});Box(s,frame,{1.8f,2,0},{.2f,2,1.5f});Box(s,frame,{0,2,-1.3f},{2,2,.2f});Box(s,frame,{0,2,1.3f},{2,2,.2f});}
auto& f=s.CreateObject("Liquid volume");f.transform.position=frame.Point({0,spacing*.5f,0});f.transform.rotation=frame.q;f.fluidVolume=SceneFluidVolumeComponent{};f.fluidVolume->spacing=spacing;f.fluidVolume->countX=int(std::lround(3/spacing));f.fluidVolume->countY=int(std::lround(3/spacing));f.fluidVolume->countZ=int(std::lround(2/spacing));
auto& g=s.CreateObject("Gravity region");g.transform.position=frame.offset;g.transform.rotation=frame.q;g.gravity=SceneGravityComponent{};g.gravity->kind=SceneGravityKind::Uniform;g.gravity->magnitude=gravity;g.gravity->regionRadius=100;
auto& p=s.CreateObject("Player start");p.transform.position=frame.Point(player);p.playerStart=ScenePlayerStartComponent{};p.playerStart->density=density;p.playerStart->fluidDrag=2;p.playerStart->swimAcceleration=4;return s;}
struct Played{RuntimeWorld world;GameSession session;Window window;bool Begin(const Scene& scene,const fs::path& out,const std::string& name){std::string text,error;bool ok=SaveSceneToString(scene,text);Check(ok,name+" serialize");if(!ok)return false;std::ofstream(out/(name+".judas"))<<text;Scene loaded;ok=LoadSceneFromString(text,loaded,error);Check(ok,name+" scene load "+error);if(!ok)return false;window.SetTestInputMode(true);ok=world.Build(loaded,nullptr,error)&&session.Begin(world,error);Check(ok,name+" runtime/session "+error);return ok;}void Step(){session.HandleFrameInput(window,false,false,false,false,false);StepPlayedWorld(session,window,dt);++steps;}};
glm::vec3 Centroid(const FluidWorld& fluid){glm::dvec3 sum(0);double mass=0;for(const auto&p:fluid.Particles()){sum+=glm::dvec3(p.position)*double(p.mass);mass+=p.mass;}return mass>0?glm::vec3(sum/mass):glm::vec3(0);}
bool Finite(glm::vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
Result Run(const fs::path& out,const std::string& family,const Frame& frame,float spacing,float density,bool pool,float gravity,glm::vec3 spawn,int count,bool swim=false){Result r;r.name=family+"-"+std::to_string(int(spacing*100))+"-"+std::to_string(int(density));r.spacing=spacing;r.density=density;const int beforeFailures=failures;const auto begin=std::chrono::steady_clock::now();Played p;if(!p.Begin(Fixture(frame,spacing,density,pool,gravity,spawn),out,r.name))return r;
Check(p.session.Player().Density()==density&&p.session.Player().FluidDrag()==2&&p.session.Player().SwimAcceleration()==4,r.name+" serialized player settings reach controller");Check(p.world.DynamicBodies().empty(),r.name+" player is not a rigid participant");
Played reference;if(swim&&!reference.Begin(Fixture(frame,spacing,density,pool,gravity,{20,1.5f,0}),out,r.name+"-particle-reference"))return r;
r.initial=p.session.Player().GetPosition();const auto initialRelative=r.initial-Centroid(p.world.Fluid());const std::size_t initialParticles=p.world.Fluid().Particles().size();std::ofstream csv(out/(r.name+".csv"));csv<<std::setprecision(17)<<"step,x,y,z,vx,vy,vz,immersion,fluid_vx,fluid_vy,fluid_vz,fluid_ax,fluid_ay,fluid_az,fluid_executed,particle_ms\n";bool entered=false,left=false;float entryZ=0;bool compared=false;
for(int i=0;i<count;++i){if(swim){p.window.SetTestActionState(Action::MoveForward,true);if(i==120)p.window.RequestTestJump();reference.Step();}p.Step();const auto&player=p.session.Player();const auto state=player.GetFluidSample();const auto position=player.GetPosition(),v=player.GetVelocity();const auto&measured=p.world.FluidCoupling().Measurements();
Check(Finite(position)&&Finite(v)&&state.immersion>=0&&state.immersion<=1,r.name+" finite bounded actual field/controller state");Check(p.world.Fluid().Particles().size()==initialParticles,r.name+" finite particle inventory preserved");r.maxSpeed=std::max(r.maxSpeed,glm::length(v));r.maxImmersion=std::max(r.maxImmersion,state.immersion);r.minImmersion=std::min(r.minImmersion,state.immersion);r.maxParticleMs=std::max(r.maxParticleMs,measured.particleMilliseconds);
if(swim){if(!entered&&state.immersion>.2f){entered=true;entryZ=position.z;}if(entered&&state.immersion==0&&position.z<entryZ-1)left=true;}
csv<<i<<','<<position.x<<','<<position.y<<','<<position.z<<','<<v.x<<','<<v.y<<','<<v.z<<','<<state.immersion<<','<<state.velocity.x<<','<<state.velocity.y<<','<<state.velocity.z<<','<<state.acceleration.x<<','<<state.acceleration.y<<','<<state.acceleration.z<<','<<measured.executed<<','<<measured.particleMilliseconds<<'\n';}
r.final=p.session.Player().GetPosition();r.velocity=p.session.Player().GetVelocity();r.finalImmersion=p.session.Player().GetFluidSample().immersion;r.particleSteps=p.world.FluidCoupling().Measurements().executedSteps;r.count=count;
const auto finalRelative=r.final-Centroid(p.world.Fluid());r.relativeDrift=glm::length(finalRelative-initialRelative);
Check(r.particleSteps>0&&r.particleSteps<static_cast<unsigned long long>(count),r.name+" real particle solver runs at reduced cadence");const float speedBound=(!pool&&gravity>0)?gravity*dt*count+1.f:12.f;Check(r.maxSpeed<speedBound,r.name+" no explosive player response beyond ordinary gravity/input budget");
const float rise=glm::dot(r.final-r.initial,frame.q*glm::vec3(0,1,0));
if(family=="density"||family=="rotated-density"){Check(r.maxImmersion>.8f,r.name+" player starts substantially inside actual coarse fluid");if(density<1000)Check(rise>.4f,r.name+" low density player rises");else if(density==1000)Check(std::abs(rise)<.25f,r.name+" neutral player has no rapid systematic drift");else Check(rise<-.35f,r.name+" dense player sinks toward physical floor");}
if(family=="common-fall"||family=="rotated-fall")Check(r.relativeDrift<.2f,r.name+" common free fall avoids large artificial relative motion");
if(family=="zero-g")Check(glm::length(r.final-r.initial)<.1f&&glm::length(r.velocity)<.05f,r.name+" quiescent zero gravity invents no buoyancy");
if(swim){Check(entered&&left&&r.finalImmersion==0,r.name+" input enters, swims through and leaves real coarse particle field");Check(r.final.z<r.initial.z-5,r.name+" no coarse-particle column stall");const auto&a=p.world.Fluid().Particles();const auto&b=reference.world.Fluid().Particles();bool equal=a.size()==b.size();if(equal)for(std::size_t i=0;i<a.size();++i)equal&=a[i].position==b[i].position&&a[i].velocity==b[i].velocity&&a[i].acceleration==b[i].acceleration;compared=equal;Check(equal,r.name+" moving player adds no particle contact/push: exact reference particle trajectory");}
r.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();r.failures=failures-beforeFailures;std::cout<<std::setprecision(9)<<"CASE "<<r.name<<" failures="<<r.failures<<" height_change="<<rise<<" final_immersion="<<r.finalImmersion<<" max_speed="<<r.maxSpeed<<" relative_drift="<<r.relativeDrift<<" particle_steps="<<r.particleSteps<<" seconds="<<r.seconds<<" particle_reference_equal="<<compared<<'\n';return r;}
}
int main(int argc,char**argv){fs::path output;bool smoke=false;for(int i=1;i<argc;++i){if(std::string(argv[i])=="--output"&&i+1<argc)output=argv[++i];else if(std::string(argv[i])=="--smoke")smoke=true;else return 2;}if(output.empty())output=fs::temp_directory_path()/("judas-ftft9-player-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));if(fs::exists(output)&&!fs::is_empty(output)){std::cerr<<"Refusing to overwrite evidence\n";return 2;}fs::create_directories(output);const Frame identity{};const Frame rotated{glm::angleAxis(.83f,glm::normalize(glm::vec3(1,-2,3))),{3,-2,4}};
for(float spacing:smoke?std::vector<float>{.25f}:std::vector<float>{.2f,.25f})for(float density:smoke?std::vector<float>{1000.f}:std::vector<float>{500.f,1000.f,2000.f})results.push_back(Run(output,"density",identity,spacing,density,true,9.81f,{0,1.5f,0},240));
if(!smoke){results.push_back(Run(output,"rotated-density",rotated,.25f,500,true,9.81f,{0,1.5f,0},240));results.push_back(Run(output,"common-fall",identity,.25f,1000,false,9.81f,{0,1.5f,0},120));results.push_back(Run(output,"rotated-fall",rotated,.25f,1000,false,9.81f,{0,1.5f,0},120));results.push_back(Run(output,"zero-g",identity,.25f,500,false,0,{0,1.5f,0},240));for(float spacing:{.2f,.25f})results.push_back(Run(output,"swim-through",identity,spacing,950,false,0,{0,1.5f,4},300,true));}
std::ostringstream out;out<<std::setprecision(17)<<"{\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"ordinary_steps\":"<<steps<<",\"pass\":"<<(failures?"false":"true")<<",\"fixtures\":[";for(std::size_t i=0;i<results.size();++i){if(i)out<<',';const auto&r=results[i];out<<"{\"name\":\""<<r.name<<"\",\"steps\":"<<r.count<<",\"failures\":"<<r.failures<<",\"spacing\":"<<r.spacing<<",\"density\":"<<r.density<<",\"initial_position\":["<<r.initial.x<<','<<r.initial.y<<','<<r.initial.z<<"],\"final_position\":["<<r.final.x<<','<<r.final.y<<','<<r.final.z<<"],\"final_velocity\":["<<r.velocity.x<<','<<r.velocity.y<<','<<r.velocity.z<<"],\"final_immersion\":"<<r.finalImmersion<<",\"maximum_immersion\":"<<r.maxImmersion<<",\"max_speed\":"<<r.maxSpeed<<",\"relative_drift\":"<<r.relativeDrift<<",\"particle_steps\":"<<r.particleSteps<<",\"max_particle_ms\":"<<r.maxParticleMs<<",\"seconds\":"<<r.seconds<<'}';}out<<"]}\n";std::ofstream(output/"results.json")<<out.str();std::cout<<"SUMMARY checks="<<checks<<" failures="<<failures<<" ordinary_steps="<<steps<<'\n';return failures?1:0;}
