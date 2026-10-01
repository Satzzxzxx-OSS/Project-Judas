#pragma once

#include "Scene.h"
#include "AssetDatabase.h"

// Prefabs reuse scene object blocks. Asset IDs are resolved only at this
// authored-data boundary; simulation sees ordinary resolved components.
using PrefabProperties = std::map<std::string, std::string>;
PrefabProperties ObjectProperties(const SceneObject& object);
bool ApplyObjectProperties(SceneObject& object, const PrefabProperties& properties, std::string& error);
std::string EncodePrefabOverrides(const PrefabProperties& properties);
bool DecodePrefabOverrides(const std::string& text, PrefabProperties& properties, std::string& error);
bool ValidateHierarchy(const Scene& scene, std::string& error);
bool FlattenHierarchy(const Scene& scene, Scene& flattened, std::string& error);
bool ValidatePrefab(const Scene& prefab, std::string& error);
bool LoadPrefab(const AssetDatabase& assets, const AssetId& asset, Scene& prefab, std::string& error);
bool CreatePrefab(const Scene& scene, SceneObjectId root, Scene& prefab, std::string& error);
bool InstantiatePrefab(Scene& scene, const Scene& prefab, const AssetId& asset,
                       const SceneTransform& placement, SceneObjectId& root, std::string& error);
// Resolution is transactional. Stable mapping retains IDs even when a source
// child is removed, so reintroducing it does not retarget another child's override.
bool ResolvePrefabs(const Scene& scene, const AssetDatabase* assets, Scene& resolved, std::string& error);
void CapturePrefabEdits(const Scene& before, Scene& after);
bool RevertPrefabProperty(Scene& scene, SceneObjectId id, const std::string& key,
                          const AssetDatabase& assets, std::string& error);
bool ApplyPrefabSource(Scene& scene, SceneObjectId root, const AssetDatabase& assets, std::string& error);
