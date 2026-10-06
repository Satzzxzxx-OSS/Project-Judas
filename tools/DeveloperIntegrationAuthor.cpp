// M65 original fixture authoring. The engine serializers supply all record defaults.
#include "Scene.h"
#include "SceneSerialization.h"
#include "Project.h"
#include "AssetDatabase.h"
#include "RuntimeUI.h"
#include <filesystem>
#include <cstdio>
int main(int argc,char** argv){if(argc!=2)return 2;std::string error;Project p;if(!p.Load(argv[1],error)){fprintf(stderr,"%s\n",error.c_str());return 1;}AssetDatabase db;db.Scan(p.RootDir(),p.AssetsDir());auto asset=[&](const char* path){auto r=db.FindByRelativePath(std::string("Assets/")+path);if(!r)throw std::runtime_error(std::string("missing asset ")+path);return r->id;};try{
 Scene s;s.Settings().name="M65 developer integration";s.Settings().backgroundColor={.14,.23,.32};s.Settings().ambientColor={.4,.4,.4};
 auto add=[&](Scene& sc,SceneObjectId id,const char* name,glm::vec3 position){SceneObject o;o.id=id;o.name=name;o.transform.position=position;sc.InsertObject(o);return sc.Find(id);};
 auto box=[&](int id,const char* name,glm::vec3 position,glm::vec3 half,glm::vec3 color,bool dynamic=false){auto* o=add(s,id,name,position);o->render=SceneRenderComponent{};o->render->halfExtents=half;o->render->color=color;o->body=SceneBodyComponent{};o->body->halfExtents=half;o->body->motion=dynamic?SceneBodyMotion::Dynamic:SceneBodyMotion::Static;o->body->restitution=0;return o;};
 auto* g=add(s,1,"Uniform gravity",{});g->gravity=SceneGravityComponent{};g->gravity->kind=SceneGravityKind::Uniform;g->gravity->regionShape=SceneRegionShape::Box;g->gravity->regionHalfExtents={60,30,60};
 auto* deck=box(2,"Deck",{0,-.3,0},{25,.3,22},{.7,.7,.7});deck->render->textureAsset=asset("textures/checker.png");MaterialSlot uv;uv.overrides.uvScale=glm::vec2(20,18);deck->render->materials.push_back(uv);
 auto* visitor=add(s,10,"Visitor",{0,1,10});visitor->characterMotor=CharacterMotorSettings{};visitor->scripts.push_back({1,asset("scripts/visitor.js"),true,"{}"});visitor->ui=SceneUIComponent{asset("ui/integration.judasui"),"integration",true};
 auto* lab=add(s,11,"Integration behaviour",{});lab->scripts.push_back({1,asset("scripts/lab.js"),true,"{}"});
 auto* start=add(s,12,"View seed",{0,1,10});start->playerStart=ScenePlayerStartComponent{};
 box(20,"Tangent ramp",{-7,.8,1},{4,.2,2},{.15,.6,.5})->transform.rotation=glm::angleAxis(glm::radians(11.f),glm::vec3(0,0,1));
 box(21,"Moving platform",{-2,1.5,-6},{2,.2,2},{.7,.5,.15});
 for(int i=0;i<4;++i)box(22+i,"Steps",{-3+float(i),.125f*(i+1),0},{.5,.125f*(i+1),2},{.6,.4,.3});
 box(26,"Wall",{-11,1.5,1},{.2,1.5,3},{.7,.35,.3});box(27,"Corner",{-10,1.5,-2},{1.2,1.5,.2},{.7,.35,.3});
 auto* sensor=add(s,28,"Real checkpoint sensor",{-4,1,2});sensor->body=SceneBodyComponent{};sensor->body->sensor=true;sensor->body->halfExtents={1,1,1};sensor->scripts.push_back({1,asset("scripts/lab_sensor.js"),true,"{}"});
 auto* board=box(100,"Tilting visual board",{5,.15,2},{1.3,.1,.6},{.7,.2,.15});board->body.reset();
 auto* rider=add(s,101,"Original IK rider",{5,-.15f,2});rider->render=SceneRenderComponent{};rider->render->shape=SceneShape::Mesh;rider->render->meshAsset=asset("models/rider.glb");rider->render->color={.15,.6,.95};rider->animation=SceneAnimationComponent{};rider->animation->clip="Lean";
 AnimationLayerSettings arm;arm.id="arm";arm.clip="Wave";arm.weight=.6;arm.mask={"Shoulder","Elbow","Hand"};rider->animation->layers.push_back(arm);
 for(const auto* side:{"Left","Right"}){LimbIKSettings k;k.id=side;k.root=std::string(side)+"Hip";k.middle=std::string(side)+"Knee";k.end=std::string(side)+"Foot";k.target={5+(k.id=="Left"?-.38f:.38f),.25,2};k.pole={5,1,3};rider->animation->limbs.push_back(k);}
 auto* prop=add(s,102,"Hand socket prop",{});prop->render=SceneRenderComponent{};prop->render->halfExtents={.12,.12,.3};prop->render->color={1,.8,.1};prop->socket=SceneSocketComponent{};prop->socket->target=101;prop->socket->joint="Hand";prop->socket->offset.position={.15,0,0};
 rider=s.Find(101);SceneObject doll=*rider;doll.id=1;doll.name="Original articulated rider";doll.animation->limbs.clear();doll.animation->layers.clear();doll.animation->playOnStart=false;doll.ragdoll=RagdollDefinition{};doll.ragdoll->selfCollision=false;doll.ragdoll->playOnStart=true;
 for(const auto* name:{"Root","LeftHip","LeftKnee","RightHip","RightKnee","Shoulder","Elbow"}){RagdollBone b;b.joint=name;b.parent=b.joint=="Root"?"":b.joint=="LeftKnee"?"LeftHip":b.joint=="RightKnee"?"RightHip":b.joint=="Elbow"?"Shoulder":"Root";b.constraint.type=JointType::Hinge;b.constraint.limits=true;b.constraint.lower=-1.2f;b.constraint.upper=1.2f;b.constraint.spring=true;b.constraint.stiffness=3;b.constraint.damping=1;b.halfExtents=b.joint=="Root"?glm::vec3(.42,.28,.18):b.joint=="Shoulder"||b.joint=="Elbow"?glm::vec3(.22,.08,.09):glm::vec3(.1,.41,.1);b.offset=b.joint=="Root"?glm::vec3(0):b.joint=="Shoulder"||b.joint=="Elbow"?glm::vec3(.22,0,0):glm::vec3(0,-.41,0);b.mass=b.joint=="Root"?3:1;doll.ragdoll->bones.push_back(b);}
 rider->ragdoll=doll.ragdoll;rider->ragdoll->playOnStart=false;
 Scene posed;auto poseSource=*rider;poseSource.id=1;poseSource.ragdoll.reset();posed.InsertObject(poseSource);auto propSource=*s.Find(102);propSource.id=2;propSource.parent=1;propSource.socket->target=1;posed.InsertObject(propSource);if(!SaveSceneToFile(posed,p.Resolve("Assets/prefabs/posed.judasprefab"),error))throw std::runtime_error(error);
 Scene prefab;prefab.InsertObject(doll);if(!SaveSceneToFile(prefab,p.Resolve("Assets/prefabs/rider.judasprefab"),error))throw std::runtime_error(error);
 for(int i=0;i<5;++i){auto d=doll;d.id=200+i;d.transform.position={-6+float(i)*2,1,-12};s.InsertObject(d);}
 for(int i=0;i<12;++i)box(300+i,"Settling debris",{float(i%4)*1.1f,.4f+float(i/4)*.81f,-5},{.4,.4,.4},{.5,.6,.3},true);
 auto* mat=box(400,"Material and runtime connection",{8,2,-4},{.45,.45,.45},{.9,.3,.15},true);mat->body->physicalMaterial=asset("physical/slippery.judasphysmat");add(s,401,"Connection owner",{8,2,-4});
 auto* copy=add(s,500,"Typed reference authoring",{});copy->scripts.push_back({1,asset("scripts/reference.js"),true,"{\"target\":{\"entity\":\"101\"}}"});
 if(!SaveSceneToFile(s,p.Resolve("Scenes/integration.judas"),error))throw std::runtime_error(error);
 Scene annex;annex.Settings().name="M65 streamed rig";auto d=*s.Find(101);d.id=1;d.ragdoll.reset();d.animation->limbs.clear();annex.InsertObject(d);auto attachment=*s.Find(102);attachment.id=2;attachment.parent=1;attachment.socket->target=1;annex.InsertObject(attachment);auto body=*s.Find(400);body.id=3;body.transform.position={5,2,2};annex.InsertObject(body);auto owner=*s.Find(401);owner.id=4;owner.parent=3;owner.transform.position={0,0,0};owner.joint=SceneJointComponent{};owner.joint->bodyA=3;owner.joint->settings.anchorB={0,0,0};annex.InsertObject(owner);auto ref=*s.Find(500);ref.id=5;ref.parent=1;ref.scripts[0].properties="{\"target\":{\"entity\":\"1\"}}";annex.InsertObject(ref);
 if(!SaveSceneToFile(annex,p.Resolve("Scenes/annex.judas"),error))throw std::runtime_error(error);
 p.Settings().name="Judas Developer Integration";p.Settings().startupScene="Scenes/integration.judas";p.Settings().legacyGameplay=false;
 for(auto [name,key]:{std::pair{"lab","key:F1"},{"parkour","key:F2"},{"wake","key:G"},{"material","key:M"},{"connect","key:J"},{"reanchor","key:K"},{"spawn","key:P"},{"save_game","key:F6"},{"load_game","key:F7"},{"region","key:T"},{"adopt","key:Y"},{"ragdoll","key:B"}}){InputEntry e;e.name=name;e.bindings.push_back({key,1,0});if(auto* existing=p.Settings().input.Find(name))*existing=e;else p.Settings().input.entries.push_back(e);}
 p.Settings().worldManifest=asset("worlds/integration.judasworld");if(!p.Save(error))throw std::runtime_error(error);puts("M65 ordinary scene/prefab/project records authored");return 0;
 }catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
