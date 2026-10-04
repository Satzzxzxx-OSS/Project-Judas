#pragma once
#include "MeshData.h"
// MikkTSpace uses per-corner output. Expand the indexed stream to preserve
// mirrored UV handedness instead of overwriting a shared vertex's tangent.
bool GenerateMeshTangents(MeshData& mesh);
