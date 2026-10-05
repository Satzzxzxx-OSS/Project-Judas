// Ordinary authored M60 content. This tool is never consulted by runtime playback.
#include "Project.h"
#include "Scene.h"
#include "SceneSerialization.h"
#include "RuntimeUI.h"
#include <filesystem>
#include <cstdio>
#include <fstream>
namespace fs=std::filesystem;
const char* music="60606060606060606060606060606001";
const char* tone="60606060606060606060606060606002";
const char* noise="60606060606060606060606060606003";
const char* pulse="60606060606060606060606060606004";
const char* hall="60606060606060606060606060606005";
const char* booth="60606060606060606060606060606006";
const char* script="60606060606060606060606060606007";
const char* ui="60606060606060606060606060606008";
SceneObject Box(SceneObjectId id,const char* name,glm::vec3 p,glm::vec3 half,glm::vec3 color){SceneObject o;o.id=id;o.name=name;o.transform.position=p;o.body=SceneBodyComponent{};o.body->halfExtents=half;o.render=SceneRenderComponent{};o.render->halfExtents=half;o.render->color=color;return o;}
SceneObject Source(SceneObjectId id,const char* name,glm::vec3 p,const char* asset,bool stream,const char* group,float volume,bool spatial=true){SceneObject o;o.id=id;o.name=name;o.transform.position=p;o.audioEmitter=SceneAudioEmitterComponent{};auto& a=*o.audioEmitter;a.asset=asset;a.loading=stream?AudioLoading::Streamed:AudioLoading::Buffered;a.group=group;a.volume=volume;a.spatial=spatial;a.loop=true;a.playOnStart=true;a.attenuation=AudioAttenuation::None;return o;}
SceneObject Zone(SceneObjectId id,const char* name,glm::vec3 p,const char* asset,glm::vec3 half){SceneObject o;o.id=id;o.name=name;o.transform.position=p;o.audioZone=SceneAudioZoneComponent{};o.audioZone->asset=asset;o.audioZone->shape=SceneRegionShape::Box;o.audioZone->halfExtents=half;return o;}
int main(){std::string error;auto root=fs::path("projects/audio_lab");Project project;if(!project.Load((root/"audio_lab.judasproj").string(),error)){std::puts(error.c_str());return 1;}
 project.Settings().legacyGameplay=false;project.Settings().startupScene="Scenes/listening.judas";project.Settings().audio.groups={{"music",{.7f,false,false}},{"effects",{.7f,false,false}},{"ui",{.7f,false,false}}};project.Settings().input.entries={
 {"move_x",true,{{"key:A",-1,0},{"key:D",1,0},{"stick:LeftX",1,.15f}}},{"move_y",true,{{"key:S",-1,0},{"key:W",1,0},{"stick:LeftY",-1,.15f}}},{"look_x",true,{{"mouse:dx",1,0}}},{"look_y",true,{{"mouse:dy",1,0}}},
 {"pause",false,{{"key:Escape"}}},{"door",false,{{"key:E"}}},{"doppler",false,{{"key:D"}}},{"bypass",false,{{"key:B"}}},{"tone",false,{{"key:T"}}},{"pulse",false,{{"key:Space"}}},{"restart",false,{{"key:R"}}},
 {"ui_click",false,{{"mouse:Left"}}},{"ui_activate",false,{{"key:Return"},{"pad:South"}}},{"ui_down",false,{{"key:Down"},{"pad:DpadDown"}}},{"ui_up",false,{{"key:Up"},{"pad:DpadUp"}}}};
 // D is movement: Doppler comparison uses the menu button, not a conflicting key.
 project.Settings().input.Find("doppler")->bindings={{"key:P"}};
 if(!project.Save(error)){std::puts(error.c_str());return 1;}
 Scene s;s.Settings().name="Audio listening lab";s.SetNextId(100);
 s.InsertObject(Box(1,"Floor",{0,-.5,0},{24,.5,24},{.16,.19,.22}));
 SceneObject gravity;gravity.id=2;gravity.gravity=SceneGravityComponent{};gravity.gravity->kind=SceneGravityKind::Uniform;gravity.gravity->regionShape=SceneRegionShape::Box;gravity.gravity->regionHalfExtents={100,100,100};s.InsertObject(gravity);
 SceneObject player;player.id=10;player.transform.position={0,.9,8};player.characterMotor=CharacterMotorSettings{};player.audioListener=SceneAudioListenerComponent{};player.audioListener->followActiveView=true;player.scripts.push_back({1,script,true,"{}"});player.ui=SceneUIComponent{};player.ui->asset=ui;player.ui->name="audio_lab";s.InsertObject(player);
 s.InsertObject(Source(20,"Long original music",{},music,true,"music",.14,false));
 auto moving=Source(21,"Moving tone",{0,2,-4},tone,false,"effects",.18);moving.audioEmitter->doppler=1;moving.render=SceneRenderComponent{};moving.render->shape=SceneShape::Sphere;moving.render->color={1,.45,.1};s.InsertObject(moving);
 auto source=Source(22,"Noise behind real door",{-8,1.5,-5},noise,true,"effects",.16);source.audioEmitter->occlusion=true;source.audioEmitter->send=.65;source.audioEmitter->attenuation=AudioAttenuation::Inverse;source.audioEmitter->maximumDistance=60;source.audioEmitter->referenceDistance=4;source.render=SceneRenderComponent{};source.render->color={.2,.8,.3};s.InsertObject(source);
 auto hit=Source(23,"Reverb impulse",{},pulse,false,"effects",.25,false);hit.audioEmitter->playOnStart=false;hit.audioEmitter->loop=false;hit.audioEmitter->send=1;s.InsertObject(hit);
 auto click=Source(24,"UI click",{},pulse,false,"ui",.1,false);click.audioEmitter->playOnStart=false;click.audioEmitter->loop=false;s.InsertObject(click);
 s.InsertObject(Box(30,"Wall left",{-11,2,0},{2,2,.3},{.3,.35,.4}));s.InsertObject(Box(31,"Wall right",{-5,2,0},{2,2,.3},{.3,.35,.4}));s.InsertObject(Box(32,"Door header",{-8,3.5,0},{1, .5,.3},{.3,.35,.4}));
 // Real swing clearance at both jambs, the header and the floor.
 auto door=Box(33,"Physical door",{-8,1.5,0},{.8,1.44,.15},{.6,.32,.1});door.body->motion=SceneBodyMotion::Dynamic;door.body->mass=12;door.body->managed=true;s.InsertObject(door);
 SceneObject hinge;hinge.id=34;hinge.joint=SceneJointComponent{};hinge.joint->bodyA=33;hinge.joint->settings.type=JointType::Hinge;hinge.transform.position={-8.8,1.5,0};hinge.joint->settings.anchorA={-.8,0,0};hinge.joint->settings.anchorB={0,0,0};hinge.joint->settings.frameA=hinge.joint->settings.frameB=glm::angleAxis(glm::half_pi<float>(),glm::vec3(0,0,1));hinge.joint->settings.limits=true;hinge.joint->settings.lower=-1.7;hinge.joint->settings.upper=.05;s.InsertObject(hinge);
 s.InsertObject(Zone(40,"Hall reverb",{-8,2,-5},hall,{5,3,6}));s.InsertObject(Zone(41,"Small damped booth",{8,2,-5},booth,{5,3,6}));
 // Colour markers indicate acoustic zones; they are not raised physical thresholds.
 auto hallMarker=Box(42,"Hall marker",{-8,.005,-5},{4,.005,4},{.2,.35,.6});hallMarker.body.reset();s.InsertObject(hallMarker);
 auto boothMarker=Box(43,"Booth marker",{8,.005,-5},{4,.005,4},{.6,.3,.2});boothMarker.body.reset();s.InsertObject(boothMarker);
 if(!SaveSceneToFile(s,(root/"Scenes/listening.judas").string(),error)){std::puts(error.c_str());return 1;}
 UIDocument d;UIElement canvas;canvas.id="canvas";canvas.kind=UIKind::Canvas;d.elements.push_back(canvas);
 UIElement title;title.id="title";title.parent="canvas";title.kind=UIKind::Text;title.offset={20,15};title.size={1220,90};title.fontSize=22;title.wrap=true;title.text="AUDIO LAB - WASD / mouse | Esc controls | E real door | P Doppler | B dry bypass | T tone | Space impulse | R reload\nBlue floor: hall reverb. Orange floor: damped booth. Outside: dry. Listen at modest volume.";d.elements.push_back(title);
 UIElement diag=title;diag.id="diagnostics";diag.offset={20,590};diag.size={1220,110};diag.fontSize=18;diag.text="Preparing audio...";d.elements.push_back(diag);
 UIElement panel;panel.id="menu";panel.parent="canvas";panel.kind=UIKind::Panel;panel.offset={350,100};panel.size={580,520};panel.background={.05,.07,.09,.96};panel.flow=UIFlow::Vertical;panel.padding={20,20,20,20};panel.visible=false;d.elements.push_back(panel);
 for(auto pair:{std::pair<const char*,const char*>{"resume","Resume / mouse capture"},{"music","Music pause / resume"},{"loop","Music loop off / on"},{"seek","Seek music +30 seconds"},{"doppler","Doppler off / on"},{"bypass","Dry bypass off / on"},{"door","Open / close physical door"},{"effects","Pause / resume effects"},{"pulse","Play reverb impulse"},{"reload","Reload clean scene"}}){UIElement button;button.id=pair.first;button.parent="menu";button.kind=UIKind::Button;button.size={520,32};button.fontSize=18;button.text=pair.second;button.background={.2,.25,.3,1};d.elements.push_back(button);}
 UIElement slider;slider.id="gain";slider.parent="menu";slider.kind=UIKind::Slider;slider.size={520,32};slider.value=.7;slider.text="Music group gain";d.elements.push_back(slider);
 if(!SaveUIDocument((root/"Assets/ui/lab.judasui").string(),d,error)){std::puts(error.c_str());return 1;}
 // Minimal integration into the current game: retained root music; region ambience/zones; carried source.
 Project range;if(!range.Load("projects/streamed_range/streamed_range.judasproj",error))return 1;range.Settings().audio.groups={{"music",{.7f,false,false}},{"effects",{.7f,false,false}},{"ui",{.7f,false,false}}};if(!range.Save(error))return 1;
 Scene bootstrap;if(!LoadSceneFromFile(range.StartupScenePath(),bootstrap,error))return 1;bootstrap.InsertObject(Source(610,"Persistent root music",{},music,true,"music",.12,false));for(auto& o:bootstrap.Objects())if(o.audioEmitter&&o.id!=610)o.audioEmitter->group="effects";if(!SaveSceneToFile(bootstrap,range.StartupScenePath(),error))return 1;
 for(unsigned i=0;i<6;++i){auto path=fs::path(range.ScenesDir())/("gallery-"+std::to_string(i)+".judas");Scene gallery;if(!LoadSceneFromFile(path.string(),gallery,error))return 1;auto ambience=Source(610,"Region-local ambience",{0,2,-5},noise,true,"effects",.08);ambience.audioEmitter->occlusion=true;ambience.audioEmitter->send=.5;ambience.audioEmitter->attenuation=AudioAttenuation::Inverse;ambience.audioEmitter->maximumDistance=30;gallery.InsertObject(ambience);gallery.InsertObject(Zone(611,"Gallery acoustics",{},i%2?booth:hall,{11,4,11}));if(auto* prop=gallery.Find(30)){prop->audioEmitter=SceneAudioEmitterComponent{};prop->audioEmitter->asset=tone;prop->audioEmitter->playOnStart=true;prop->audioEmitter->loop=true;prop->audioEmitter->volume=.03;prop->audioEmitter->group="effects";prop->audioEmitter->doppler=1;prop->audioEmitter->send=.4;prop->audioEmitter->occlusion=true;}if(!SaveSceneToFile(gallery,path.string(),error))return 1;}
 std::puts("M60 authored lab and minimal current game audio content written");return 0;
}
