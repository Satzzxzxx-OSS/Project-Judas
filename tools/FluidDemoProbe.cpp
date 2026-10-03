// Observe the normal Application loop; do not replace its clock or simulation.
#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "ProductionFluidCoupling.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <memory>
int main(int argc,char** argv){
 if(argc<3)return 2;
 std::filesystem::create_directories(argv[2]);
 std::ofstream frames(std::filesystem::path(argv[2])/"frames.csv"),steps(std::filesystem::path(argv[2])/"steps.csv");
 frames<<"frame,wall_ms,frame_cpu_ms,fixed_steps,surface_ms,render_submit_ms,particles,bodies,cavities,phase\n";
 steps<<"step,fixed_ms,particle_executed,particle_ms,coupling_ms,rigid_ms,broadphase_ms,narrowphase_ms,solver_ms,contacts,contained_mass,density_ms,boundary_ms,velocity_ms,hydrostatic_ms,player_ms,phase\n";
 frames<<std::setprecision(10);steps<<std::setprecision(10);
 int frame=0,step=0;float wall=0;using Clock=std::chrono::steady_clock;Clock::time_point started;
 const bool cupRun=argc>4 && std::string(argv[4])=="cup";
 std::unique_ptr<ObjectManipulation> hands;BodyHandle cup;glm::quat guideRotation(1,0,0,0);glm::vec3 guideOrigin(0);std::string phase="view";
 const int count=argc>3?std::stoi(argv[3]):120;
 ApplicationControl control;control.hidden=true;InteractivePlay::FixedStepObserver observer;
 control.frameSeconds=[&](float measured){wall=measured;return measured;};
 control.hostReady=[](EngineHost& h){std::string error;h.Audio().Init(error,true);};
 control.worldReady=[&](EngineHost&,RuntimeWorld& initial,InteractivePlay& play){
  if(cupRun)for(const auto& entity:initial.Entities())if(entity.definition.body && !entity.definition.body->fluidCavities.empty()){
   cup=initial.DynamicBodies().at(entity.slot).Handle();const auto pose=initial.Physics().GetTransform(cup);
   guideRotation=pose.rotation;guideOrigin=pose.position-guideRotation*glm::vec3(2,3.9f,4.7f);
   hands=std::make_unique<ObjectManipulation>(std::vector<BodyHandle>{cup});hands->TryPickUp(cup,initial.Physics());break;
  }
  play.SetFixedStepMeasurementFlags(false,false,true);
  observer=[&,active=&play](const FixedStepMeasurements&,double fixed){
   auto& current=active->Session().World();
   const auto& m=current.FluidCoupling().Measurements();const auto& p=current.Physics().LastStepStats();
   double contained=0;for(const auto& b:m.bodies)contained+=b.containedMass;
   steps<<step++<<','<<fixed<<','<<m.executed<<','<<m.particleMilliseconds<<','<<m.totalMilliseconds<<','<<p.totalMilliseconds<<','<<p.broadphaseMilliseconds<<','<<p.narrowphaseMilliseconds<<','<<p.solverMilliseconds<<','<<p.contactPoints<<','<<contained<<','<<current.Fluid().GetDiagnostics().densityMilliseconds<<','<<current.Fluid().GetDiagnostics().boundaryMilliseconds<<','<<current.Fluid().GetDiagnostics().velocityMilliseconds<<','<<m.hydrostaticMilliseconds<<','<<m.playerMilliseconds<<','<<phase<<'\n';
   // Diagnostic hand targets use existing forces/torques for the NEXT ordinary
   // fixed step. They never change particle state or body poses directly.
   if(hands && step>120){glm::vec3 target(1.6f,4.4f,-1.5f);glm::quat attitude=guideRotation;
    if(step<270)phase="over_pool";
    else if(step<480){phase="dip";target.y=1.1f;attitude*=glm::angleAxis(1.5707963f,glm::vec3(1,0,0));}
    else if(step<570){phase="upright";target.y=1.1f;}
    else if(step<750)phase="lift";
    else if(step<930){phase="carry";target.z=5;}
    else if(step<1110)phase="return";
    else {phase="pour";attitude*=glm::angleAxis(2.2f,glm::vec3(1,0,0));}
    hands->ApplyCarryForce(current.Physics(),guideOrigin+guideRotation*target,glm::vec3(0));
    hands->ApplyCarryOrientationTorque(current.Physics(),attitude);
   }
  };
 };
 control.beforeFrame=[&](EngineHost&,RuntimeWorld&,InteractivePlay& play){play.SetFixedStepObserver(observer);play.SetFixedStepMeasurementFlags(false,false,true);started=Clock::now();};
 control.afterFrame=[&](EngineHost&,RuntimeWorld& w,InteractivePlay& play){
  const double cpu=std::chrono::duration<double,std::milli>(Clock::now()-started).count();
  size_t cavities=0;for(const auto& e:w.Entities())if(e.definition.body)cavities+=e.definition.body->fluidCavities.size();
  frames<<frame++<<','<<wall*1000<<','<<cpu<<','<<play.LastFixedStepsThisFrame()<<','<<play.LastSurfaceMilliseconds()<<','<<play.LastSceneMilliseconds()<<','<<w.Fluid().Particles().size()<<','<<w.Physics().AliveBodies().size()<<','<<cavities<<','<<phase<<'\n';
  if(cupRun?step>=count:frame>=count){SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
 };
 char name[]="judas";char* args[]={name,argv[1]};Application app;
 const int result=app.Run(2,args,&control);std::printf("Measured %d frames / %d fixed steps; normal measured frame clock; 60 Hz rigid / authored fluid cadence (see scene metadata).\n",frame,step);return result;
}
