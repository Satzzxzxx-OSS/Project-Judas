#pragma once
#include "MeshData.h"
#include <string>
bool ParseGltfMesh(const void* bytes,size_t size,MeshData& mesh,std::string& error);
