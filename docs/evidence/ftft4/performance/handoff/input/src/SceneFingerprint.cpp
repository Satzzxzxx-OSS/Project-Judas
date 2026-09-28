#include "SceneFingerprint.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <set>
#include <string>

#include "Scene.h"
#include "SceneSerialization.h"

namespace {

constexpr std::array<std::uint32_t, 64> kSha256Constants{{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
}};

std::uint32_t RotateRight(std::uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32 - bits));
}

class CanonicalWriter {
public:
    void Context(const std::string& context) { m_context = context; }
    void Fail(const std::string& message) {
        if (m_error.empty()) m_error = "authored scene fingerprint: " + m_context + ": " + message;
    }
    void U32(std::uint32_t value) {
        for (int shift = 24; shift >= 0; shift -= 8) m_bytes.push_back(static_cast<char>((value >> shift) & 0xffu));
    }
    void U64(std::uint64_t value) {
        for (int shift = 56; shift >= 0; shift -= 8) m_bytes.push_back(static_cast<char>((value >> shift) & 0xffu));
    }
    void Integer(int value) {
        static_assert(sizeof(int) == 4, "canonical authored integers require 32 bits");
        U32(static_cast<std::uint32_t>(value));
    }
    void Boolean(bool value) { m_bytes.push_back(value ? '\1' : '\0'); }
    void Text(const std::string& value) { U64(value.size()); m_bytes.append(value); }
    void Number(float value) {
        static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                      "fingerprints require IEEE-754 binary32");
        if (!std::isfinite(value)) Fail("non-finite number");
        if (value == 0.0f) value = 0.0f;
        std::uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        U32(bits);
    }
    void Number(double value) {
        static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
                      "fingerprints require IEEE-754 binary64");
        if (!std::isfinite(value)) Fail("non-finite number");
        if (value == 0.0) value = 0.0;
        std::uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        U64(bits);
    }
    void Vector(const glm::vec3& value) { Number(value.x); Number(value.y); Number(value.z); }
    void Vector(const glm::dvec3& value) { Number(value.x); Number(value.y); Number(value.z); }
    void Quaternion(const glm::quat& value) {
        Number(value.w); Number(value.x); Number(value.y); Number(value.z);
    }
    template <typename T> void Enum(T value, int last, const char* name) {
        const int tag = static_cast<int>(value);
        if (tag < 0 || tag > last) Fail(std::string("invalid ") + name);
        U32(static_cast<std::uint32_t>(tag));
    }
    const std::string& Bytes() const { return m_bytes; }
    const std::string& Error() const { return m_error; }

private:
    std::string m_bytes;
    std::string m_context;
    std::string m_error;
};

void WriteObject(CanonicalWriter& w, const SceneObject& o) {
    const std::string object = "object " + std::to_string(o.id);
    w.Context(object + " transform");
    w.U64(o.id); w.Text(o.name);
    w.Vector(o.transform.position); w.Quaternion(o.transform.rotation); w.Vector(o.transform.scale);
    w.Boolean(o.render.has_value());
    if (o.render) {
        w.Context(object + " render");
        const auto& r = *o.render;
        w.Enum(r.shape, 4, "render shape"); w.Vector(r.halfExtents); w.Number(r.radius);
        w.Vector(r.color); w.Number(r.alpha); w.Vector(r.secondaryColor); w.Number(r.secondaryAlpha);
        w.Text(r.meshAsset); w.Text(r.textureAsset);
    }
    w.Boolean(o.body.has_value());
    if (o.body) {
        w.Context(object + " body");
        const auto& b = *o.body;
        w.Enum(b.motion, 1, "body motion"); w.Enum(b.shape, 4, "body shape");
        w.Vector(b.halfExtents); w.Number(b.radius); w.Text(b.terrainSurface);
        w.Number(b.mass); w.Number(b.friction); w.Number(b.restitution); w.Vector(b.initialLinearVelocity);
        w.Boolean(b.pickable); w.Boolean(b.managed); w.U64(b.compoundBoxes.size());
        for (const auto& box : b.compoundBoxes) { w.Vector(box.localCenter); w.Vector(box.halfExtents); }
    }
    w.Boolean(o.gravity.has_value());
    if (o.gravity) {
        w.Context(object + " gravity");
        const auto& g = *o.gravity;
        w.Enum(g.kind, 1, "gravity kind"); w.Number(g.magnitude); w.Enum(g.regionShape, 1, "gravity region");
        if (g.regionShape == SceneRegionShape::Sphere) w.Number(g.regionRadius);
        else w.Vector(g.regionHalfExtents);
    }
    w.Boolean(o.light.has_value());
    if (o.light) {
        w.Context(object + " light");
        const auto& l = *o.light;
        w.Enum(l.kind, 1, "light kind"); w.Vector(l.color); w.Number(l.range);
        w.Number(l.innerConeDegrees); w.Number(l.outerConeDegrees);
    }
    w.Boolean(o.door.has_value());
    if (o.door) {
        w.Context(object + " door");
        const auto& d = *o.door;
        w.Vector(d.localHingeAxis); w.Number(d.openAngleDegrees); w.Number(d.angularSpeedDegreesPerSecond);
    }
    w.Boolean(o.lightSwitch.has_value());
    if (o.lightSwitch) {
        w.Context(object + " light switch");
        const auto& s = *o.lightSwitch;
        w.Vector(s.localHingeAxis); w.Number(s.toggleAngleDegrees); w.Number(s.angularSpeedDegreesPerSecond);
        w.Vector(s.lampLocalOffset); w.Vector(s.lampColor); w.Number(s.lampRange);
    }
    w.Boolean(o.vehicle.has_value());
    if (o.vehicle) {
        w.Context(object + " vehicle");
        const auto& v = *o.vehicle;
        w.Enum(v.gravity, 1, "vehicle gravity"); w.Boolean(v.headlight); w.Boolean(v.navigationLights);
        w.Number(v.dragCoefficient); w.Boolean(v.initialPilotAttached);
    }
    w.Boolean(o.celestial.has_value());
    if (o.celestial) {
        w.Context(object + " celestial");
        w.Number(o.celestial->gravitationalParameter); w.Number(o.celestial->operatorThrustForce);
    }
    w.Boolean(o.atmosphere.has_value());
    if (o.atmosphere) {
        w.Context(object + " atmosphere");
        const auto& a = *o.atmosphere;
        w.Number(a.referenceRadius); w.Number(a.topRadius); w.Number(a.referenceDensity);
        w.Number(a.polytropicExponent); w.Number(a.oxidizerMassFraction); w.Number(a.referenceTemperatureKelvin);
    }
    w.Boolean(o.combustible.has_value());
    if (o.combustible) {
        w.Context(object + " combustible");
        const auto& c = *o.combustible;
        w.Number(c.heatCapacityJPerK); w.Number(c.initialFuelMassKg); w.Number(c.ignitionTemperatureK);
        w.Number(c.maximumFuelRateKgPerSecond); w.Number(c.radiativeAreaSquareMeters);
        w.Number(c.retainedCombustionHeatFraction);
    }
    w.Boolean(o.fluidVolume.has_value());
    if (o.fluidVolume) {
        w.Context(object + " fluid volume");
        const auto& f = *o.fluidVolume;
        w.Number(f.spacing); w.Integer(f.countX); w.Integer(f.countY); w.Integer(f.countZ);
        w.Boolean(f.emitter); w.Vector(f.emitterLocalOffset); w.Integer(f.maxParticles);
    }
    w.Boolean(o.playerStart.has_value());
    if (o.playerStart) {
        w.Context(object + " player start");
        w.Enum(o.playerStart->view, 1, "player view"); w.Number(o.playerStart->yawDegrees);
    }
}

}  // namespace

