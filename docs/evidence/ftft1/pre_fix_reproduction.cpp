// FTFT1 production persistence compatibility and failure atomicity.
#include <cstdio>
#include <string>
#include "RuntimeWorld.h"
#include "Scene.h"
#include "SceneSerialization.h"
#include "WorldState.h"

namespace {
int failures=0;
void Check(bool condition,const std::string& label){ std::printf("%s %s\n",condition?"PASS":"FAIL",label.c_str()); if(!condition)++failures; }
Scene Baseline(){
 Scene s; s.Settings().name="FTFT1 unchanged name";
 for(int i=0;i<3;++i){auto& o=s.CreateObject("body "+std::to_string(i));o.transform.position=glm::vec3(float(i*3),2,0);o.body=SceneBodyComponent{};o.body->motion=SceneBodyMotion::Dynamic;if(i==2)o.vehicle=SceneVehicleComponent{};}
 return s;
}
void Reproduce(){
 std::string error; Scene scene=Baseline();RuntimeWorld source;Check(source.Build(scene,nullptr,error),"build source: "+error);
 WorldState saved=CaptureWorldState(source);WorldStateEntityChange moved;moved.id=1;source.GetEntityState(1,moved.state);moved.state.position.x=44;saved.entities.push_back(moved);
 Scene edited=scene;edited.Find(1)->transform.position.x=99;RuntimeWorld destination;Check(destination.Build(edited,nullptr,error),"build different authored content with same name: "+error);
 const bool applied=ApplyWorldState(destination,saved,error);EntityPhysicalState after;destination.GetEntityState(1,after);
 std::printf("OBSERVED baseline_mismatch applied=%d position_x=%g error='%s'\n",applied,after.position.x,error.c_str());
 Check(!applied&&after.position.x==99,"same-name changed baseline must reject without overwriting live state");
 RuntimeWorld fresh;Check(fresh.Build(scene,nullptr,error),"build atomicity world: "+error);WorldState mixed=CaptureWorldState(fresh);
 WorldStateEntityChange destroy;destroy.id=1;destroy.destroyed=true;mixed.entities.push_back(destroy);destroy.id=3;mixed.entities.push_back(destroy);
 const bool mixedApplied=ApplyWorldState(fresh,mixed,error);const auto life=fresh.FindEntity(1)->lifecycle;
 std::printf("OBSERVED partial_apply result=%d first_entity_lifecycle=%d error='%s'\n",mixedApplied,int(life),error.c_str());
 Check(!mixedApplied&&life==EntityLifecycle::Active,"later non-destroyable vehicle must be rejected before earlier entity destruction");
}
}
int main(){Reproduce();std::printf("FTFT1 checks: %d failures\n",failures);return failures?1:0;}
