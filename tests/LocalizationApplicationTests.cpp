#include "Application.h"
#include "EngineHost.h"
#include "InteractivePlay.h"
#include "RuntimeWorld.h"
#include "SceneSession.h"
#include "SceneSerialization.h"
#include "Prefab.h"
#include "Project.h"
#include "ScreenshotWriter.h"
#include "PerformanceProfiler.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <locale>
namespace fs=std::filesystem;int checks=0,failures=0;void Check(bool x,const char* s){++checks;failures+=!x;std::printf("%s %s\n",x?"PASS":"FAIL",s);}
std::string Read(const fs::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
struct Comma:std::numpunct<char>{char do_decimal_point()const override{return ',';}};
int main(int argc,char** argv){fs::path out=argc>1?argv[1]:"build/m58-application";fs::create_directories(out);std::string error;EngineHost host;Check(host.Init("M58 session/resources",640,360,false,error),"normal host");if(failures)return 1;
 auto fixture=fs::absolute(out/"fixture");fs::remove_all(fixture);fs::copy("projects/text_lab",fixture,fs::copy_options::recursive);Project p;Check(p.Load((fixture/"text_lab.judasproj").string(),error),"ordinary text lab project");host.OpenProjectAssets(p.RootDir(),p.AssetsDir());auto& resources=host.Resources();
 // PR #1 / D-03: hold a real catalog worker before publication, then exercise
 // the public JudasJS binding. A C++ fallback alone does not prove JS won't throw.
 {
  auto scriptPath=fixture/"Assets/scripts/catalog-startup-proof.js";
  fs::create_directories(scriptPath.parent_path());
  std::ofstream(scriptPath)<<R"JS(
import {localization} from 'judas';
export default class {
 constructor(){this.state={early:false,published:false,missing:false,invalid:false};}
 start(){this.state.early=localization.format('lab.named',{first:'A',second:'B'})==='[lab.named]';}
 update(){
  this.state.published=localization.format('lab.named',{first:'A',second:'B'})==='B — A';
  this.state.missing=localization.format('absent.integration.key')==='[absent.integration.key]';
  try{localization.format('lab.named',{first:true,second:'B'});}catch(e){this.state.invalid=true;}
 }
})JS";
  AssetRecord script;Check(host.Assets().Track(scriptPath.string(),script,error),"catalog startup JS fixture registered normally");
  auto catalog=p.Settings().localization.locales.at("en").catalog;
  std::mutex gateMutex;std::condition_variable gate;bool decoded=false,publish=false;
  resources.SetTrace([&](const ResourceTraceEvent& e){if(e.asset==catalog&&e.point==ResourceTracePoint::DecodeEnd){std::unique_lock<std::mutex> lock(gateMutex);decoded=true;gate.notify_all();gate.wait(lock,[&]{return publish;});}});
  Scene probeScene;auto& owner=probeScene.CreateObject("catalog startup binding proof");owner.scripts.push_back({1,script.id,true,"{}"});
  RuntimeWorld probe;auto session=std::make_shared<SceneSession>(p,p.StartupScenePath());probe.SetSceneControl(session);
  Check(probe.Build(probeScene,&resources,error,&p.Settings().classification,&p.Settings().navigation),"catalog startup scripted world builds");
  probe.Localization().Refresh();
  {std::unique_lock<std::mutex> lock(gateMutex);Check(gate.wait_for(lock,std::chrono::seconds(10),[&]{return decoded;}),"actual catalog decode paused before asynchronous publication");}
  Check(probe.Localization().Format("lab.named",{{"first","A"},{"second","B"}},error)=="[lab.named]"&&error.rfind("localization catalog not yet published",0)==0,"unpublished catalog retains distinct non-missing diagnostic");
  probe.UpdateScripts(nullptr,.01f);auto before=probe.Scripts()->Capture();
  Check(probe.Scripts()->Diagnostics().empty()&&!before.empty()&&before[0].json.find("\"early\":true")!=std::string::npos,"JudasJS unpublished catalog returns visible placeholder without TypeError");
  {std::lock_guard<std::mutex> lock(gateMutex);publish=true;gate.notify_all();}
  resources.WaitForAll();resources.SetTrace({});probe.Localization().Refresh();probe.UpdateScripts(nullptr,.01f);auto after=probe.Scripts()->Capture();
  Check(probe.Scripts()->Diagnostics().empty()&&!after.empty()&&after[0].json.find("\"published\":true")!=std::string::npos&&after[0].json.find("\"missing\":true")!=std::string::npos&&after[0].json.find("\"invalid\":true")!=std::string::npos,"published JS lookup and genuine missing fallback work; invalid arguments still throw");
  Check(probe.Localization().Format("absent.integration.key",{},error)=="[absent.integration.key]"&&error=="missing localization key absent.integration.key","genuinely absent published key retains existing missing-key diagnostic");
  probe.Destroy();probe.SetSceneControl(nullptr);session.reset();
  Check(host.Assets().Remove(script.id,error),"startup-only test asset removed before unchanged-baseline fingerprint checks");
 }
 {LocalizationSession l(p.Settings().localization);l.Bind(&resources);l.Refresh();resources.WaitForAll();l.Refresh();auto old=l.Format("lab.caption",{},error);auto id=p.Settings().localization.locales.at("en").catalog;auto path=host.Assets().Find(id)->path;auto source=Read(path);auto revision=l.Revision();std::ofstream(path)<<"JudasCatalog 1 \"en\"\n\"bad\" \"{broken\"";l.Reload();resources.WaitForAll();l.Refresh();Check(resources.StateOf(id)==ResourceState::Failed&&l.Format("lab.caption",{},error)==old&&l.Revision()==revision,"invalid replacement never publishes partial catalog / prior message remains live");
  std::ofstream(path)<<source+"\"reload.proof\" \"Café 😀\"\n";l.Reload();resources.WaitForAll();l.Refresh();Check(l.Format("reload.proof",{},error)=="Café 😀"&&l.Revision()>revision,"valid replacement reload publishes revision and supplementary UTF-8");std::ofstream(path)<<source;l.Reload();resources.WaitForAll();l.Refresh();
 }
 // Force a font decode to finish after project data is replaced, using existing trace seam.
 auto font=p.Settings().localization.fonts[0];std::mutex mutex;std::condition_variable cv;bool reached=false,open=false;std::atomic<unsigned> discarded{0};resources.Invalidate(font);resources.SetTrace([&](const ResourceTraceEvent& e){if(e.asset==font&&e.point==ResourceTracePoint::StaleDiscarded)++discarded;if(e.asset==font&&e.point==ResourceTracePoint::CancelRequested){std::lock_guard<std::mutex> lock(mutex);open=true;cv.notify_all();}if(e.asset==font&&e.point==ResourceTracePoint::DecodeEnd){std::unique_lock<std::mutex> lock(mutex);reached=true;cv.notify_all();cv.wait(lock,[&]{return open;});}});resources.RequestFont(font);{std::unique_lock<std::mutex> lock(mutex);Check(cv.wait_for(lock,std::chrono::seconds(10),[&]{return reached;}),"real font decode worker reaches controlled completion");}resources.ReleaseAll();{std::lock_guard<std::mutex> lock(mutex);open=true;cv.notify_all();}resources.WaitForAll();Check(!resources.TryGetFont(font)&&discarded>0,"delayed old-generation font cannot publish after project release");resources.SetTrace({});
 Scene scene;Check(LoadSceneFromFile(p.StartupScenePath(),scene,error),"lab scene parses");std::string authored;SaveSceneToString(scene,authored);auto control=std::make_shared<SceneSession>(p,p.StartupScenePath());RuntimeWorld world;world.SetSceneControl(control);Check(world.Build(scene,&resources,error,&p.Settings().classification,&p.Settings().navigation),"session-bound world baseline builds");resources.WaitForAll();auto baseline=world.BaselineFingerprint();
 {Scene prefab;Scene copy=scene;SceneObjectId instance=0;Check(CreatePrefab(scene,scene.Objects()[0].id,prefab,error)&&InstantiatePrefab(copy,prefab,"0123456789abcdef0123456789abcdef",{},instance,error)&&copy.Find(instance)&&copy.Find(instance)->ui&&copy.Find(instance)->ui->asset==prefab.Objects()[0].ui->asset,"prefab instance keeps ordinary UI document/font/catalog asset references");}
 {AssetDatabase originalAssets;originalAssets.Scan("projects/text_lab","projects/text_lab/Assets");ResourceManager original(nullptr,&originalAssets);original.SetBlockingMode(true);Project sourceProject;sourceProject.Load("projects/text_lab/text_lab.judasproj",error);RuntimeWorld other;auto session=std::make_shared<SceneSession>(sourceProject,sourceProject.StartupScenePath());other.SetSceneControl(session);Check(other.Build(scene,&original,error,&sourceProject.Settings().classification,&sourceProject.Settings().navigation)&&other.BaselineFingerprint()==baseline,"authored localized fingerprint independent of project machine path");other.Destroy();other.SetSceneControl(nullptr);session.reset();}
 {auto id=p.Settings().localization.locales.at("en").catalog;auto path=host.Assets().Find(id)->path;auto bytes=Read(path);std::ofstream(path)<<bytes+"\"fingerprint.proof\" \"new authored content\"\n";RuntimeWorld other;other.SetSceneControl(control);Check(other.Build(scene,&resources,error,&p.Settings().classification,&p.Settings().navigation)&&other.BaselineFingerprint()!=baseline,"authored catalog byte revision changes baseline fingerprint");other.Destroy();other.SetSceneControl(nullptr);std::ofstream(path)<<bytes;}
 auto& locale=world.Localization();locale.Refresh();locale.SetLocale("ar",error);resources.WaitForAll();locale.Refresh();Check(world.BaselineFingerprint()==baseline,"transient locale is not an authored fingerprint input");{SceneSession replacement(p,p.StartupScenePath());Check(replacement.Localization(&resources).Locale()=="en","fresh Play/project session starts from authored default rather than leaked override");}
 auto oldLocale=std::locale();std::locale::global(std::locale(std::locale::classic(),new Comma));std::string invariant;SaveSceneToString(scene,invariant);ProjectSettings decoded;Check(invariant==authored&&Project::ParseFromString(Project::SerializeToString(p.Settings()),decoded,error)&&decoded.localization.Encode()==p.Settings().localization.Encode(),"custom process C++ numeric locale does not change scene/project/UI serialization");std::locale::global(oldLocale);
 RuntimeUI u(&resources);u.SetLocalization(&locale);UIDocument doc;UIElement root;root.id="canvas";root.kind=UIKind::Canvas;root.flow=UIFlow::Horizontal;root.mirrorRow=true;doc.elements.push_back(root);for(auto id:{"a","b"}){UIElement e;e.id=id;e.parent="canvas";e.kind=UIKind::Button;e.textKey="lab.caption";doc.elements.push_back(e);}auto first=u.Add(doc,"one",1,error),second=u.Add(doc,"two",2,error);u.Layout(640,360);Check(first!=second&&u.Element(first,"a")!=u.Element(second,"a"),"two documents have independent runtime elements");Check(u.LayoutOf(first,"a")->rect.position.x>u.LayoutOf(first,"b")->rect.position.x,"authored horizontal row explicitly mirrors under RTL locale");auto* a=u.Element(first,"a");a->textKey.clear();a->text="runtime override";Check(!u.Element(second,"a")->textKey.empty(),"per-instance key/text override does not mutate shared authoring");u.Unload(first);Check(!u.Element(first,"a"),"unloaded document handle remains invalid");u.Clear();world.Destroy();world.SetSceneControl(nullptr);control.reset();host.Shutdown();
 // Actual outer application frames, including paused language switch and safe scene reload.
 for(auto project:{"text_lab","shooter_game"}){ApplicationControl c;c.hidden=true;c.frameSeconds=[](float){return 1.f/60;};int frame=0;uint32_t document=0;bool reload=false;std::string before;size_t pauseElapsed=0;auto& profiler=PerformanceProfiler::Get();profiler.Clear();profiler.Enable(true);
  c.hostReady=[&](EngineHost& h){h.Audio().Init(error,true);h.GetWindow().SetTestInputMode(true);std::printf("GL application %s / %s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER));};
  c.worldReady=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay&){w.Localization().Refresh();h.Resources().WaitForAll();w.Localization().Refresh();document=w.UI().Find(std::string(project)=="text_lab"?"text_lab":"range_ui");Check(document!=0,"registered UI document loads in real application");};
  c.beforeFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& play){
   if(frame==10){w.Localization().SetLocale("ar",error);h.Resources().WaitForAll();w.Localization().Refresh();}
   if(std::string(project)=="shooter_game"){if(frame==20||frame==21){SDL_Event e{};e.type=frame==20?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=SDL_SCANCODE_ESCAPE;e.key.keysym.sym=SDLK_ESCAPE;SDL_PushEvent(&e);}if(frame==23){pauseElapsed=play.FixedStepsSinceReset();before=w.Scripts()->Capture().front().json;w.Localization().SetLocale("ja",error);h.Resources().WaitForAll();w.Localization().Refresh();}if(frame==28||frame==29){SDL_Event e{};e.type=frame==28?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=SDL_SCANCODE_ESCAPE;e.key.keysym.sym=SDLK_ESCAPE;SDL_PushEvent(&e);}}
   if(frame==36){w.SceneControl()->Request(w.SceneControl()->Current(),error);reload=true;}
  };
  c.afterFrame=[&](EngineHost& h,RuntimeWorld& w,InteractivePlay& play){++frame;
   if(frame==15){Check(w.Localization().Locale()=="ar"&&w.Scripts()->Diagnostics().empty(),"playing locale changes without script faults");std::vector<unsigned char> pixels;h.GetRenderer().CaptureFrame(h.GetWindow().Width(),h.GetWindow().Height(),pixels);WriteRgbPng((out/(std::string(project)+"-ar.png")).string(),h.GetWindow().Width(),h.GetWindow().Height(),pixels);}
   if(frame==27&&std::string(project)=="shooter_game"){Check(w.UI().Paused()&&w.Localization().Locale()=="ja"&&!w.pointerCapture,"paused language change preserves modal state and released capture");Check(play.FixedStepsSinceReset()==pauseElapsed,"paused localization does not advance gameplay time");std::vector<unsigned char> pixels;h.GetRenderer().CaptureFrame(h.GetWindow().Width(),h.GetWindow().Height(),pixels);WriteRgbPng((out/"range-ja-paused.png").string(),h.GetWindow().Width(),h.GetWindow().Height(),pixels);}
   if(frame==33&&std::string(project)=="shooter_game")Check(!w.UI().Paused()&&w.pointerCapture,"resume after locale change restores pointer policy");
   if(frame==40)Check(reload&&w.Localization().Locale()==(std::string(project)=="text_lab"?"ar":"ja")&&w.Scripts()->Diagnostics().empty(),"safe scene reload reconstructs world but retains session locale");
   if(frame==90){auto stats=h.GetRenderer().TextStats();std::printf("APPLICATION %s fixed_ms=%.4f glyph_rasters=%llu atlas_bytes=%zu font_bytes=%zu layout_bytes=%zu bodies=%zu\n",project,play.LastFixedStepMilliseconds(),(unsigned long long)stats.rasters,h.GetRenderer().TextAtlasBytes(),stats.fontBytes,stats.layoutBytes,w.CountLifecycle().physicsBodies);}
   if(frame==120)w.UI().RequestQuit();
  };
  c.beforeShutdown=[&](EngineHost&,RuntimeWorld& w,InteractivePlay&){Check(w.Scripts()->Diagnostics().empty(),"final application scripts fault-free");Check(profiler.Export((out/(std::string(project)+"-profile.json")).string(),error),"M56 application text/locale profile exports");};
  auto path="projects/"+std::string(project)+"/"+project+".judasproj";char* args[]={const_cast<char*>("judas"),path.data()};Application app;Check(app.Run(2,args,&c)==0&&frame==120,"application startup/render/reload/Stop completed");profiler.Enable(false);
 }
 std::printf("SUMMARY %d checks %d failures\n",checks,failures);return failures?1:0;}
