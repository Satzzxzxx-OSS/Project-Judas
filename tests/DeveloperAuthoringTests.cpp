#include "StructuredAuthor.h"
#include "Scene.h"
#include "SceneSerialization.h"
#include "RuntimeUI.h"
#include "ProjectExporter.h"
#include "AudioSystem.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
unsigned n=0,bad=0;void check(bool ok,const char* name){n++;bad+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",name);}
int main(int argc,char** argv){namespace fs=std::filesystem;auto out=fs::absolute(argc>1?argv[1]:"build/m65-authoring");fs::create_directories(out);std::string error;
 auto generate=[&](const char* json,const char* output){std::ofstream(out/"source.json")<<json;return WriteStructuredContent((out/"source.json").string(),(out/output).string(),error);};
 check(generate(R"({"kind":"ui","elements":[{"id":"root","kind":"canvas"},{"id":"start","parent":"root","kind":"button","text":"Go","size":[240,50]}]})","menu.judasui"),"named menu through normal serializer");UIDocument ui;check(LoadUIDocument((out/"menu.judasui").string(),ui,error)&&ui.elements.size()==2&&ui.elements[1].text=="Go","normal UI loader round-trip");
 check(!generate(R"({"kind":"ui","elements":[{"id":"root","size":[1]}]})","bad.judasui"),"invalid vector rejected without crash");check(!generate(R"({"kind":"ui","elements":[{"id":"a","parent":"b"},{"id":"b","parent":"a"}]})","bad.judasui"),"cycle rejected by ordinary UI validator");
 check(generate(R"({"kind":"input","entries":[{"name":"move","axis":true,"bindings":[{"control":"stick:LeftX","deadzone":0.2}]}]})","input.txt"),"named input uses normal map serializer");std::ifstream in(out/"input.txt");std::string raw((std::istreambuf_iterator<char>(in)),{});InputMap map;check(InputMap::Parse(raw,map,error)&&map.Find("move")->axis,"input normal parser round-trip");
 check(!generate(R"({"kind":"input","entries":[{"name":"x","bindings":[{"control":"stick:LeftX","deadzone":1}]}]})","bad.txt"),"invalid binding rejected by normal map validation");
 check(generate(R"({"kind":"scene","name":"Named","objects":[{"id":"1","name":"Floor","components":["body"],"fields":{"position":"0 -0.5 0","body.half-extents":"5 0.5 5"}}]})","main.judas"),"named scene applies actual property records");Scene scene;check(LoadSceneFromFile((out/"main.judas").string(),scene,error)&&scene.Find(1)->transform.position.y==-.5f,"scene normal loader round-trip");
 check(!generate(R"({"kind":"scene","objects":[{"id":"1","components":[],"fields":{"invented":"1"}}]})","bad.judas"),"unknown scene property rejected");
 check(!generate(R"({"kind":"scene","objects":[{"id":"1","components":[],"fields":{"position":123}}]})","bad.judas"),"canonical scene fields reject non-string authoring values");
 fs::create_directories(out/"project/Scenes");fs::create_directories(out/"project/Assets");for(auto name:{"main","extra"})SaveSceneToFile(scene,(out/"project/Scenes"/(std::string(name)+".judas")).string(),error);
 std::ofstream(out/"project/main.judasproj")<<"JudasProject 1\nname \"Authoring check\"\nstartup-scene \"Scenes/main.judas\"\nassets-dir \"Assets\"\nscenes-dir \"Scenes\"\nsaves-dir \"Saves\"\n";
 Project p;check(p.Load((out/"project/main.judasproj").string(),error),"ordinary project");p.Settings().excludeScenes={"Scenes/extra.judas"};check(p.Save(error),"export policy serializer");Project reload;check(reload.Load(p.ProjectFile(),error)&&reload.Settings().excludeScenes==p.Settings().excludeScenes,"export selection normal round-trip");
 ProjectExportOptions options;options.destination=(out/"package").string();options.runtimeExecutable=fs::absolute("build/judas").string();ProjectExportResult result;check(ExportProject(reload,options,result,error)&&result.sceneCount==1&&!fs::exists(out/"package/Scenes/extra.judas"),"explicit export excludes unused registered scene");reload.Settings().excludeScenes={"Scenes/main.judas"};check(!ExportProject(reload,options,result,error)&&error.find("Startup scene")!=std::string::npos,"invalid startup policy rejects package");
 AudioSystem audio;audio.Init(error,true);AudioSettings s;check(!audio.CreateVoice({},s,error).IsValid()&&error.find("clip")!=std::string::npos,"audio invalid clip diagnostic");AudioData data;data.channels=1;data.samples.resize(4800);auto clip=audio.CreateClip(std::move(data));s.pitch=-1;check(!audio.CreateVoice(clip,s,error).IsValid()&&error.find("settings")!=std::string::npos,"audio invalid settings diagnostic");s.pitch=1;s.group="absent";check(!audio.CreateVoice(clip,s,error).IsValid()&&error.find("group")!=std::string::npos,"audio missing group diagnostic");s.group="";for(int i=0;i<128;++i)audio.CreateVoice(clip,s,error);check(!audio.CreateVoice(clip,s,error).IsValid()&&error.find("capacity")!=std::string::npos,"audio capacity diagnostic");
 printf("SUMMARY %u checks %u failures\n",n,bad);return bad?1:0;}
