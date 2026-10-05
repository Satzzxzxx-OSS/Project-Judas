// Content builder: uses normal scene/prefab/bake APIs, not a runtime demo branch.
#include "WorldStreaming.h"
#include "SceneSerialization.h"
#include "Prefab.h"
#include "NavigationAsset.h"
#include "LiquidTypes.h"
#include <fstream>
#include <filesystem>
#include <cstdio>
int main(){namespace fs=std::filesystem;std::string error;Project project;auto root=fs::path("projects/streamed_range");
 if(!project.Load((root/"streamed_range.judasproj").string(),error)){std::puts(error.c_str());return 1;}AssetDatabase assets;assets.Scan(project.RootDir(),project.AssetsDir());
 Scene original;if(!LoadSceneFromFile("projects/shooter_game/Scenes/range.judas",original,error))return 1;
 Scene bootstrap;bootstrap.Settings()=original.Settings();bootstrap.Settings().name="Streamed Spring Range";bootstrap.SetNextId(2000);
 for(auto id:{1ull,2ull,10ull,11ull,12ull,20ull,21ull})bootstrap.InsertObject(*original.Find(id));
 auto* floor=bootstrap.Find(1);floor->transform.position={0,-.5f,0};floor->body->halfExtents={12,.5f,12};floor->render->halfExtents=floor->body->halfExtents;
 bootstrap.Find(10)->transform.position={0,.9f,8};bootstrap.Find(2)->gravity->regionRadius=300;
 SceneObject rootNav;rootNav.id=550;rootNav.navigationSurface=NavigationSurfaceSettings{};rootNav.navigationSurface->halfExtents={12,4,12};rootNav.navigationSurface->minRegion=1;bootstrap.InsertObject(rootNav);
 SceneObject rootLink;rootLink.id=552;rootLink.navigationLink=NavigationLinkSettings{};rootLink.navigationLink->start={5,.1f,-11};rootLink.navigationLink->end={5,.1f,-13};rootLink.navigationLink->radius=1;bootstrap.InsertObject(rootLink);
 auto bake=[&](Scene& scene,const std::string& name){Scene resolved,flat;if(!ResolvePrefabs(scene,&assets,resolved,error)||!FlattenHierarchy(resolved,flat,error))return false;
  for(auto& o:scene.Objects())if(o.navigationSurface){NavigationData data;if(!BakeNavigation(flat,*flat.Find(o.id),project.Settings().navigation,data,error))return false;fs::create_directories(root/"Assets/navigation");auto path=root/"Assets/navigation"/(name+".judasnav");if(!SaveNavigation(path.string(),data,error))return false;AssetRecord record;if(!assets.Track(path.string(),record,error)){auto* old=assets.FindByRelativePath(project.MakeRelative(path.string()));if(!old)return false;record=*old;}o.navigationSurface->asset=record.id;}return true;};
 if(!bake(bootstrap,"root")||!SaveSceneToFile(bootstrap,(root/"Scenes/bootstrap.judas").string(),error)){std::puts(error.c_str());return 1;}
 for(unsigned i=0;i<6;++i){Scene region;region.SetNextId(2000);region.Settings().name="Gallery "+std::to_string(i+1);region.InsertObject(*floor);
  for(auto id:{100ull,101ull,102ull,103ull,104ull,105ull})region.InsertObject(*original.Find(id));
  region.InsertObject(*original.Find(30));region.Find(30)->transform.position={4,1,0};
  for(int side:{-1,1}){SceneObject wall;wall.id=side<0?510:511;wall.transform.position={float(side)*12,1.5f,0};wall.body=SceneBodyComponent{};wall.body->halfExtents={.2f,2,12};wall.render=SceneRenderComponent{};wall.render->halfExtents=wall.body->halfExtents;wall.render->color={.08f,.14f,.18f};region.InsertObject(wall);}
  auto surface=rootNav;region.InsertObject(surface);SceneObject link;link.id=552;link.navigationLink=NavigationLinkSettings{};link.navigationLink->start={5,.1f,-11};link.navigationLink->end={5,.1f,-13};link.navigationLink->radius=1;region.InsertObject(link);
  SceneObject sign;sign.id=60;sign.transform.position={0,3,-6};sign.transform.scale={2,2,2};sign.render=SceneRenderComponent{};sign.render->shape=SceneShape::Mesh;sign.render->meshAsset="5959595959595959595959595959500"+std::to_string(i);sign.render->color={.2f+.1f*i,.6f,.8f-.1f*i};region.InsertObject(sign);
  if(i==0){region.InsertObject(*original.Find(1000));region.Find(1000)->transform.position={5,1,0};}
  if(!bake(region,"gallery-"+std::to_string(i))||!SaveSceneToFile(region,(root/"Scenes"/("gallery-"+std::to_string(i)+".judas")).string(),error)){std::puts(error.c_str());return 1;}
 }
 // A deliberately small conserved liquid group. Its quantity is never suspended.
 Scene water;water.SetNextId(40);water.Settings().name="Pinned liquid exhibit";water.InsertObject(*floor);water.Find(1)->body->halfExtents={3,.5f,3};water.Find(1)->render->halfExtents={3,.5f,3};
 SceneObject g;g.id=3;g.gravity=SceneGravityComponent{};g.gravity->kind=SceneGravityKind::Uniform;g.gravity->regionRadius=8;water.InsertObject(g);
 fs::create_directories(root/"Assets/liquid");auto geometry=LiquidBox({-1,0,-1},{1,1,1});auto cavity=root/"Assets/liquid/exhibit.judascavity";std::ofstream f(cavity);f<<"JudasCavity1 "<<geometry.cells.size()<<"\n";for(auto& t:geometry.cells){for(auto& v:t)f<<v.x<<" "<<v.y<<" "<<v.z<<" ";f<<"\n";}f.close();AssetRecord cavityAsset;if(!assets.Track(cavity.string(),cavityAsset,error)){auto* old=assets.FindByRelativePath(project.MakeRelative(cavity.string()));if(!old)return 1;cavityAsset=*old;}
 SceneObject basin;basin.id=10;basin.liquidBasin=LiquidBasinSettings{};basin.liquidBasin->geometry=cavityAsset.id;basin.liquidBasin->initialVolume=1;water.InsertObject(basin);
 for(unsigned k=0;k<4;++k){SceneObject wall;wall.id=20+k;wall.body=SceneBodyComponent{};wall.render=SceneRenderComponent{};bool x=k<2;wall.transform.position={x?(k==0?-1.1f:1.1f):0,.5f,x?0:(k==2?-1.1f:1.1f)};wall.body->halfExtents=x?glm::vec3(.1f,.6f,1.2f):glm::vec3(1,.6f,.1f);wall.render->halfExtents=wall.body->halfExtents;wall.render->color={.3f,.4f,.5f};water.InsertObject(wall);}
 if(!SaveSceneToFile(water,(root/"Scenes/liquid.judas").string(),error))return 1;
 // Oblique and radial source fixture: absolute origin is intentionally huge.
 Scene oblique=water;oblique.Objects().clear();oblique.Settings().name="Oblique radial placement";oblique.Settings().worldOrigin={1e12,1e12,1e12};SceneObject sphere;sphere.id=1;sphere.body=SceneBodyComponent{};sphere.body->shape=SceneShape::Sphere;sphere.body->radius=5;sphere.render=SceneRenderComponent{};sphere.render->shape=SceneShape::Sphere;sphere.render->radius=5;oblique.InsertObject(sphere);g.gravity->kind=SceneGravityKind::Radial;g.gravity->regionRadius=20;oblique.InsertObject(g);SaveSceneToFile(oblique,(root/"Scenes/oblique.judas").string(),error);
 std::puts("M59 ordinary scenes and source-validated navigation bakes written");return 0;
}
