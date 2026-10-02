#include "GamePackage.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

bool ReadGamePackage(const std::string& root, GamePackage& package, std::string& error) {
    std::ifstream file(std::filesystem::path(root) / kGamePackageMarker);
    std::string tag, project, save, trailing;
    int version = 0;
    if (!(file >> tag >> version) || tag != "JudasPackage" || version != 1 ||
        !(file >> tag >> std::quoted(project)) || tag != "project" ||
        !(file >> tag >> std::quoted(save)) || tag != "save-id" || (file >> trailing) ||
        std::filesystem::path(project).filename() != project ||
        std::filesystem::path(project).extension() != ".judasproj" || save.size() != 64 ||
        save.find_first_not_of("0123456789abcdef") != std::string::npos) {
        error = "Invalid or unsupported game package launch record: " + root;
        return false;
    }
    package = {project, save};
    return true;
}

bool PackageSaveDirectory(const GamePackage& package, std::string& directory, std::string& error) {
    const char* xdg = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    std::filesystem::path base;
    if (xdg && *xdg) base = xdg;
    else if (home && *home) base = std::filesystem::path(home) / ".local/share";
    if (base.empty() || !base.is_absolute()) {
        error = "Packaged runtime requires an absolute XDG_DATA_HOME or HOME for writable saves";
        return false;
    }
    directory = (base / "judas/games" / package.saveId / "Saves").generic_string();
    return true;
}
