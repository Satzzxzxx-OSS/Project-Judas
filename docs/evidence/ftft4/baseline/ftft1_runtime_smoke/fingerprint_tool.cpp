// Test fixture generator only: obtains the same authored baseline identity
// as the application without supplying any persistence implementation.
#include <cstdio>
#include <string>
#include "Scene.h"
#include "SceneSerialization.h"
#include "SceneFingerprint.h"
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    Scene scene;
    std::string hash, error;
    if (!LoadSceneFromFile(argv[1], scene, error) || !ComputeSceneFingerprint(scene, hash, error)) {
        std::fprintf(stderr, "%s\n", error.c_str()); return 1;
    }
    std::printf("%s\n", hash.c_str());
    return 0;
}
