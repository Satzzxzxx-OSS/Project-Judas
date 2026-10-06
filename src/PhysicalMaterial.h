#pragma once
#include <string>
struct PhysicalMaterial {float friction=.6f,restitution=.1f;};
bool ValidPhysicalMaterial(const PhysicalMaterial&,std::string& error);
bool ParsePhysicalMaterial(const std::string&,PhysicalMaterial&,std::string& error);
bool LoadPhysicalMaterial(const std::string&,PhysicalMaterial&,std::string& error);
std::string SerializePhysicalMaterial(const PhysicalMaterial&);
