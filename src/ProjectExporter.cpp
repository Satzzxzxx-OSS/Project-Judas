#include "Material.h"
#include "Environment.h"
#include "NavigationAsset.h"
#include "LiquidTypes.h"
#include "ProjectExporter.h"
#include "ScriptSystem.h"
#include "AssetDatabase.h"
#include "EnginePaths.h"
#include "GamePackage.h"
#include "Prefab.h"
#include "RuntimeUI.h"
#include "SceneFingerprint.h"
#include "SceneSerialization.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {
void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool Inside(const fs::path& path, const fs::path& root) {
    const auto relative = path.lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
}
void Copy(const fs::path& from, const fs::path& to) {
    fs::create_directories(to.parent_path());
    fs::copy_file(from, to, fs::copy_options::overwrite_existing);
}
// Query the executable itself, rather than trusting a possibly stale adjacent
// build file. No shell, renderer, application initialization or asset loading.
bool ReleaseRuntime(const fs::path& executable) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return false;
    const pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO); close(pipefd[1]);
        execl(executable.c_str(), executable.c_str(), "--build-info", nullptr);
        _exit(127);
    }
    close(pipefd[1]);
    if (pid < 0) { close(pipefd[0]); return false; }
    std::string text; char buffer[128]; ssize_t count;
    while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        if (text.size() < 1024) text.append(buffer, static_cast<std::size_t>(count));
    }
    close(pipefd[0]); int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 && text == "Judas runtime Linux Release\n";
}
void References(const Scene& scene, const AssetDatabase& assets, const ProjectClassification& categories,const ProjectNavigation& navigation) {
    const auto checkUI=[&](const SceneObject& o){if(!o.ui)return;auto* r=assets.Find(o.ui->asset);UIDocument d;std::string error;Require(r&&!r->missing&&r->type==AssetType::UI,"Missing UI document "+o.ui->asset);Require(LoadUIDocument(r->path,d,error)&&ValidateUIAssets(d,assets,error),"UI: "+error);};
    const auto check = [&](const AssetId& id, AssetType type) {
        if (id.empty()) return;
        const auto* record = assets.Find(id);
        Require(record && !record->missing && record->type == type,
                "Missing or wrong-type required " + std::string(AssetTypeName(type)) + " asset " + id);
    };
    check(scene.Settings().environmentAsset,AssetType::Environment);
    for (const auto& object : scene.Objects()) {
        checkUI(object);
        check(object.prefabAsset, AssetType::Prefab);
        if (object.render) {
            check(object.render->meshAsset, AssetType::Mesh);
            check(object.render->textureAsset, AssetType::Texture);
            for(auto& slot:object.render->materials)check(slot.asset,AssetType::Material);
        }
        for(const auto& slot:object.scripts)check(slot.asset,AssetType::Script);
        if (object.audioEmitter) check(object.audioEmitter->asset, AssetType::Audio);
        if (object.particleEmitter) check(object.particleEmitter->textureAsset, AssetType::Texture);
    }
    Scene resolved; std::string error;
    Require(ResolvePrefabs(scene, &assets, resolved, error), "Prefab resolution: " + error);
    Require(ValidateSceneClassification(resolved,categories,error),"Classification: "+error);
    std::string scripts;Require(ScriptSystem::SourceFingerprint(assets,resolved,scripts,error),"Scripts: "+error);
    // Overrides/source content must be validated too, not just placeholders.
    Scene flattened;Require(FlattenHierarchy(resolved,flattened,error),error);
    for (const auto& object : flattened.Objects()) {
        if(object.liquidBasin){check(object.liquidBasin->geometry,AssetType::Liquid);check(object.liquidBasin->asset,AssetType::Liquid);auto* r=assets.Find(object.liquidBasin->asset);LiquidBasinData b;Require(r&&LoadLiquidBasin(r->path,b,error),"Liquid basin: "+error);Require(LiquidSourceFingerprint(flattened,object,assets,error)==b.fingerprint,"Stale liquid basin: "+error);}
        if(object.liquidContainer){check(object.liquidContainer->geometry,AssetType::Liquid);auto* r=assets.Find(object.liquidContainer->geometry);LiquidGeometry g;Require(r&&LoadLiquidGeometry(r->path,g,error),"Container cavity: "+error);double capacity=0;for(auto& t:g.cells)capacity+=LiquidClip(t,{0,0,0,0},1).volume;Require(object.liquidContainer->initialVolume<=capacity,"Container initial volume exceeds capacity");}
        if(object.navigationSurface && object.navigationSurface->enabled){check(object.navigationSurface->asset,AssetType::Navigation);const auto* record=assets.Find(object.navigationSurface->asset);NavigationData data;NavigationGeometry geometry;Require(record&&LoadNavigation(record->path,data,error),"Navigation: "+error);Require(CollectNavigationGeometry(flattened,object,navigation,geometry,error)&&geometry.fingerprint==data.fingerprint,"Stale navigation bake: "+error);}
        checkUI(object);
        if (object.render) {
            check(object.render->meshAsset, AssetType::Mesh);
            check(object.render->textureAsset, AssetType::Texture);
            for(auto& slot:object.render->materials)check(slot.asset,AssetType::Material);
        }
        for(const auto& slot:object.scripts)check(slot.asset,AssetType::Script);
        if (object.audioEmitter) check(object.audioEmitter->asset, AssetType::Audio);
        if (object.particleEmitter) check(object.particleEmitter->textureAsset, AssetType::Texture);
    }
}
}

