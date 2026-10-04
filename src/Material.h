#pragma once
#include "TextureData.h"
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
// Factors are scene-linear. Images stay RGBA8; shader interpretation is per
// material role, so sharing one image between colour and data is safe.
enum class MaterialModel { Legacy, PBR, Unlit };
enum class MaterialAlpha { Opaque, Mask, Blend };
struct MaterialSampler {int wrapS=10497,wrapT=10497,minFilter=9987,magFilter=9729;};
struct MaterialMap {std::string asset;TextureData embedded;MaterialSampler sampler;};
struct MaterialDefinition {
 MaterialModel model=MaterialModel::PBR;
 MaterialAlpha alpha=MaterialAlpha::Opaque;
 glm::vec4 baseColor{1};float metallic=0,roughness=.5f;
 glm::vec3 emissive{0};float emissiveIntensity=1,normalStrength=1,occlusionStrength=1,alphaCutoff=.5f;
 bool doubleSided=false,flipV=false;glm::vec2 uvScale{1},uvOffset{0};
 // base colour, metallic-roughness, normal, occlusion, emissive
 std::array<MaterialMap,5> maps;
};
struct MaterialOverride {
 std::optional<glm::vec4> baseColor;
 std::optional<float> metallic,roughness,emissiveIntensity;
 std::optional<glm::vec3> emissive;
};
struct MaterialSlot {std::string asset;MaterialOverride overrides;};
bool ValidateMaterial(const MaterialDefinition&,std::string& error);
bool ParseMaterial(const std::string&,MaterialDefinition&,std::string& error);
std::string SerializeMaterial(const MaterialDefinition&);
bool LoadMaterial(const std::string&,MaterialDefinition&,std::string& error);
bool SaveMaterial(const std::string&,const MaterialDefinition&,std::string& error);
MaterialDefinition ApplyMaterialOverride(const MaterialDefinition&,const MaterialOverride&);
std::string EncodeMaterialSlots(const std::vector<MaterialSlot>&);
bool DecodeMaterialSlots(const std::string&,std::vector<MaterialSlot>&,std::string& error);
