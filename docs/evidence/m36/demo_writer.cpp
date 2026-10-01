// Reproducible authored content generation; uses actual engine APIs.
#include "Prefab.h"
#include "SceneSerialization.h"
#include <filesystem>
#include <cstdio>
int main(){
    std::string error;Scene prefab;prefab.Settings().name="Reusable beacon";
    auto& root=prefab.CreateObject("Beacon base");const auto rootId=root.id;
    root.render=SceneRenderComponent{};root.render->color={0.1f,0.4f,0.9f};root.render->halfExtents={0.6f,0.4f,0.6f};
    root.body=SceneBodyComponent{};root.body->motion=SceneBodyMotion::Dynamic;root.body->halfExtents=root.render->halfExtents;root.body->mass=10;
    auto& orb=prefab.CreateObject("Beacon lamp");orb.parent=rootId;orb.transform.position={0,1,0};orb.render=SceneRenderComponent{};orb.render->shape=SceneShape::Sphere;orb.render->radius=0.35f;orb.render->color={0.2f,0.9f,0.4f};orb.light=SceneLightComponent{};orb.light->color={0.2f,3,0.4f};orb.light->range=6;
    auto& cap=prefab.CreateObject("Lamp cap");cap.parent=orb.id;cap.transform.position={0,0.5f,0};cap.render=SceneRenderComponent{};cap.render->halfExtents={0.2f,0.1f,0.2f};cap.render->color={0.8f,0.8f,0.8f};
    std::filesystem::create_directories("assets/prefabs");
    if(!SaveSceneToFile(prefab,"assets/prefabs/beacon.judasprefab",error))return 1;
    AssetDatabase assets;assets.Scan(std::filesystem::current_path().string(),(std::filesystem::current_path()/"assets").string());AssetRecord record;
    if(!assets.Track("assets/prefabs/beacon.judasprefab",record,error,"36363636363636363636363636363636")){std::puts(error.c_str());return 1;}
    AssetDatabase::WriteMeta("assets/prefabs/beacon.judasprefab.judasmeta",record.id,AssetType::Prefab,"Authored Judas M36 demo",error);
    Scene scene;scene.Settings().name="Prefab workshop";scene.Settings().ambientColor={0.3f,0.3f,0.3f};
    auto& ground=scene.CreateObject("Ground");ground.transform.position={0,-0.5f,0};ground.render=SceneRenderComponent{};ground.render->halfExtents={12,0.5f,12};ground.render->color={0.2f,0.2f,0.25f};ground.body=SceneBodyComponent{};ground.body->halfExtents=ground.render->halfExtents;
    auto& player=scene.CreateObject("Player start");player.transform.position={0,1,7};player.playerStart=ScenePlayerStartComponent{};player.playerStart->view=ScenePlayerView::FirstPerson;
    SceneTransform placement;placement.position={-2,0.5f,0};SceneObjectId a,b;
    if(!InstantiatePrefab(scene,prefab,record.id,placement,a,error))return 1;
    placement.position.x=2;if(!InstantiatePrefab(scene,prefab,record.id,placement,b,error))return 1;
    Scene before=scene;scene.Find(b)->render->color={1,0.7f,0.05f};CapturePrefabEdits(before,scene);
    return SaveSceneToFile(scene,"assets/scenes/prefab_demo.judas",error)?0:1;
}
