#pragma once
#include "NavigationTypes.h"
#include "Scene.h"
#include <memory>
// Immutable CPU resource. Runtime worlds build independent mutable Detour caches.
struct NavigationData {
 NavigationSurfaceSettings settings; NavigationProfile profile;
 std::string fingerprint; std::vector<std::vector<unsigned char>> layers;
};
struct NavigationGeometry {
 std::vector<float> vertices; std::vector<int> triangles;
 struct Volume {std::vector<float> polygon;float bottom=0,top=0;unsigned area=0;bool blocked=false;};
 std::vector<Volume> modifiers; std::string fingerprint;
};
bool CollectNavigationGeometry(const Scene&,const SceneObject&,const ProjectNavigation&,NavigationGeometry&,std::string&);
bool DecodeNavigation(const std::vector<unsigned char>&,NavigationData&,std::string&);
bool LoadNavigation(const std::string&,NavigationData&,std::string&);
bool SaveNavigation(const std::string&,const NavigationData&,std::string&);
// Editor/tool target only; never linked into standalone runtime.
bool BakeNavigation(const Scene&,const SceneObject&,const ProjectNavigation&,NavigationData&,std::string&);
