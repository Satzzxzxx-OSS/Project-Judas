#pragma once

#include <cstdint>
#include <vector>
#include <memory>
struct SkeletalAsset;

#include <glm/glm.hpp>

// Judas-owned, importer-agnostic CPU-side mesh representation. This is what
// a model importer (src/ModelLoader.h) produces and what Renderer::CreateMesh
// consumes to upload GPU buffers — nothing above this point in the engine
// (Application, gameplay code) ever needs to know an OBJ file or
// tinyobjloader exists; it only ever sees a MeshData. See
// docs/ARCHITECTURE.md, "Milestone 9," for the ownership boundary this
// enforces.
//
// Deliberately minimal: position/normal/uv per vertex is exactly what this
// milestone's lighting + texturing needs, nothing more (no tangents/
// bitangents — no normal mapping exists to need them; no vertex color, bone
// weights in the static stream, or per-vertex material index). M46 skin
// indices/weights live in a separate optional stream.
struct MeshVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec2 uv{0.0f};

};

struct MeshSkinVertex {glm::uvec4 joints{0};glm::vec4 weights{1,0,0,0};};

struct MeshData {
    std::vector<MeshSkinVertex> skinVertices;
    std::shared_ptr<const SkeletalAsset> skeletal;
    std::vector<MeshVertex> vertices;
    // Empty means "draw non-indexed" (glDrawArrays) — Judas's existing
    // built-in cube/sphere primitives use this; an imported model always
    // populates it (glDrawElements). 32-bit indices: this engine's meshes
    // are all small (a demo's worth of hand-authored/imported geometry),
    // so there's no evidence 16-bit indexing is worth the added complexity
    // of two index-type code paths.
    std::vector<std::uint32_t> indices;
};
