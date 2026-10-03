#pragma once
#include "Classification.h"
#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>
struct NavigationProfile {std::string name="Default";float radius=.3f,height=1.8f,slope=50,climb=.35f;};
struct ProjectNavigation {
 ProjectNavigation(){areas.names.emplace(0,"Default");areas.nextId=1;}
 CategoryRegistry areas;
 std::map<unsigned,NavigationProfile> profiles{{0,{}}};unsigned nextProfile=1;
 bool Validate(std::string&) const;
 std::string Serialize() const;
 static bool Parse(const std::string&,ProjectNavigation&,std::string&);
};
struct NavigationSurfaceSettings {bool enabled=true,includeDynamic=false;unsigned profile=0;CategoryMask sources=kAllCategories;glm::vec3 halfExtents{32,8,32};float cellSize=.2f,cellHeight=.1f,simplification=1.3f;int tileSize=32,minRegion=8;std::string asset;};
struct NavigationAgentSettings {bool enabled=true,avoidance=true;unsigned profile=0;CategoryMask areas=kAllCategories;std::map<unsigned,float> costs;float speed=3,arrival=.3f,cornerDistance=.25f,repathSeconds=.5f;};
struct NavigationObstacleSettings {bool enabled=true;bool cylinder=false;glm::vec3 halfExtents{.5f};float radius=.5f,height=2,updateDistance=.1f;};
struct NavigationLinkSettings {bool enabled=true,bidirectional=true;glm::vec3 start{0},end{0,0,-2};float radius=.5f;unsigned area=0;};
struct NavigationModifierSettings {bool enabled=true,excludeSource=false,blocked=false;glm::vec3 halfExtents{1};unsigned area=0;};
struct SceneObject;
// Ordinary scene property fields, also consumed by prefab property overrides.
std::map<std::string,std::string> NavigationProperties(const SceneObject&);
bool ApplyNavigationProperties(const std::map<std::string,std::string>&,SceneObject&,std::string&);
bool ValidateNavigationComponents(const SceneObject&,std::string&);
