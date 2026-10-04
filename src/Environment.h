#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
struct EnvironmentLevel {unsigned width=0,height=0;std::vector<glm::vec3> pixels;};
struct EnvironmentData {std::string sourceHash;std::vector<EnvironmentLevel> specular;EnvironmentLevel diffuse;unsigned brdfSize=0;std::vector<glm::vec2> brdf;};
// Content/settings/version keyed offline CPU bake. No convolution at runtime.
bool BakeEnvironment(const std::string& hdrPath,unsigned width,unsigned samples,EnvironmentData&,std::string&);
bool SaveEnvironment(const std::string&,const EnvironmentData&,std::string&);
bool DecodeEnvironment(const std::vector<unsigned char>&,EnvironmentData&,std::string&);
bool LoadEnvironment(const std::string&,EnvironmentData&,std::string&);