std::string SceneFingerprintSha256(std::string_view bytes) {
    // FIPS 180-4 SHA-256: 512-bit blocks and a 64-bit big-endian bit length.
    std::string message(bytes);
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8u;
    message.push_back(static_cast<char>(0x80));
    while (message.size() % 64 != 56) message.push_back('\0');
    for (int shift = 56; shift >= 0; shift -= 8) message.push_back(static_cast<char>((bitLength >> shift) & 0xffu));
    std::array<std::uint32_t, 8> hash{{
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    }};
    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            for (std::size_t byte = 0; byte < 4; ++byte)
                words[i] = (words[i] << 8) | static_cast<unsigned char>(message[offset + 4 * i + byte]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = RotateRight(words[i - 15], 7) ^ RotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const auto s1 = RotateRight(words[i - 2], 17) ^ RotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        auto a = hash[0], b = hash[1], c = hash[2], d = hash[3];
        auto e = hash[4], f = hash[5], g = hash[6], h = hash[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto s1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const auto choice = (e & f) ^ (~e & g);
            const auto t1 = h + s1 + choice + kSha256Constants[i] + words[i];
            const auto s0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + majority;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d;
        hash[4] += e; hash[5] += f; hash[6] += g; hash[7] += h;
    }
    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (const auto word : hash)
        for (int shift = 28; shift >= 0; shift -= 4) result.push_back(hex[(word >> shift) & 0xfu]);
    return result;
}

bool ComputeSceneFingerprint(const Scene& scene, std::string& outFingerprint,
                             std::string& outError) {
    outError.clear();
    CanonicalWriter w;
    w.Text("Judas.AuthoredSceneFingerprint");
    w.U32(kSceneFingerprintVersion);
    w.U32(kSceneFormatVersion);
    w.Context("settings");
    const auto& s = scene.Settings();
    w.Text(s.name); w.Vector(s.worldOrigin); w.Vector(s.sunDirection);
    w.Vector(s.sunColor); w.Vector(s.ambientColor); w.Number(s.fluidScale);
    w.Enum(s.fidelityPolicy, 1, "fidelity policy");
    if (s.fidelityPolicy == SceneFidelityPolicy::Distance) {
        w.Number(s.fidelityFullRadius); w.Number(s.fidelityCoarseRadius);
    }
    w.U64(scene.NextId()); w.U64(scene.Objects().size());
    if (scene.NextId() == 0) w.Fail("NextId must be positive");
    std::set<SceneObjectId> ids;
    for (const auto& o : scene.Objects()) {
        w.Context("object " + std::to_string(o.id));
        if (o.id == 0 || !ids.insert(o.id).second) w.Fail("invalid or duplicate stable ID");
        if (o.id >= scene.NextId()) w.Fail("stable ID must precede NextId");
        WriteObject(w, o);
    }
    if (!w.Error().empty()) { outError = w.Error(); return false; }
    outFingerprint = SceneFingerprintSha256(w.Bytes());
    return true;
}
