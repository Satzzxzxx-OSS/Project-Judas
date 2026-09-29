#include "AssetDatabase.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
namespace {
constexpr const char* kIdA = "11111111111111111111111111111111";
constexpr const char* kIdB = "22222222222222222222222222222222";
constexpr const char* kMesh = "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
int checks = 0, failures = 0, cases = 0;
void Check(bool condition, const std::string& description) {
    ++checks;
    if (!condition) ++failures;
    std::cout << (condition ? "PASS " : "FAIL ") << description << '\n';
}
void Write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << bytes;
    if (!out) throw std::runtime_error("fixture write failed: " + path.string());
}
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("fixture read failed: " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::map<std::string, std::string> Files(const fs::path& root) {
    std::map<std::string, std::string> result;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) result.emplace(entry.path().lexically_relative(root).generic_string(), Read(entry.path()));
    }
    return result;
}
std::string Records(const AssetDatabase& db) {
    std::ostringstream out;
    for (const auto& [id, r] : db.Records())
        out << id << '\n' << static_cast<int>(r.type) << '\n' << r.path << '\n' << r.relativePath << '\n'
            << r.source << '\n' << r.missing << '\n';
    for (const auto& p : db.Untracked()) out << "untracked " << p << '\n';
    for (const auto& p : db.Problems()) out << "problem " << p.path << ' ' << p.message << '\n';
    return out.str();
}
struct Fixture {
    fs::path root, assets, source;
    AssetDatabase db;
    explicit Fixture(const fs::path& base, const std::string& name)
        : root(base / name), assets(root / "Assets"), source(root / "external" / "source.obj") {
        ++cases;
        fs::create_directories(assets);
        Write(source, kMesh);
        Scan();
    }
    void Scan() { db.Scan(root.string(), assets.string()); }
    AssetRecord Tracked() {
        const fs::path path = assets / "original.obj";
        Write(path, kMesh);
        AssetRecord r;
        std::string error;
        if (!db.Track(path.string(), r, error, kIdA)) throw std::runtime_error("fixture Track failed: " + error);
        return r;
    }
    void Sidecar(const fs::path& asset, bool valid) {
        if (valid) {
            std::string error;
            if (!AssetDatabase::WriteMeta(asset.string() + kAssetMetaExtension, kIdB, AssetType::Mesh, "retained provenance", error))
                throw std::runtime_error(error);
        } else Write(asset.string() + kAssetMetaExtension, "JudasAssetMeta 1\nid broken\n# preserve these bytes\n");
    }
    template<class Operation> void Refusal(const std::string& label, Operation operation) {
        const auto beforeFiles = Files(root);
        const std::string beforeRecords = Records(db);
        std::string error;
        const bool accepted = operation(error);
        std::cout << "OBSERVATION " << label << " accepted=" << accepted << " error=" << error << '\n';
        Check(!accepted, label + ": request refused");
        Check(!error.empty(), label + ": useful refusal error");
        Check(Files(root) == beforeFiles, label + ": every existing file/sidecar byte preserved and no new files");
        Check(Records(db) == beforeRecords, label + ": database identity and diagnostics unchanged");
    }
};
void Run(const fs::path& base) {
    for (bool absolute : {false, true}) {
        Fixture f(base, absolute ? "import_absolute_escape" : "import_relative_escape");
        const std::string target = absolute ? (f.root / "Assets_backup" / "escaped.obj").string() : "../Assets_backup/escaped.obj";
        AssetRecord output;
        f.Refusal(absolute ? "Import absolute sibling-prefix escape" : "Import normalized sibling-prefix escape",
                  [&](std::string& error) { return f.db.Import(f.source.string(), target, output, error); });
    }
    for (bool absolute : {false, true}) {
        Fixture f(base, absolute ? "move_absolute_escape" : "move_relative_escape");
        const AssetRecord original = f.Tracked();
        const std::string target = absolute ? (f.root / "Assets_backup" / "escaped.obj").string() : "../Assets_backup/escaped.obj";
        f.Refusal(absolute ? "Move absolute sibling-prefix escape" : "Move normalized sibling-prefix escape",
                  [&](std::string& error) { return f.db.Move(original.id, target, error); });
    }
    for (bool valid : {false, true}) {
        Fixture f(base, valid ? "import_valid_orphan" : "import_corrupt_orphan");
        f.Sidecar(f.assets / "orphan.obj", valid);
        f.Scan();
        AssetRecord output;
        f.Refusal(valid ? "Import over missing-asset identity" : "Import over corrupt orphan sidecar",
                  [&](std::string& error) { return f.db.Import(f.source.string(), "orphan.obj", output, error); });
    }
    for (bool valid : {false, true}) {
        Fixture f(base, valid ? "move_valid_orphan" : "move_corrupt_orphan");
        const AssetRecord original = f.Tracked();
        f.Sidecar(f.assets / "orphan.obj", valid);
        f.Scan();
        f.Refusal(valid ? "Move over missing-asset identity" : "Move over corrupt orphan sidecar",
                  [&](std::string& error) { return f.db.Move(original.id, "orphan.obj", error); });
    }
    {
        Fixture f(base, "track_existing_identity");
        const AssetRecord original = f.Tracked();
        AssetRecord output;
        f.Refusal("Track existing identity with fresh forced ID",
                  [&](std::string& error) { return f.db.Track(original.path, output, error, kIdB); });
    }
    {
        Fixture f(base, "track_corrupt_sidecar");
        const fs::path asset = f.assets / "broken.obj";
        Write(asset, kMesh);
        f.Sidecar(asset, false);
        f.Scan();
        AssetRecord output;
        f.Refusal("Track existing corrupt sidecar",
                  [&](std::string& error) { return f.db.Track(asset.string(), output, error, kIdA); });
    }
    {
        Fixture f(base, "track_outside");
        const fs::path outside = f.root / "Assets_backup" / "external.obj";
        Write(outside, kMesh);
        AssetRecord output;
        f.Refusal("Track sibling directory asset",
                  [&](std::string& error) { return f.db.Track(outside.string(), output, error, kIdA); });
    }
    {
        Fixture f(base, "track_dotdot_filename");
        const fs::path asset = f.assets / "..legitimate.obj";
        Write(asset, kMesh);
        AssetRecord output;
        std::string error;
        const bool ok = f.db.Track(asset.string(), output, error, kIdA);
        Check(ok, "Track accepts a file component beginning with two dots inside assets");
        Check(ok && output.path == asset.string() && output.id == kIdA, "Track records exact inside file identity");
    }
    {
        Fixture f(base, "normalized_inside_paths");
        AssetRecord imported;
        std::string error;
        const bool importedOk = f.db.Import(f.source.string(), "unused/../imported.obj", imported, error);
        Check(importedOk, "Import normalized path within assets");
        if (importedOk) {
            const auto oldMeta = Read(imported.path + kAssetMetaExtension);
            const bool moved = f.db.Move(imported.id, "models/../models/renamed.obj", error);
            Check(moved, "Move normalized path within assets");
            if (moved) {
                const auto* current = f.db.Find(imported.id);
                Check(current && current->relativePath == "Assets/models/renamed.obj", "Move preserves referenced ID and resolves new path");
                Check(current && Read(current->path + kAssetMetaExtension) == oldMeta, "Move keeps metadata byte-for-byte");
                f.Scan();
                current = f.db.Find(imported.id);
                Check(current && !current->missing && Read(current->path) == kMesh, "Rescan resolves same ID to moved decoded asset");
            }
        }
    }
    {
        Fixture f(base, "duplicate_metadata");
        const AssetRecord original = f.Tracked();
        const fs::path duplicate = f.assets / "z_duplicate.obj";
        Write(duplicate, kMesh);
        Write(duplicate.string() + kAssetMetaExtension, Read(original.path + kAssetMetaExtension));
        const auto before = Files(f.root);
        f.Scan();
        Check(f.db.Records().size() == 1 && f.db.Find(original.id) && f.db.Find(original.id)->path == original.path,
              "Duplicate metadata keeps deterministic first holder");
        bool reported = false;
        for (const auto& problem : f.db.Problems()) if (problem.message.find("duplicate asset id") != std::string::npos) reported = true;
        Check(reported, "Duplicate metadata reported explicitly");
        Check(Files(f.root) == before, "Duplicate scan preserves every file byte");
    }
}
} // namespace

int main() {
    const fs::path base = fs::temp_directory_path() / ("judas_asset_integrity_" +
        std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    try {
        Run(base);
    } catch (const std::exception& error) {
        Check(false, std::string("unexpected exception: ") + error.what());
    }
    std::error_code error;
    fs::remove_all(base, error);
    Check(!error, "temporary fixtures removed");
    std::cout << "Asset integrity: " << cases << " cases, " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
