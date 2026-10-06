#pragma once
#include "Scene.h"
class ResourceManager;
bool ExtendedCompound(const SceneBodyComponent&);
bool ResolveBodyCollision(const SceneBodyComponent&,ResourceManager*,Shape&,std::string&);

bool ValidateCollisionFluid(const SceneObject&,bool legacyParticleWater,std::string&);