bool ExportProject(const Project& project, const ProjectExportOptions& options,
                   ProjectExportResult& result, std::string& error) {
    const auto start = std::chrono::steady_clock::now();
    fs::path staging, backup;
    try {
        Require(project.IsLoaded(), "Open a project before exporting");
        ProjectSettings validated; std::string validationError;
        const bool settingsValid = Project::ParseFromString(Project::SerializeToString(project.Settings()), validated, validationError);
        Require(settingsValid, "Invalid project settings: " + validationError);
        Require(!options.destination.empty(), "Choose a package destination directory");
        const auto root = fs::canonical(project.RootDir());
        const auto destination = fs::weakly_canonical(fs::absolute(options.destination));
        Require(!Inside(destination, root) && !Inside(root, destination) && destination != root,
                "Export destination must be outside the source project and must not contain it");
        const auto executable = fs::absolute(options.runtimeExecutable.empty()
            ? fs::path(EngineExecutableDir()) / "judas" : fs::path(options.runtimeExecutable));
        Require(fs::is_regular_file(executable) && ReleaseRuntime(executable),
                "A runnable Release judas executable is required: " + executable.string());
        const auto engineRoot = options.engineDataRoot.empty()
            ? fs::path(ResolveEngineDataPath("assets/fonts/DejaVuSans.ttf")).parent_path().parent_path().parent_path()
            : fs::path(options.engineDataRoot);
        Require(fs::is_regular_file(engineRoot / "assets/fonts/DejaVuSans.ttf"), "Missing engine UI font");
        Require(fs::is_regular_file(engineRoot / "third_party/RUNTIME_NOTICES.txt"), "Missing runtime dependency license notices");
        AssetDatabase assets; assets.Scan(project.RootDir(), project.AssetsDir());
        Require(assets.Problems().empty(), assets.Problems().empty() ? "" :
                "Asset database: " + assets.Problems().front().path + ": " + assets.Problems().front().message);
        std::set<fs::path> scenes;
        Require(!project.Settings().startupScene.empty(), "Project has no startup scene");
        const auto startup = fs::absolute(project.StartupScenePath()).lexically_normal();
        Require(fs::canonical(startup) == startup, "Startup scene must use its real project-relative path, not a symlink alias");
        scenes.insert(startup);
        if (fs::exists(project.ScenesDir())) {
            for (const auto& entry : fs::recursive_directory_iterator(project.ScenesDir()))
                if (entry.is_regular_file() && entry.path().extension() == ".judas") {
                    Require(fs::canonical(entry.path()) == fs::absolute(entry.path()).lexically_normal(), "Scene symlink aliases are not exportable: " + entry.path().string());
                    scenes.insert(fs::canonical(entry.path()));
                }
        }
        const auto safeContentPath = [&](const fs::path& relative) {
            Require(!relative.empty() && !relative.is_absolute() && Inside((root / relative).lexically_normal(), root), "Unsafe package content path: " + relative.string());
            const auto first = relative.begin()->string();
            Require(first != "engine" && first != "judas" && first != "game.judasproj" &&
                    first != kGamePackageMarker && first != "RUNTIME_REQUIREMENTS.txt",
                    "Project content collides with reserved package path: " + relative.string());
        };
        safeContentPath(project.Settings().assetsDir);
        safeContentPath(project.Settings().scenesDir);
        for (const auto& path : scenes) {
            safeContentPath(path.lexically_relative(root));
            Require(Inside(path, root), "Scene escapes project root: " + path.string());
            Scene scene; std::string detail;
            const bool loaded = LoadSceneFromFile(path.string(), scene, detail);
            Require(loaded, "Invalid scene " + path.string() + ": " + detail);
            References(scene, assets, project.Settings().classification,project.Settings().navigation);
        }
        for (const auto& [id, record] : assets.Records()) {
            (void)id; std::string detail;
            safeContentPath(record.relativePath);
            Require(Inside(fs::canonical(record.path), root), "Asset escapes project root: " + record.relativePath);
            const bool valid = AssetDatabase::ValidateAssetFile(record.path, record.type, detail);
            Require(valid, "Invalid asset " + record.relativePath + ": " + detail);
            if(record.type==AssetType::Material){MaterialDefinition m;Require(LoadMaterial(record.path,m,detail),detail);for(auto& map:m.maps)if(!map.asset.empty()){auto* texture=assets.Find(map.asset);Require(texture&&!texture->missing&&texture->type==AssetType::Texture,"Missing material texture "+map.asset);}}
            if(record.type==AssetType::UI){UIDocument d;Require(LoadUIDocument(record.path,d,detail)&&ValidateUIAssets(d,assets,detail),"UI dependency: "+detail);}
            if (record.type == AssetType::Prefab) {
                Scene prefab; Require(LoadSceneFromFile(record.path, prefab, detail), detail);
                References(prefab, assets, project.Settings().classification,project.Settings().navigation);
            }
        }
        fs::create_directories(destination.parent_path());
        if (fs::exists(destination)) {
            GamePackage old; std::string detail;
            Require(fs::is_directory(destination) && ReadGamePackage(destination.string(), old, detail),
                    "Refusing to replace a directory that is not a Judas package: " + destination.string());
        }
        // Unique sibling directory, no deletion of guessed temporary names.
        std::string pattern = (destination.parent_path() / ".judas-export-XXXXXX").string();
        Require(mkdtemp(pattern.data()) != nullptr, "Cannot create export staging directory");
        staging = pattern;
        Copy(executable, staging / "judas");
        fs::permissions(staging / "judas", fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);
        fs::create_directories(staging / project.Settings().assetsDir);
        fs::create_directories(staging / project.Settings().scenesDir);
        for (const auto& path : scenes) Copy(path, staging / path.lexically_relative(root));
        for (const auto& [id, record] : assets.Records()) {
            const auto relative = fs::path(record.relativePath);
            Require(!relative.is_absolute() && Inside((root / relative).lexically_normal(), root), "Unsafe asset path");
            Copy(record.path, staging / relative);
            std::string detail;
            Require(AssetDatabase::WriteMeta((staging / relative).string() + kAssetMetaExtension,
                    id, record.type, relative.generic_string(), detail), detail);
        }
        const auto write = [&](const fs::path& path, const std::string& text) {
            std::ofstream file(staging / path, std::ios::binary); file << text;
            Require(static_cast<bool>(file), "Cannot write " + path.string());
        };
        // Preserve co-located asset redistribution notices without copying
        // arbitrary development files. Only ancestors of registered assets.
        std::set<fs::path> notices;
        for (const auto& [id, record] : assets.Records()) {
            (void)id;
            for (auto directory = fs::path(record.path).parent_path(); Inside(directory, root); directory = directory.parent_path())
                for (const char* name : {"LICENSE", "LICENSE.txt", "LICENSE.md", "COPYING"})
                    if (fs::is_regular_file(directory / name)) notices.insert(directory / name);
        }
        for (const auto& notice : notices) {
            const auto relative = notice.lexically_relative(root); safeContentPath(relative);
            Copy(notice, staging / relative);
        }
        write("game.judasproj", Project::SerializeToString(project.Settings()));
        // Stable across moves/re-exports/settings edits. Equal project names AND
        // source project filenames share a save namespace; strict scene digest
        // still rejects incompatible authored baselines. Not an absolute path.
        const auto saveId = SceneFingerprintSha256(project.Settings().name + "\n" + fs::path(project.ProjectFile()).filename().string());
        write(kGamePackageMarker, "JudasPackage 1\nproject \"game.judasproj\"\nsave-id \"" + saveId + "\"\n");
        Copy(engineRoot / "assets/fonts/DejaVuSans.ttf", staging / "engine/assets/fonts/DejaVuSans.ttf");
        Copy(engineRoot / "assets/fonts/DejaVuSans-LICENSE.txt", staging / "engine/licenses/DejaVuSans.txt");
        Copy(engineRoot / "third_party/RUNTIME_NOTICES.txt", staging / "engine/third_party/RUNTIME_NOTICES.txt");
        Copy(engineRoot / "LICENSE", staging / "engine/licenses/Judas.txt");
        write("RUNTIME_REQUIREMENTS.txt", "Judas Linux desktop Release package. Run ./judas (no arguments).\nRequires compatible glibc/libstdc++, SDL2 and its system dependencies, OpenGL 3.3 drivers.\nNo editor or development tree is required. Libraries are system provided; not bundled.\nAll registered project assets and project scenes are included for dynamic ID loading.\nSaves: $XDG_DATA_HOME/judas/games/<save-id>/Saves, otherwise $HOME/.local/share/...\nThird-party notices: engine/third_party/RUNTIME_NOTICES.txt and engine/licenses.\n");
        ProjectExportResult completed; completed.packageDirectory = destination.string();
        completed.assetCount = assets.Records().size(); completed.sceneCount = scenes.size();
        for (const auto& entry : fs::recursive_directory_iterator(staging))
            if (entry.is_regular_file()) completed.bytes += entry.file_size();
        if (fs::exists(destination)) {
            backup = staging.string() + "-previous";
            fs::rename(destination, backup);
        }
        try { fs::rename(staging, destination); }
        catch (...) { if (!backup.empty()) fs::rename(backup, destination); throw; }
        staging.clear();
        // A cleanup error does not undo a successfully promoted complete game.
        std::error_code cleanup;
        if (!backup.empty()) fs::remove_all(backup, cleanup);
        completed.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        result = completed;
        error.clear(); return true;
    } catch (const std::exception& exception) {
        std::error_code cleanup;
        if (!staging.empty()) fs::remove_all(staging, cleanup);
        error = "Export failed: " + std::string(exception.what()); return false;
    }
}
