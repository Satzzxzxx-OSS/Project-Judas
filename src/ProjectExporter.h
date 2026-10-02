#pragma once
#include "Project.h"
#include <cstdint>

struct ProjectExportOptions {
    std::string destination; // exact package directory, not its parent
    std::string runtimeExecutable; // empty: sibling Release judas
    std::string engineDataRoot; // empty: resolve installed/development engine data
};
struct ProjectExportResult {
    std::string packageDirectory;
    std::size_t assetCount = 0, sceneCount = 0;
    std::uintmax_t bytes = 0;
    double seconds = 0;
};
// Linux desktop baseline. Copies a verified Release runtime, all registered
// assets (including runtime-only IDs), and all project scenes. Transactional
// staging: existing packages are replaced, arbitrary directories are refused.
bool ExportProject(const Project& project, const ProjectExportOptions& options,
                   ProjectExportResult& result, std::string& error);
