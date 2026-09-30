// FTFT9: authored hybrid-fluid parameters, independent of fluid dynamics.
#include <cstdio>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Scene.h"
#include "SceneFingerprint.h"
#include "SceneSerialization.h"

namespace {
int checks = 0, failures = 0;
void Check(bool ok, const std::string& label) {
    ++checks;
    if (!ok) ++failures;
    std::printf("FTFT9 METADATA %s %s\n", ok ? "PASS" : "FAIL", label.c_str());
}
std::string Fingerprint(const Scene& scene) {
    std::string fingerprint, error;
    Check(ComputeSceneFingerprint(scene, fingerprint, error), "fingerprint valid authored scene: " + error);
    return fingerprint;
}
Scene Fixture(bool nondefault) {
    Scene s;
    s.Settings().name = "Generic cavity metadata";
    auto& o = s.CreateObject("Arbitrary body and player");
    o.body = SceneBodyComponent{};
    o.body->motion = SceneBodyMotion::Dynamic;
    o.playerStart = ScenePlayerStartComponent{};
    if (nondefault) {
        o.body->shape = SceneShape::Compound;
        o.body->compoundBoxes.push_back({glm::vec3(0), glm::vec3(1, .1f, 1)});
        o.body->fluidCavities.push_back({glm::vec3(.2f, .5f, -.3f), glm::vec3(.7f, .4f, .6f)});
        o.body->fluidCavities.push_back({glm::vec3(-1, 2, 3), glm::vec3(.1f, .2f, .3f)});
        o.playerStart->density = 1025;
        o.playerStart->fluidDrag = 3;
        o.playerStart->swimAcceleration = 7;
        s.Settings().fluidUpdateRateHz = 24;
        s.Settings().fluidHydrostaticDragRate = .5f;
    }
    return s;
}
std::string Replace(const std::string& text, const std::string& old, const std::string& replacement) {
    const auto at = text.find(old);
    Check(at != std::string::npos, "hostile mutation located: " + old);
    if (at == std::string::npos) return text;
    return text.substr(0, at) + replacement + text.substr(at + old.size());
}
std::string ReplaceLine(const std::string& text, const std::string& prefix, const std::string& replacement) {
    std::istringstream input(text); std::string result, line;
    bool found = false;
    while (std::getline(input, line)) {
        if (!found && line.find(prefix) != std::string::npos) { result += replacement + "\n"; found = true; }
        else result += line + "\n";
    }
    Check(found, "hostile line located: " + prefix);
    return result;
}
void RejectText(const std::string& text, const std::string& label) {
    Scene output = Fixture(true), before = output;
    std::string error;
    Check(!LoadSceneFromString(text, output, error) && !error.empty(), label + " rejected with error");
    Check(ScenesEqual(output, before), label + " load leaves destination unchanged");
}
void InvalidCanonical(const Scene& source, const std::string& label,
                      const std::function<void(Scene&)>& mutate) {
    Scene bad = source; mutate(bad);
    std::string digest = "unchanged", error;
    Check(!ComputeSceneFingerprint(bad, digest, error) && digest == "unchanged" && !error.empty(),
          label + " in-memory baseline rejected without digest mutation");
}
}
int main() {
    Check(kSceneFingerprintVersion == 2 && kSceneFormatVersion == 3,
          "explicit canonical schema 2; authored scene grammar stays version 3");
    const Scene authored = Fixture(true);
    std::string text, error;
    Check(SaveSceneToString(authored, text), "serialize cavity and fluid parameters");
    Scene loaded;
    Check(LoadSceneFromString(text, loaded, error), "load cavity and fluid parameters: " + error);
    Check(ScenesEqual(authored, loaded), "all authored parameters round-trip exactly");
    Check(Fingerprint(authored) == Fingerprint(loaded), "round-trip baseline compatibility unchanged");
    std::string repeated;
    SaveSceneToString(loaded, repeated);
    Check(text == repeated, "second serialization is byte-identical");

    Scene defaults = Fixture(false);
    std::string withDefaults;
    SaveSceneToString(defaults, withDefaults);
    const std::vector<std::string> newKeys = {"fluid-update-rate-hz", "fluid-hydrostatic-drag-rate",
        "body.fluid-cavity-count", "player-start.density", "player-start.fluid-drag", "player-start.swim-acceleration"};
    std::istringstream input(withDefaults); std::string oldText, line;
    while (std::getline(input, line)) {
        bool newField = false;
        for (const auto& key : newKeys) newField |= line.find(key) != std::string::npos;
        if (!newField) oldText += line + "\n";
    }
    Scene legacyScene;
    Check(LoadSceneFromString(oldText, legacyScene, error), "older authored scene remains readable");
    Check(ScenesEqual(defaults, legacyScene), "omitted authored fields use documented defaults");
    Check(Fingerprint(defaults) == Fingerprint(legacyScene), "omitted/explicit defaults have same schema-2 identity");
    for (const auto& key : newKeys) Check(withDefaults.find(key) != std::string::npos, "writer emits explicit default: " + key);

    for (const char* value : {"0 0 0 0 .2 .3", "0 0 0 -.1 .2 .3", "nan 0 0 .1 .2 .3",
                              "0 0 0 .1 inf .3", "0 0 0 .1 .2", "0 0 0 .1 .2 .3 4"}) {
        RejectText(ReplaceLine(text, "body.fluid-cavity ", std::string("  body.fluid-cavity ") + value),
                   std::string("invalid cavity ") + value);
    }
    RejectText(Replace(text, "body.fluid-cavity-count 2", "body.fluid-cavity-count 1"), "cavity count mismatch");
    RejectText(Replace(text, "body.fluid-cavity-count 2", "body.fluid-cavity-count -1"), "negative cavity count");
    RejectText(Replace(withDefaults, "settings\n", "settings\n  body.fluid-cavity 0 0 0 .1 .2 .3\n"),
               "cavity line misplaced in settings");
    RejectText(Replace(withDefaults, "body dynamic box\n", ""), "cavity count requires a body header");
    for (const auto& [key, value] : std::vector<std::pair<std::string, std::string>>{
            {"fluid-update-rate-hz", "0"}, {"fluid-update-rate-hz", "nan"},
            {"fluid-hydrostatic-drag-rate", "-1"}, {"fluid-hydrostatic-drag-rate", "inf"},
            {"player-start.density", "0"}, {"player-start.density", "-1"},
            {"player-start.density", "nan"}, {"player-start.fluid-drag", "-1"},
            {"player-start.swim-acceleration", "-1"}, {"player-start.swim-acceleration", "inf"}}) {
        RejectText(ReplaceLine(text, key + " ", "  " + key + " " + value), "invalid " + key + "=" + value);
    }
    auto changed = authored;
    changed.Objects()[0].body->fluidCavities[0].localCenter.x += .1f;
    Check(!ScenesEqual(authored, changed) && Fingerprint(authored) != Fingerprint(changed), "cavity-center edit changes identity/equality");
    changed = authored;
    std::swap(changed.Objects()[0].body->fluidCavities[0], changed.Objects()[0].body->fluidCavities[1]);
    Check(!ScenesEqual(authored, changed) && Fingerprint(authored) != Fingerprint(changed), "cavity ordering is strict authored identity");
    InvalidCanonical(authored, "negative cavity extent", [](Scene& s) { s.Objects()[0].body->fluidCavities[0].halfExtents.y = -1; });
    InvalidCanonical(authored, "infinite cavity center", [](Scene& s) {
        s.Objects()[0].body->fluidCavities[0].localCenter.x = std::numeric_limits<float>::infinity();
    });
    InvalidCanonical(authored, "zero player density", [](Scene& s) { s.Objects()[0].playerStart->density = 0; });
    InvalidCanonical(authored, "negative player drag", [](Scene& s) { s.Objects()[0].playerStart->fluidDrag = -1; });
    InvalidCanonical(authored, "negative swim acceleration", [](Scene& s) { s.Objects()[0].playerStart->swimAcceleration = -1; });
    InvalidCanonical(authored, "zero fluid rate", [](Scene& s) { s.Settings().fluidUpdateRateHz = 0; });
    InvalidCanonical(authored, "negative hydrostatic drag", [](Scene& s) { s.Settings().fluidHydrostaticDragRate = -1; });
    changed = authored;
    changed.Objects()[0].playerStart->fluidDrag = 0;
    changed.Objects()[0].playerStart->swimAcceleration = 0;
    changed.Settings().fluidHydrostaticDragRate = 0;
    Check(!Fingerprint(changed).empty(), "zero drag/propulsion is a valid configured choice");
    Scene classic;
    Check(LoadSceneFromFile("assets/scenes/classic.judas", classic, error), "shipped classic scene loads");
    for (SceneObjectId id : {23, 24}) {
        const auto* o = classic.Find(id);
        Check(o && o->body && o->body->fluidCavities.size() == 1, "shipped resolved cup has explicit cavity");
    }
    std::printf("FTFT9 METADATA SUMMARY checks=%d failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
