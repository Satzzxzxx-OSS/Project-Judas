#pragma once
#include <cstdint>
#include <map>
#include <string>

using CategoryMask = std::uint64_t;
constexpr CategoryMask kAllCategories = ~CategoryMask{0};
constexpr CategoryMask CategoryBit(unsigned id) { return id < 64 ? CategoryMask{1} << id : 0; }

// IDs are bit positions, not row positions. Deleted IDs remain retired. The
// monotonic next ID is persisted; rename/reorder never retargets authored data.
struct CategoryRegistry {
    std::map<unsigned,std::string> names;
    unsigned nextId = 0;
    bool Add(const std::string& name, unsigned& id);
    bool Rename(unsigned id, const std::string& name);
    bool Remove(unsigned id, bool preserveDefault=false);
    CategoryMask ActiveMask() const;
    bool Validate(std::string& error) const;
    int Find(const std::string& name) const;
};
struct ProjectClassification {
    CategoryRegistry tags;
    CategoryRegistry collision{{{0,"Default"}},1};
    CategoryRegistry render{{{0,"Default"}},1};
    bool Validate(std::string& error) const;
    bool IsDefault() const;
    std::string Serialize() const;
    static bool Parse(const std::string& text, ProjectClassification& out, std::string& error);
};
inline bool CollisionPermitted(unsigned a, CategoryMask maskA, unsigned b, CategoryMask maskB) {
    return (maskA & CategoryBit(b)) && (maskB & CategoryBit(a));
}

class Scene;
bool ValidateSceneClassification(const Scene&,const ProjectClassification&,std::string& error);
