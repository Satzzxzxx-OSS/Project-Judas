#pragma once
#include <string>

// Small launch record, not a second asset database. All content still uses
// Project/AssetDatabase. Paths are relative to the executable's directory.
struct GamePackage {
    std::string projectFile;
    std::string saveId;
};
constexpr const char* kGamePackageMarker = "judas-package.txt";
bool ReadGamePackage(const std::string& root, GamePackage& package, std::string& error);
bool PackageSaveDirectory(const GamePackage& package, std::string& directory, std::string& error);
