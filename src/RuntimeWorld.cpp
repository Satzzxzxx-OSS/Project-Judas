#include "RuntimeWorld.h"
#include <fstream>
#include <sstream>
#include "Prefab.h"
#include <set>
#include <sstream>
#include "SceneSerialization.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/gtc/quaternion.hpp>

#include "BoxVolume.h"
#include "FaithfulGravity.h"
#include "RadialTerrain.h"
#include "ProductionFluidCoupling.h"
#include "RadicalGravity.h"
#include "ResourceManager.h"
#include "SceneFingerprint.h"
#include "SphericalVolume.h"
#include "TerrainLibrary.h"
#include "UniformGravity.h"

namespace {
constexpr float kFaithfulMagnitude = 9.81f;
// Below these an entity leaving Full simulation is treated as resting on
// whatever supported it (CoarseMotion::Settled) rather than in free flight.
constexpr float kSettledLinearSpeed = 0.05f;
constexpr float kSettledAngularSpeed = 0.05f;

EntityPhysicalState StateFromDefinition(const SceneObject& o) {
    EntityPhysicalState state;
    state.position = o.transform.position;
    state.rotation = glm::normalize(o.transform.rotation);
    if (o.body) state.linearVelocity = o.body->initialLinearVelocity;
    return state;
}
}  // namespace

RuntimeWorld::RuntimeWorld() = default;

RuntimeWorld::~RuntimeWorld() {
    Destroy();
}

bool RuntimeWorld::EntityRequiresFull(const SceneObject& o) {
    if (o.vehicle || o.combustible || !o.scripts.empty()) return true;
    if (o.body && o.body->shape == SceneShape::Compound) return true;
    return false;
}

bool RuntimeWorld::ValidateVisualAssets(const SceneObject& o, std::string& error) const {
    if (!o.render || o.render->shape != SceneShape::Mesh || !m_assets) return true;
    const AssetDatabase* db = m_assets->Assets();
    const auto check = [&](const AssetId& id, AssetType type) {
        if (id.empty()) return true;
        const AssetRecord* record = db ? db->Find(id) : nullptr;
        if (!record) {
            error = "unknown asset id " + id;
            return false;
        }
        if (record->type != type) {
            error = "asset " + record->relativePath + " is a " + AssetTypeName(record->type) + ", not a " + AssetTypeName(type);
            return false;
        }
        return true;
    };
    if (o.render->meshAsset.empty()) {
        error = "mesh render has no mesh asset";
        return false;
    }
    if (!check(o.render->meshAsset, AssetType::Mesh) || !check(o.render->textureAsset, AssetType::Texture)) return false;
    return true;
}

bool RuntimeWorld::RequestVisualAssets(const SceneObject& o, std::string* outError) {
    std::string error;
    if (!ValidateVisualAssets(o, error)) { if (outError) *outError = error; return false; }
    if (!o.render || o.render->shape != SceneShape::Mesh || !m_assets) return true;
    // Demand: referenced for the life of this world; the load runs in the
    // background and presentation picks it up when Ready.
    m_assets->AddRef(o.render->meshAsset);
    m_referencedAssets.push_back(o.render->meshAsset);
    m_assets->RequestMesh(o.render->meshAsset, JobPriority::High);
    if (!o.render->textureAsset.empty()) {
        m_assets->AddRef(o.render->textureAsset);
        m_referencedAssets.push_back(o.render->textureAsset);
        m_assets->RequestTexture(o.render->textureAsset, JobPriority::High);
    }
    return true;
}

bool RuntimeWorld::InstantiateEntityBody(EntityRecord& record, const EntityPhysicalState& state,
                                         std::string* outError) {
    const SceneObject& o = record.definition;
    const SceneBodyComponent& b = *o.body;
    BodyHandle handle;
    switch (b.shape) {
        case SceneShape::Box:
            handle = m_physics.CreateDynamicBox(state.position, b.halfExtents, b.mass, b.friction, b.restitution);
            break;
        case SceneShape::Sphere:
            handle = m_physics.CreateDynamicSphere(state.position, b.radius, b.mass, b.friction, b.restitution);
            break;
        case SceneShape::Compound:
            handle = m_physics.CreateDynamicCompoundBoxes(state.position, b.compoundBoxes, b.mass, b.friction,
                                                          b.restitution);
            break;
        case SceneShape::Terrain:
        case SceneShape::Mesh:
            break;
    }
    if (!handle.IsValid()) {
        if (outError) *outError = "the body could not be created";
        return false;
    }
    // Reconstruction hands the body its retained pose AND velocities: no
    // reset to rest, no impulse.
    m_physics.SetCollisionFilter(handle,b.collisionLayer,b.collisionMask);
    m_physics.SetBodySensor(handle,b.sensor);m_physics.SetBodyEnabled(handle,b.enabled);
    m_physics.SetBodyTags(handle,TagsOf(record.id));
    m_entityCategories[record.id].body=handle;
    m_physics.ResetBody(handle, state.position, state.rotation);
    m_physics.SetLinearVelocity(handle, state.linearVelocity);
    m_physics.SetAngularVelocity(handle, state.angularVelocity);
    DynamicBody& slot = m_dynamicBodies[record.slot];
    slot.Rebind(handle);
    slot.SetPoseFromState(state.position, state.rotation);
    slot.SnapPresentation();
    return true;
}

void RuntimeWorld::ReleaseEntityBody(EntityRecord& record) {
    DynamicBody& slot = m_dynamicBodies[record.slot];
    if (slot.IsLive()) {
        m_physics.DestroyBody(slot.Handle());
        slot.Rebind(BodyHandle{});
    }
}

bool RuntimeWorld::AppendEntitySlot(const SceneObject& o, bool authored, const EntityPhysicalState& state,
                                    SimulationFidelity fidelity, std::string* outError) {
    const SceneBodyComponent& b = *o.body;
    DynamicBody::Visual visual;
    visual.shape = b.shape == SceneShape::Sphere ? DynamicBody::Shape::Sphere : DynamicBody::Shape::Box;
    visual.halfExtents = b.halfExtents;
    visual.radius = b.radius;
    if (o.render) visual.color = o.render->color;

    DynamicVisual dv;
    dv.id = o.id;
    dv.name = o.name;
    dv.hasRender = o.render.has_value();
    if (o.render) dv.render = *o.render;
    dv.compoundBoxes = b.compoundBoxes;
    dv.scale = o.transform.scale;
    dv.initialLinearVelocity = b.initialLinearVelocity;
    dv.pickable = b.pickable;
    if (!RequestVisualAssets(o, outError)) return false;

    EntityRecord record;
    record.id = o.id;
    record.name = o.name;
    record.definition = o;
    record.authored = authored;
    record.managed = b.managed;
    record.requiresFull = EntityRequiresFull(o)||m_jointParticipants.count(o.id);
    record.state = state;
    record.slot = m_dynamicBodies.size();
    if (record.requiresFull) fidelity = SimulationFidelity::Full;
    record.fidelity = fidelity;
    record.lifecycle = fidelity == SimulationFidelity::Dormant ? EntityLifecycle::Unloaded : EntityLifecycle::Active;
    if (fidelity == SimulationFidelity::Dormant) record.dormantSinceSeconds = m_simulationTime;
    record.coarseMotion = glm::length(state.linearVelocity) < kSettledLinearSpeed &&
                                  glm::length(state.angularVelocity) < kSettledAngularSpeed
                              ? CoarseMotion::Settled
                              : CoarseMotion::Inertial;

    m_dynamicBodies.emplace_back(BodyHandle{}, visual, o.transform.position, glm::normalize(o.transform.rotation));
    m_dynamicBodies.back().SetPoseFromState(state.position, state.rotation);
    m_dynamicBodies.back().SnapPresentation();
    m_dynamicVisuals.push_back(dv);
    m_entities.push_back(record);
    if (fidelity == SimulationFidelity::Full) {
        if (!InstantiateEntityBody(m_entities.back(), state, outError)) {
            m_dynamicBodies.pop_back();
            m_dynamicVisuals.pop_back();
            m_entities.pop_back();
            return false;
        }
    }
    ++m_entityVersion;
    return true;
}

bool RuntimeWorld::Build(const Scene& authored, ResourceManager* resources, std::string& outError, const ProjectClassification* categories) {
    Scene resolved, scene;
    if (!ResolvePrefabs(authored, resources ? resources->Assets() : nullptr, resolved, outError) ||
        !FlattenHierarchy(resolved, scene, outError)) return false;
    const ProjectClassification selected=categories?*categories:ProjectClassification{};
    if(!ValidateSceneClassification(scene,selected,outError))return false;
    int activeAudioListeners=0;
    for(const auto& o:scene.Objects())if(o.audioListener&&o.audioListener->enabled)++activeAudioListeners;
    if(activeAudioListeners>1){outError="more than one enabled audio listener";return false;}
    for (const auto& o : scene.Objects()) {
        if (o.renderCamera) {
            const auto& c = *o.renderCamera;
            if (c.width < 1 || c.width > 4096 || c.height < 1 || c.height > 4096 ||
                c.updateEveryFrames < 1 || !(c.verticalFovDegrees > 0 && c.verticalFovDegrees < 179) ||
                !(c.nearPlane > 0 && c.farPlane > c.nearPlane) || o.door || o.lightSwitch) {
                outError = "invalid render-camera configuration"; return false;
            }
        }
        if (o.render && o.render->textureCamera) {
            const auto* c = scene.Find(o.render->textureCamera);
            if (!c || !c->renderCamera || !o.render->textureAsset.empty() ||
                (o.render->shape != SceneShape::Box && o.render->shape != SceneShape::Sphere && o.render->shape != SceneShape::Mesh)) {
                outError = "invalid render-camera texture reference"; return false;
            }
        }
    }
    std::string fingerprint;
    if (!ComputeSceneFingerprint(scene, fingerprint, outError)) return false;
    if(std::any_of(scene.Objects().begin(),scene.Objects().end(),[](const auto& o){return !o.scripts.empty();})){
        if(!resources||!resources->Assets()){outError="scripted scene requires a project asset database";return false;}
        std::string scripts;if(!ScriptSystem::SourceFingerprint(*resources->Assets(),scene,scripts,outError,false))return false;
        fingerprint=SceneFingerprintSha256(fingerprint+scripts);
    }
    if(std::any_of(scene.Objects().begin(),scene.Objects().end(),[](const auto& o){return o.ui.has_value()||!o.scripts.empty();})&&resources&&resources->Assets()&&std::any_of(resources->Assets()->Records().begin(),resources->Assets()->Records().end(),[](const auto& p){return p.second.type==AssetType::UI;})){
        if(!resources||!resources->Assets()){outError="UI scene requires project assets";return false;}
        std::string bytes="Judas.UISources.1";
        for(const auto& p:resources->Assets()->Records())if(p.second.type==AssetType::UI){std::ifstream file(p.second.path);std::ostringstream contents;contents<<file.rdbuf();bytes+=p.first+SceneFingerprintSha256(contents.str());}
        fingerprint=SceneFingerprintSha256(fingerprint+SceneFingerprintSha256(bytes));
    }
    Destroy();
    m_categories=selected;
    m_assets = resources;
    m_audioSystem=resources?resources->GetAudioSystem():nullptr;
    m_settings = scene.Settings();
    if(std::any_of(resolved.Objects().begin(),resolved.Objects().end(),[](const auto& o){return o.parent!=0;}))m_hierarchy=resolved;
    if (!m_physics.Init()) {
        outError = "physics initialization failed";
        return false;
    }
    m_built = true;
    for(const auto& o:scene.Objects())if(o.joint){m_jointParticipants.insert(o.joint->bodyA);if(o.joint->bodyB)m_jointParticipants.insert(o.joint->bodyB);}

    if (!(m_settings.fluidScale > 0.0f)) {
        outError = "settings: fluid-scale must be positive";
        Destroy();
        return false;
    }
    m_fluidSettings.updateRateHz = m_settings.fluidUpdateRateHz;
    m_fluidSettings.hydrostaticDragRate = m_settings.fluidHydrostaticDragRate;
    m_fluidSettings.particleRadius *= m_settings.fluidScale;
    m_fluidSettings.smoothingRadius *= m_settings.fluidScale;
    m_fluidSettings.maxDensityCorrection *= m_settings.fluidScale;
    m_fluid = std::make_unique<FluidWorld>(m_fluidSettings);

    if (m_settings.fidelityPolicy == SceneFidelityPolicy::Distance) {
        m_policy = std::make_unique<DistanceFidelityPolicy>(m_settings.fidelityFullRadius,
                                                            m_settings.fidelityCoarseRadius);
    }
    // The policy's focus at load: the player start. A managed entity that
    // the policy would not simulate fully is never given a live body at
    // all — a large world does not wake everything up just to put most of
    // it back to sleep.
    FidelityPolicyContext loadContext;
    for (const SceneObject& o : scene.Objects()) {
        if (o.playerStart) loadContext.focus = o.transform.position;
    }

    if (!AppendSceneObjects(scene, true, loadContext, outError)) { Destroy(); return false; }
    SynchronizeJoints();
    for(const auto& o:scene.Objects())if(m_hasScripts&&!FindEntity(o.id)){
        std::string unsupported;
        if(o.scripts.empty()&&!ValidateEntityDefinition(o,unsupported))continue;
        EntityRecord e;e.id=o.id;e.name=o.name;e.definition=o;e.authored=true;e.requiresFull=true;
        e.state=StateFromDefinition(o);e.slot=std::numeric_limits<std::size_t>::max();m_extraEntities.push_back(e);
    }
    m_baselineFingerprint = std::move(fingerprint);
    return true;
}

bool RuntimeWorld::AppendSceneObjects(const Scene& scene, bool authored,
    const FidelityPolicyContext& loadContext, std::string& outError) {
    for(const auto& o:scene.Objects())if(o.joint){m_jointOwners.insert(o.id);m_jointParticipants.insert(o.joint->bodyA);if(o.joint->bodyB)m_jointParticipants.insert(o.joint->bodyB);}
    const auto fail = [&](const SceneObject& o, const std::string& what) {
        outError = "object " + std::to_string(o.id) + " \"" + o.name + "\": " + what;
        return false;
    };

    for (const SceneObject& o : scene.Objects()) {
        if(o.ui&&o.ui->enabled){std::string uiError;if(!UI().Load(o.ui->asset,o.ui->name,o.id,uiError))return fail(o,uiError);}
        m_hasScripts|=!o.scripts.empty();
        m_scriptDefinitions[o.id]=o;
        m_entityCategories[o.id]={o.tags,o.tags,o.renderLayer,{}};
        const glm::vec3 position = o.transform.position;
        const glm::quat rotation = glm::normalize(o.transform.rotation);

        // --- Body ---
        BodyHandle bodyHandle;
        std::size_t dynamicIndex = m_dynamicBodies.size();
        bool isDynamic = false;
        if (o.body) {
            const SceneBodyComponent& b = *o.body;
            if (b.motion == SceneBodyMotion::Static) {
                switch (b.shape) {
                    case SceneShape::Box:
                        bodyHandle = m_physics.CreateStaticBox(position, rotation, b.halfExtents, b.friction, b.restitution);
                        break;
                    case SceneShape::Sphere:
                        bodyHandle = m_physics.CreateStaticSphere(position, b.radius, b.friction, b.restitution);
                        break;
                    case SceneShape::Terrain: {
                        std::shared_ptr<const RadialTerrain> surface = CreateTerrainSurface(b.terrainSurface);
                        if (!surface) return fail(o, "unknown terrain surface '" + b.terrainSurface + "'");
                        bodyHandle = m_physics.CreateStaticTerrain(position, rotation, surface, b.friction, b.restitution);
                        Terrain terrain;
                        terrain.id = o.id;
                        terrain.handle = bodyHandle;
                        terrain.surface = surface;
                        terrain.identifier = b.terrainSurface;
                        terrain.position = position;
                        terrain.rotation = rotation;
                        if (o.render) {
                            terrain.color = o.render->color;
                            if (m_assets) terrain.mesh = m_assets->GetTerrainMesh(b.terrainSurface, *surface);
                        }
                        m_terrains.push_back(terrain);
                        break;
                    }
                    case SceneShape::Compound:
                        return fail(o, "static compound bodies are not supported");
                    case SceneShape::Mesh:
                        return fail(o, "a body cannot use the mesh shape");
                }
                if (b.shape != SceneShape::Terrain) {
                    StaticBody sb;
                    sb.id = o.id;
                    sb.handle = bodyHandle;
                    sb.shape = b.shape;
                    sb.position = position;
                    sb.rotation = rotation;
                    sb.halfExtents = b.halfExtents;
                    sb.radius = b.radius;
                    m_staticBodies.push_back(sb);
                }
            } else {
                if (b.shape == SceneShape::Terrain) return fail(o, "terrain bodies must be static");
                if (b.shape == SceneShape::Mesh) return fail(o, "a body cannot use the mesh shape");
                isDynamic = true;
                const EntityPhysicalState state = StateFromDefinition(o);
                SimulationFidelity fidelity = SimulationFidelity::Full;
                if (m_policy && b.managed && !EntityRequiresFull(o)) {
                    FidelityPolicyEntity view;
                    view.id = o.id;
                    view.current = SimulationFidelity::Dormant;  // nothing exists yet
                    view.position = state.position;
                    view.linearVelocity = state.linearVelocity;
                    fidelity = m_policy->Desired(view, loadContext);
                }
                std::string entityError;
                if (!AppendEntitySlot(o, /*authored=*/authored, state, fidelity, &entityError)) return fail(o, entityError);
                bodyHandle = m_dynamicBodies[dynamicIndex].Handle();
            }
        } else if (o.render && (o.render->shape == SceneShape::Compound || o.render->shape == SceneShape::Terrain)) {
            return fail(o, "compound/terrain rendering needs a body");
        }

        if(bodyHandle.IsValid()&&o.body){
            m_physics.SetCollisionFilter(bodyHandle,o.body->collisionLayer,o.body->collisionMask);
            m_physics.SetBodySensor(bodyHandle,o.body->sensor);m_physics.SetBodyEnabled(bodyHandle,o.body->enabled);
            m_physics.SetBodyTags(bodyHandle,o.tags);m_entityCategories[o.id].body=bodyHandle;
        }
        // --- Renderable without a dynamic body ---
        if (o.render && !isDynamic && o.render->shape != SceneShape::Terrain && !o.door && !o.lightSwitch) {
            StaticRenderable sr;
            sr.id = o.id;
            sr.render = *o.render;
            sr.position = position;
            sr.rotation = rotation;
            sr.scale = o.transform.scale;
            std::string assetError;
            if (!RequestVisualAssets(o, &assetError)) return fail(o, assetError);
            if (o.render->shape == SceneShape::Compound) return fail(o, "compound render needs a dynamic body");
            m_staticRenderables.push_back(sr);
        }

        // --- Gravity region ---
        if (o.gravity) {
            const SceneGravityComponent& g = *o.gravity;
            std::unique_ptr<GravityField> field;
            if (g.kind == SceneGravityKind::Radial) {
                field = std::make_unique<RadicalGravity>(position, g.magnitude);
            } else {
                const glm::vec3 acceleration = rotation * glm::vec3(0.0f, -g.magnitude, 0.0f);
                // Constructor selection must preserve the complete authored
                // vector. An angular tolerance would erase small rotations.
                if (acceleration == glm::vec3(0.0f, -kFaithfulMagnitude, 0.0f)) {
                    field = std::make_unique<FaithfulGravity>();
                } else {
                    field = std::make_unique<UniformGravity>(acceleration);
                }
            }
            std::unique_ptr<GravityVolume> volume;
            if (g.regionShape == SceneRegionShape::Sphere) {
                volume = std::make_unique<SphericalVolume>(position, g.regionRadius);
            } else {
                volume = std::make_unique<BoxVolume>(position, g.regionHalfExtents);
            }
            m_gravityMap.AddRegion(*field, *volume);
            m_gravityFields.push_back(std::move(field));
            m_gravityVolumes.push_back(std::move(volume));
            m_gravityRegions.push_back(GravityRegion{o.id, g, position, rotation});
        }

        if(o.particleEmitter){
            ParticleEmitter e;e.id=o.id;e.transform=o.transform;e.pool=VisualParticlePool(*o.particleEmitter);
            m_particleEmitters.push_back(std::move(e));
            if(m_assets&&!o.particleEmitter->textureAsset.empty()){
                m_assets->AddRef(o.particleEmitter->textureAsset);m_referencedAssets.push_back(o.particleEmitter->textureAsset);
                m_assets->RequestTexture(o.particleEmitter->textureAsset,JobPriority::High);
            }
        }
        if(o.audioEmitter){
            AudioEmitter emitter;emitter.id=o.id;emitter.transform=o.transform;emitter.settings=*o.audioEmitter;
            emitter.wantPlay=emitter.settings.playOnStart;
            if(!isDynamic)emitter.staticBody=bodyHandle;
            m_audioEmitters.push_back(emitter);
            if(m_assets&&!emitter.settings.asset.empty()){
                m_assets->AddRef(emitter.settings.asset);m_referencedAssets.push_back(emitter.settings.asset);
                if(emitter.settings.enabled)m_assets->RequestAudio(emitter.settings.asset);
            }
        }
        if(o.audioListener&&o.audioListener->enabled){
            AudioListener listener;listener.id=o.id;listener.transform=o.transform;listener.settings=*o.audioListener;
            if(!isDynamic)listener.staticBody=bodyHandle;
            m_audioListener=listener;
        }
        if (o.renderCamera) {
            RenderCamera camera; camera.id = o.id; camera.transform = o.transform; camera.settings = *o.renderCamera;
            if (!isDynamic) camera.staticBody = bodyHandle;
            m_renderCameras.push_back(camera);
        }
        // --- Standalone light ---
        if (o.light) {
            StaticLight light;
            light.id = o.id;
            light.light = *o.light;
            light.position = position;
            light.direction = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
            m_staticLights.push_back(light);
        }

        // --- Door / switch ---
        if (o.door) {
            if (!o.render) return fail(o, "door needs a render component");
            m_doors.emplace_back(m_physics, position, rotation, o.render->halfExtents, o.door->localHingeAxis,
                                 glm::radians(o.door->openAngleDegrees),
                                 glm::radians(o.door->angularSpeedDegreesPerSecond), o.render->color);
            const auto handle=m_doors.back().Handle();
            m_physics.SetCollisionFilter(handle,o.door->collisionLayer,o.door->collisionMask);
            m_physics.SetBodyTags(handle,o.tags);m_entityCategories[o.id].body=handle;
            m_doorIds.push_back(o.id);
        }
        if (o.lightSwitch) {
            if (!o.render) return fail(o, "light switch needs a render component");
            const SceneLightSwitchComponent& s = *o.lightSwitch;
            m_lightSwitches.emplace_back(position, rotation, o.render->halfExtents, s.localHingeAxis,
                                         glm::radians(s.toggleAngleDegrees),
                                         glm::radians(s.angularSpeedDegreesPerSecond), o.render->color,
                                         position + rotation * s.lampLocalOffset, s.lampColor, s.lampRange);
            m_lightSwitchIds.push_back(o.id);
        }

        // --- Vehicle ---
        if (o.vehicle) {
            if (!isDynamic || o.body->shape != SceneShape::Box) return fail(o, "vehicle needs a dynamic box body");
            if (m_vehicle) return fail(o, "M28 supports one vehicle per scene");
            Vehicle v;
            v.id = o.id;
            v.handle = bodyHandle;
            v.dynamicIndex = dynamicIndex;
            v.component = *o.vehicle;
            v.halfExtents = o.body->halfExtents;
            m_vehicle = v;
        }

        // --- Celestial ---
        if (o.celestial) {
            if (!o.body) return fail(o, "celestial needs a body");
            if (!isDynamic) {
                if (!(o.celestial->gravitationalParameter > 0.0f)) {
                    return fail(o, "a static celestial source needs a positive gravitational parameter");
                }
                m_pointMassSources.push_back({o.id, o.name, position, o.celestial->gravitationalParameter});
            }
        }

        // --- Atmosphere ---
        if (o.atmosphere) {
            if (m_atmosphere) return fail(o, "M28 supports one atmosphere per scene");
            if (!o.celestial || !(o.celestial->gravitationalParameter > 0.0f)) {
                return fail(o, "atmosphere needs a static celestial gravitational parameter");
            }
            const SceneAtmosphereComponent& a = *o.atmosphere;
            AtmosphereParameters parameters;
            parameters.referenceRadius = a.referenceRadius;
            parameters.topRadius = a.topRadius;
            parameters.gravitationalParameter = o.celestial->gravitationalParameter;
            parameters.polytropicExponent = a.polytropicExponent;
            parameters.referenceDensity = a.referenceDensity;
            parameters.oxidizerMassFraction = a.oxidizerMassFraction;
            parameters.referenceTemperatureKelvin = a.referenceTemperatureKelvin;
            const RadialTerrain* solid = nullptr;
            if (!m_terrains.empty() && m_terrains.back().id == o.id) {
                m_atmosphereTerrain = m_terrains.back().surface;
                solid = m_atmosphereTerrain.get();
            }
            m_atmosphere.emplace(Atmosphere{o.id, o.name, AtmosphereField(parameters, solid),
                                            ReferenceFrame{position, rotation, glm::vec3(0.0f), glm::vec3(0.0f)},
                                            o.celestial->gravitationalParameter});
        }

        // --- Combustible ---
        if (o.combustible) {
            if (!isDynamic) return fail(o, "combustible needs a dynamic body");
            Combustible c;
            c.id = o.id;
            c.name = o.name;
            c.handle = bodyHandle;
            c.dynamicIndex = dynamicIndex;
            c.sourceRadius = o.body->shape == SceneShape::Sphere ? o.body->radius : o.body->halfExtents.x;
            m_combustibles.push_back(c);
        }

        // --- Fluid volume ---
        if (o.fluidVolume) {
            FluidVolume fv;
            fv.id = o.id;
            fv.component = *o.fluidVolume;
            fv.position = position;
            fv.rotation = rotation;
            const float s = o.fluidVolume->spacing;
            fv.particleMass = m_fluidSettings.restDensity * s * s * s;
            m_fluidVolumes.push_back(fv);
        }

        // --- Player start ---
        if (o.playerStart) {
            m_physics.SetPlayerCollisionFilter(o.playerStart->collisionLayer,o.playerStart->collisionMask);
            if (m_playerStart) return fail(o, "more than one player-start");
            m_playerStart = PlayerStart{position, o.playerStart->yawDegrees, o.playerStart->view,
                o.playerStart->density, o.playerStart->fluidDrag, o.playerStart->swimAcceleration};
        }
    }

    for (const Combustible& c : m_combustibles) {
        if (!scene.Find(c.id)) continue;
        const SceneObject* o = scene.Find(c.id);
        const SceneCombustibleComponent& sc = *o->combustible;
        CombustibleMaterial fuel;
        fuel.heatCapacityJPerK = sc.heatCapacityJPerK;
        fuel.initialFuelMassKg = sc.initialFuelMassKg;
        fuel.ignitionTemperatureK = sc.ignitionTemperatureK;
        fuel.maximumFuelRateKgPerSecond = sc.maximumFuelRateKgPerSecond;
        fuel.radiativeAreaSquareMeters = sc.radiativeAreaSquareMeters;
        fuel.retainedCombustionHeatFraction = sc.retainedCombustionHeatFraction;
        float initialTemperature = 300.0f;
        if (m_atmosphere) {
            const AtmosphereSample gas = m_atmosphere->field.Sample(m_physics.GetTransform(c.handle).position,
                                                                    m_atmosphere->frame);
            if (gas.temperatureKelvin > 0.0f) initialTemperature = gas.temperatureKelvin;
        }
        m_combustion.AddBody(c.handle, fuel, initialTemperature);
    }

    RebuildCelestialParticipants();
    if (m_assets && !m_fluidVolumes.empty() && m_assets->GetRenderer()) {
        m_fluidMesh = m_assets->GetRenderer()->CreateMesh(MeshData{});
    }
    if (authored) PopulateFluid();
    return true;
}

void RuntimeWorld::RebuildCelestialParticipants() {
    // Pairwise Newtonian set: every LIVE dynamic celestial body, plus a
    // vehicle that samples local gravity (the classic scene's spacecraft).
    // Coarse celestial entities contribute through CoarseSimulation and
    // Simulation's coarse-to-live pass instead.
    m_celestialParticipants.clear();
    m_operatorThrusts.clear();
    bool anyCelestial = false;
    for (const EntityRecord& e : m_entities) {
        if (e.lifecycle == EntityLifecycle::Destroyed || !e.definition.celestial) continue;
        anyCelestial = true;
        if (e.fidelity == SimulationFidelity::Full) {
            const BodyHandle handle = m_dynamicBodies[e.slot].Handle();
            m_celestialParticipants.push_back(handle);
            // Operator forces belong to the entity definition, but their
            // physics handle belongs to this live incarnation. Rebuild both
            // inventories after creation, destruction or fidelity transitions.
            if (e.definition.celestial->operatorThrustForce > 0.0f)
                m_operatorThrusts.push_back({handle, e.definition.celestial->operatorThrustForce});
        }
    }
    if (m_vehicle && m_vehicle->component.gravity == SceneVehicleGravity::Local && anyCelestial) {
        // A vehicle may also explicitly carry the celestial component. It is
        // still one physical participant, not a second copy of every pair.
        const auto present = std::find_if(m_celestialParticipants.begin(), m_celestialParticipants.end(),
            [&](BodyHandle handle) { return handle.id == m_vehicle->handle.id; });
        if (present == m_celestialParticipants.end()) m_celestialParticipants.push_back(m_vehicle->handle);
    }
    m_celestial = std::make_unique<CelestialGravity>(m_celestialParticipants);
}

std::vector<BodyHandle> RuntimeWorld::PickableBodies() const {
    std::vector<BodyHandle> handles;
    for (const EntityRecord& e : m_entities) {
        if (e.lifecycle == EntityLifecycle::Destroyed || e.fidelity != SimulationFidelity::Full) continue;
        if (e.definition.body && e.definition.body->pickable) handles.push_back(m_dynamicBodies[e.slot].Handle());
    }
    return handles;
}

void RuntimeWorld::PopulateFluid() {
    if (!m_fluidVolumes.empty()) m_fluidCoupling = std::make_unique<ProductionFluidCoupling>();
    else m_fluidCoupling.reset();
    m_fluid->Clear();
    m_emittedParticles = 0;
    for (const FluidVolume& fv : m_fluidVolumes) {
        const SceneFluidVolumeComponent& c = fv.component;
        const float s = c.spacing;
        const float x0 = -0.5f * static_cast<float>(c.countX - 1);
        const float z0 = -0.5f * static_cast<float>(c.countZ - 1);
        for (int y = 0; y < c.countY; ++y) {
            for (int z = 0; z < c.countZ; ++z) {
                for (int x = 0; x < c.countX; ++x) {
                    const glm::vec3 local((x0 + static_cast<float>(x)) * s, static_cast<float>(y) * s,
                                          (z0 + static_cast<float>(z)) * s);
                    m_fluid->AddParticle(fv.position + fv.rotation * local, glm::vec3(0.0f), fv.particleMass);
                }
            }
        }
    }
}

bool RuntimeWorld::EmitFluidParticle() {
    for (const FluidVolume& fv : m_fluidVolumes) {
        const SceneFluidVolumeComponent& c = fv.component;
        if (!c.emitter) continue;
        if (static_cast<int>(m_fluid->Particles().size()) >= c.maxParticles) continue;
        const int column = static_cast<int>(m_emittedParticles % 9);
        const glm::vec3 spread(static_cast<float>(column % 3 - 1) * 0.3f, 0.0f,
                               static_cast<float>(column / 3 - 1) * 0.3f);
        m_fluid->AddParticle(fv.position + fv.rotation * (c.emitterLocalOffset + spread), glm::vec3(0.0f),
                             fv.particleMass);
        ++m_emittedParticles;
        return true;
    }
    return false;
}

void RuntimeWorld::RestoreAuthoredState() {
    m_scripts.reset();m_ui.reset();m_touchEntityHistory.clear();m_physics.ClearTouchHistory();
    if (!m_built) return;
    for(const auto& o:ScriptObjects())if(o.ui&&o.ui->enabled){std::string error;UI().Load(o.ui->asset,o.ui->name,o.id,error);}
    for(auto& [id,info]:m_entityCategories){(void)id;info.tags=info.authoredTags;m_physics.SetBodyTags(info.body,info.tags);}
    // Every surviving entity returns to its definition's state at Full
    // fidelity (the policy re-decides on the next step). Destroyed entities
    // stay destroyed: destruction is permanent within a run.
    for (EntityRecord& e : m_entities) {
        if (e.lifecycle == EntityLifecycle::Destroyed) continue;
        const EntityPhysicalState authored = StateFromDefinition(e.definition);
        e.state = authored;
        e.coarseMotion = CoarseMotion::Settled;
        e.forcedFidelity.reset();
        if (e.fidelity == SimulationFidelity::Full) {
            const BodyHandle handle = m_dynamicBodies[e.slot].Handle();
            m_physics.ResetBody(handle, authored.position, authored.rotation);
            m_physics.SetLinearVelocity(handle, authored.linearVelocity);
            m_physics.SetAngularVelocity(handle, authored.angularVelocity);
            m_dynamicBodies[e.slot].SetPoseFromState(authored.position, authored.rotation);
            m_dynamicBodies[e.slot].SnapPresentation();
        } else {
            std::string error;
            InstantiateEntityBody(e, authored, &error);
            e.fidelity = SimulationFidelity::Full;
            e.lifecycle = EntityLifecycle::Active;
            e.dormantSinceSeconds = -1.0;
            ++e.reconstructions;
        }
    }
    if(m_hasScripts){
        for(auto& e:m_extraEntities)if(e.authored&&e.lifecycle!=EntityLifecycle::Destroyed){
            SetEntityState(e.id,StateFromDefinition(e.definition));SetRuntimeTransform(e.id,e.definition.transform);
        }
        // Restore parent-local data independently of entity ordering. Physical
        // records hold the flattened authored baseline; no game rule is involved.
        for(auto& o:m_hierarchy.Objects())if(const auto* e=FindEntity(o.id))if(e->authored){
            auto local=e->definition.transform;
            if(o.parent){const auto* parent=RuntimeDefinition(o.parent);const auto* record=FindEntity(o.parent);
                if(parent){const auto p=record&&record->authored?record->definition.transform:parent->transform;
                    local.position=glm::inverse(p.rotation)*(local.position-p.position)/p.scale;
                    local.rotation=glm::inverse(p.rotation)*local.rotation;local.scale/=p.scale;}}
            o.transform=local;
        }
    }
    for(auto& [id,d]:m_scriptDefinitions)if(const auto* record=FindEntity(id))if(record->authored&&record->definition.body&&d.body){
        d.body->enabled=record->definition.body->enabled;
        m_physics.SetBodyEnabled(RuntimeBody(id),d.body->enabled);
    }
    ++m_entityVersion;
    RebuildCelestialParticipants();
    m_combustion.Reset();
    PopulateFluid();
}

std::string RuntimeWorld::NameOfBody(BodyHandle handle) const {
    for (std::size_t i = 0; i < m_dynamicBodies.size(); ++i) {
        if (m_dynamicBodies[i].IsLive() && m_dynamicBodies[i].Handle().id == handle.id) return m_dynamicVisuals[i].name;
    }
    return std::string();
}

// --- Milestone 29 -------------------------------------------------------

const EntityRecord* RuntimeWorld::FindEntity(EntityId id) const {
    for (const EntityRecord& e : m_entities) {
        if (e.id == id) return &e;
    }
    for (const auto& e : m_extraEntities) if (e.id == id) return &e;
    for (auto& e : m_extraEntities) if (e.id == id) return &e;
    return nullptr;
}

EntityRecord* RuntimeWorld::FindEntity(EntityId id) {
    for (EntityRecord& e : m_entities) {
        if (e.id == id) return &e;
    }
    for (auto& e : m_extraEntities) if (e.id == id) return &e;
    return nullptr;
}

EntityId RuntimeWorld::EntityIdOfBody(BodyHandle handle) const {
    if (!handle.IsValid()) return kInvalidSceneObjectId;
    for (const EntityRecord& e : m_entities) {
        if (e.lifecycle != EntityLifecycle::Destroyed && m_dynamicBodies[e.slot].IsLive() &&
            m_dynamicBodies[e.slot].Handle().id == handle.id) {
            return e.id;
        }
    }
    for(const auto& entry:m_entityCategories){
        // Dynamic entries retain authored category data while unloaded. Their
        // cached handle is not a live incarnation; only the loop above may
        // resolve them. This fallback supplies ordinary static bodies.
        const auto* entity=FindEntity(entry.first);
        if(entity && entity->slot!=std::numeric_limits<std::size_t>::max())continue;
        if(entry.second.body.id==handle.id && RuntimeDefinition(entry.first))return entry.first;
    }
    return kInvalidSceneObjectId;
}

bool RuntimeWorld::GetEntityState(EntityId id, EntityPhysicalState& outState) const {
    const EntityRecord* e = FindEntity(id);
    if (!e || e->lifecycle == EntityLifecycle::Destroyed) return false;
    if (e->slot == std::numeric_limits<std::size_t>::max()) { outState=e->state; return true; }
    if (e->fidelity == SimulationFidelity::Full) {
        const BodyHandle handle = m_dynamicBodies[e->slot].Handle();
        const BodyTransform transform = m_physics.GetTransform(handle);
        outState.position = transform.position;
        outState.rotation = transform.rotation;
        outState.linearVelocity = m_physics.GetLinearVelocity(handle);
        outState.angularVelocity = m_physics.GetAngularVelocity(handle);
    } else {
        outState = e->state;
    }
    return true;
}

bool RuntimeWorld::SetEntityState(EntityId id, const EntityPhysicalState& state) {
    EntityRecord* e = FindEntity(id);
    if (!e || e->lifecycle == EntityLifecycle::Destroyed) return false;
    e->state = state;
    if (e->slot == std::numeric_limits<std::size_t>::max()) {
        for(auto& r:m_staticRenderables)if(r.id==id){r.position=state.position;r.rotation=state.rotation;}
        for(auto& b:m_staticBodies)if(b.id==id){m_physics.ResetBody(b.handle,state.position,state.rotation);b.position=state.position;b.rotation=state.rotation;}
        for(auto& a:m_audioEmitters)if(a.id==id){a.transform.position=state.position;a.transform.rotation=state.rotation;}
        for(auto& c:m_renderCameras)if(c.id==id){c.transform.position=state.position;c.transform.rotation=state.rotation;}
        for(auto& l:m_staticLights)if(l.id==id){l.position=state.position;l.direction=state.rotation*glm::vec3(0,0,-1);}
        return true;
    }
    e->coarseMotion = glm::length(state.linearVelocity) < kSettledLinearSpeed &&
                              glm::length(state.angularVelocity) < kSettledAngularSpeed
                          ? CoarseMotion::Settled
                          : CoarseMotion::Inertial;
    if (e->fidelity == SimulationFidelity::Full) {
        const BodyHandle handle = m_dynamicBodies[e->slot].Handle();
        m_physics.ResetBody(handle, state.position, state.rotation);
        m_physics.SetLinearVelocity(handle, state.linearVelocity);
        m_physics.SetAngularVelocity(handle, state.angularVelocity);
    }
    m_dynamicBodies[e->slot].SetPoseFromState(state.position, state.rotation);
    m_dynamicBodies[e->slot].SnapPresentation();
    return true;
}

bool RuntimeWorld::SetEntityFidelity(EntityId id, SimulationFidelity fidelity, std::string* outError) {
    EntityRecord* e = FindEntity(id);
    if (!e) {
        if (outError) *outError = "unknown entity id " + std::to_string(id);
        return false;
    }
    if (e->lifecycle == EntityLifecycle::Destroyed) {
        if (outError) *outError = "entity " + std::to_string(id) + " is destroyed";
        return false;
    }
    if (fidelity != SimulationFidelity::Full && e->requiresFull) {
        if (outError) *outError = "entity " + std::to_string(id) + " (" + e->name + ") has no reduced representation";
        return false;
    }
    if (fidelity == e->fidelity) return true;

    if (e->fidelity == SimulationFidelity::Full) {
        // Leaving Full: capture the live state, then release the body.
        GetEntityState(id, e->state);
        e->coarseMotion = glm::length(e->state.linearVelocity) < kSettledLinearSpeed &&
                                  glm::length(e->state.angularVelocity) < kSettledAngularSpeed
                              ? CoarseMotion::Settled
                              : CoarseMotion::Inertial;
        ReleaseEntityBody(*e);
        m_dynamicBodies[e->slot].SetPoseFromState(e->state.position, e->state.rotation);
        m_dynamicBodies[e->slot].SnapPresentation();
    } else if (fidelity == SimulationFidelity::Full) {
        // Reconstruction from the retained state.
        if (!InstantiateEntityBody(*e, e->state, outError)) return false;
        ++e->reconstructions;
    }
    e->fidelity = fidelity;
    e->lifecycle = fidelity == SimulationFidelity::Dormant ? EntityLifecycle::Unloaded : EntityLifecycle::Active;
    e->dormantSinceSeconds = fidelity == SimulationFidelity::Dormant ? m_simulationTime : -1.0;
    ++m_transitionsThisStep;
    ++m_entityVersion;
    if (e->definition.celestial) RebuildCelestialParticipants();
    return true;
}

bool RuntimeWorld::ForceEntityFidelity(EntityId id, std::optional<SimulationFidelity> fidelity, std::string* outError) {
    EntityRecord* e = FindEntity(id);
    if (!e) {
        if (outError) *outError = "unknown entity id";
        return false;
    }
    e->forcedFidelity = fidelity;
    if (fidelity) return SetEntityFidelity(id, *fidelity, outError);
    return true;
}

bool RuntimeWorld::ValidateEntityDestruction(EntityId id, std::string& error) const {
    const EntityRecord* e = FindEntity(id);
    if (!e) {
        error = "unknown entity id " + std::to_string(id);
        return false;
    }
    if (e->lifecycle == EntityLifecycle::Destroyed) return true;
    const bool ownedComponent = e->slot == std::numeric_limits<std::size_t>::max() &&
        (e->definition.door || e->definition.lightSwitch || e->definition.gravity || e->definition.atmosphere ||
         e->definition.fluidVolume || e->definition.playerStart || e->definition.audioListener);
    if (e->definition.vehicle || e->definition.combustible || ownedComponent) {
        error = "entity " + std::to_string(id) + " (" + e->name + ") cannot be destroyed at runtime";
        return false;
    }
    return true;
}

bool RuntimeWorld::DestroyEntity(EntityId id, std::string* outError) {
    std::string error;
    if (!ValidateEntityDestruction(id, error)) { if (outError) *outError = error; return false; }
    EntityRecord* e = FindEntity(id);
    if (e->lifecycle == EntityLifecycle::Destroyed) return true;
    const bool extra=e->slot==std::numeric_limits<std::size_t>::max();
    if(!extra)ReleaseEntityBody(*e);
    else {
        for(auto& b:m_staticBodies)if(b.id==id)m_physics.DestroyBody(b.handle);
        const auto eraseId=[id](auto& values){values.erase(std::remove_if(values.begin(),values.end(),[id](const auto& v){return v.id==id;}),values.end());};
        eraseId(m_staticBodies);eraseId(m_staticRenderables);eraseId(m_staticLights);
    }
    m_particleEmitters.erase(std::remove_if(m_particleEmitters.begin(),m_particleEmitters.end(),[&](const auto& e){return e.id==id;}),m_particleEmitters.end());
    if(m_audioSystem)for(auto& emitter:m_audioEmitters)if(emitter.id==id){m_audioSystem->DestroyVoice(emitter.voice);emitter.voice={};emitter.wantPlay=false;}
    if(m_ui)m_ui->RemoveOwner(id);
    e->lifecycle = EntityLifecycle::Destroyed;
    if (m_cameraRenderer) for (auto& camera : m_renderCameras) {
        if (camera.id == id) { m_cameraRenderer->DestroyRenderTarget(camera.target); camera.target = {}; }
    }
    m_staticLights.erase(std::remove_if(m_staticLights.begin(),m_staticLights.end(),[id](const auto& light){return light.id==id;}),m_staticLights.end());
    e->fidelity = SimulationFidelity::Dormant;
    if(!extra)m_dynamicVisuals[e->slot].hasRender = false;
    ++m_entityVersion;
    if (e->definition.celestial) RebuildCelestialParticipants();
    return true;
}

EntityId RuntimeWorld::AllocateRuntimeEntityId() {
    return m_nextRuntimeId++;
}

void RuntimeWorld::SetNextRuntimeEntityId(EntityId next) {
    m_nextRuntimeId = std::max(next, kRuntimeEntityIdBase);
    for (const EntityRecord& e : m_entities) {
        if (!e.authored && e.id >= m_nextRuntimeId) m_nextRuntimeId = e.id + 1;
    }
}

bool RuntimeWorld::ValidateEntityDefinition(const SceneObject& definition, std::string& error) {
    const auto singleLine = [](const std::string& value) {
        return value.find_first_of("\r\n") == std::string::npos && value.find('\0') == std::string::npos;
    };
    if (!singleLine(definition.name) || (definition.body && !singleLine(definition.body->terrainSurface)) ||
        (definition.render && (!singleLine(definition.render->meshAsset) || !singleLine(definition.render->textureAsset)))) {
        error = "runtime entity strings must be single-line and contain no NUL bytes";
        return false;
    }
    // Finite fields / enum validation follows the canonical authored schema.
    // Check the smaller subset runtime creation can actually instantiate below.
    Scene validation;
    if(definition.joint&&(!definition.joint->bodyA||definition.joint->bodyA==definition.joint->bodyB||!ValidJointSettings(definition.joint->settings))){error="invalid runtime joint settings";return false;}
    SceneObject copy = definition;
    // Cross-body references are checked at scene/batch/save preflight, not in
    // this one-object component validator.
    copy.joint.reset();
    copy.id = 1;
    validation.InsertObject(copy);
    std::string fingerprint;
    if (!ComputeSceneFingerprint(validation, fingerprint, error)) return false;
    if (definition.vehicle || definition.combustible || definition.atmosphere || definition.fluidVolume ||
        definition.audioListener || definition.playerStart || definition.door || definition.lightSwitch || definition.gravity ||
        (definition.body && (definition.body->shape==SceneShape::Terrain || definition.body->shape==SceneShape::Mesh))) {
        error="runtime creation supports prop bodies/render/light/audio-emitter/render-camera/celestial components; scene-global and gameplay ownership components remain scene-authored";
        return false;
    }
    std::string block;WriteSceneObjectBlock(copy,block);std::vector<std::string> lines;
    std::istringstream in(block);std::string line;while(std::getline(in,line))lines.push_back(line);
    size_t index=0;SceneObject parsed;if(!ParseSceneObjectBlock(lines,index,parsed,error))return false;
    if (!definition.body) return true;
    if(definition.body->motion==SceneBodyMotion::Static&&definition.body->shape==SceneShape::Compound){error="static compound creation is unsupported";return false;}
    const auto positive = [](const glm::vec3& v) { return v.x > 0 && v.y > 0 && v.z > 0; };
    const auto& b = *definition.body;
    const float norm = glm::dot(definition.transform.rotation, definition.transform.rotation);
    if (!(b.mass > 0) || !std::isfinite(1.0f / b.mass) || !(norm > 0) || !std::isfinite(norm) ||
        b.friction < 0 || b.restitution < 0 || b.restitution > 1 ||
        (b.shape == SceneShape::Box && !positive(b.halfExtents)) ||
        (b.shape == SceneShape::Sphere && !(b.radius > 0)) ||
        (b.shape == SceneShape::Compound && b.compoundBoxes.empty())) {
        error = "invalid runtime body mass, geometry, material or rotation";
        return false;
    }
    for (const auto& box : b.compoundBoxes) {
        if (!positive(box.halfExtents)) { error = "compound half-extents must be positive"; return false; }
    }
    if (definition.render && (definition.render->shape == SceneShape::Terrain ||
        (definition.render->shape == SceneShape::Compound && b.shape != SceneShape::Compound))) {
        error = "render geometry is incompatible with the runtime body";
        return false;
    }
    return true;
}

bool RuntimeWorld::ValidateEntityCreation(const SceneObject& definition, std::string& error) const {
    if (!m_built) { error = "no world"; return false; }
    if(definition.ui&&definition.ui->enabled){
        const auto* db=m_assets?m_assets->Assets():nullptr;const auto* asset=db?db->Find(definition.ui->asset):nullptr;UIDocument doc;
        if(!asset||asset->missing||asset->type!=AssetType::UI){error="missing UI document asset";return false;}
        if(!LoadUIDocument(asset->path,doc,error)||!ValidateUIAssets(doc,*db,error))return false;
        if(m_ui&&m_ui->Find(definition.ui->name)){error="duplicate runtime UI document name";return false;}
    }
    Scene classified;auto classifiedDefinition=definition;
    if(!classifiedDefinition.id)classifiedDefinition.id=1;
    classified.InsertObject(classifiedDefinition);
    if(!ValidateSceneClassification(classified,m_categories,error))return false;
    if (!ValidateEntityDefinition(definition, error) || !ValidateVisualAssets(definition, error)) return false;
    if (definition.render && definition.render->textureCamera) {
        const SceneObjectId reference = definition.render->textureCamera;
        const bool present = std::any_of(m_renderCameras.begin(), m_renderCameras.end(), [reference](const RenderCamera& camera) { return camera.id == reference; });
        // References identify authored camera definitions, not live GPU images.
        // Destroyed cameras resolve to the ordinary white fallback, including
        // when a validated delta destroys a camera before creating its consumer.
        if (!present || !definition.render->textureAsset.empty()) {
            error = "runtime texture references no authored camera or conflicts with an asset texture"; return false;
        }
    }
    const EntityId id = definition.id == kInvalidSceneObjectId ? m_nextRuntimeId : definition.id;
    if (id < kRuntimeEntityIdBase || id >= static_cast<EntityId>(std::numeric_limits<std::int64_t>::max())) {
        error = "runtime-created entity id is outside the allocatable runtime range";
        return false;
    }
    if (FindEntity(id)) { error = "entity id " + std::to_string(id) + " already exists"; return false; }
    return true;
}

EntityId RuntimeWorld::CreateEntity(const SceneObject& definitionIn, const EntityPhysicalState* state,
                                    std::string* outError) {
    std::string error;
    if (!ValidateEntityCreation(definitionIn, error)) { if (outError) *outError = error; return kInvalidSceneObjectId; }
    SceneObject definition = definitionIn;
    if (definition.id == kInvalidSceneObjectId) definition.id = m_nextRuntimeId;
    auto local=definition;
    if(!state&&definition.parent){
        const auto* parent=m_hierarchy.Find(definition.parent);
        SceneTransform parentPose;
        if(parent)parentPose=PresentedTransform(parent->id,parent->transform,1.0f);
        else {EntityPhysicalState p;if(!GetEntityState(definition.parent,p)){if(outError)*outError="unknown runtime parent";return 0;}parentPose.position=p.position;parentPose.rotation=p.rotation;}
        definition.transform.position=parentPose.position+parentPose.rotation*(parentPose.scale*local.transform.position);
        definition.transform.rotation=glm::normalize(parentPose.rotation*local.transform.rotation);definition.transform.scale*=parentPose.scale;
    }
    if(state){definition.transform.position=state->position;definition.transform.rotation=state->rotation;
        if(definition.body)definition.body->initialLinearVelocity=state->linearVelocity;}
    Scene batch;batch.Settings()=m_settings;batch.InsertObject(definition);
    FidelityPolicyContext context;
    if(!AppendSceneObjects(batch,false,context,error)){if(outError)*outError=error;return 0;}
    if(!definition.body||definition.body->motion==SceneBodyMotion::Static){
        EntityRecord e;e.id=definition.id;e.name=definition.name;e.definition=definition;e.authored=false;
        e.requiresFull=true;e.state=StateFromDefinition(definition);e.slot=std::numeric_limits<std::size_t>::max();
        m_extraEntities.push_back(e);
    }
    FindEntity(definition.id)->definition=local;
    m_hierarchy.InsertObject(local);
    if(state)SetEntityState(definition.id,*state);
    m_nextRuntimeId=std::max(m_nextRuntimeId,definition.id+1);
    return definition.id;
}

EntityId RuntimeWorld::SpawnPrefab(const AssetId& asset,const SceneTransform& placement,std::string& error) {
    if(!m_assets||!m_assets->Assets()){error="no project asset database";return 0;}
    Scene source;if(!LoadPrefab(*m_assets->Assets(),asset,source,error))return 0;
    Scene instance;instance.SetNextId(m_nextRuntimeId);SceneObjectId root=0;
    if(!InstantiatePrefab(instance,source,asset,placement,root,error))return 0;
    Scene flat;if(!FlattenHierarchy(instance,flat,error))return 0;
    if(!ValidateSceneClassification(flat,m_categories,error))return 0;
    // Complete preflight before creating any member. The resolved hierarchy is
    // ordinary data; runtime state and saved creations never depend on a live source.
    for(auto& o:flat.Objects()){
        o.prefabAsset.clear();o.prefabRoot=o.prefabSource=0;o.prefabIds.clear();o.prefabOverrides.clear();
        if(!ValidateEntityDefinition(o,error)||!ValidateVisualAssets(o,error))return 0;
    }
    for(const auto& o:instance.Objects())if(o.joint){m_jointParticipants.insert(o.joint->bodyA);if(o.joint->bodyB)m_jointParticipants.insert(o.joint->bodyB);}
    std::vector<EntityId> created;
    std::set<SceneObjectId> pending;
    for(const auto& o:instance.Objects())pending.insert(o.id);
    while(!pending.empty()){
        bool progress=false;
        for(const auto& original:instance.Objects())if(pending.count(original.id)&&(!original.parent||!pending.count(original.parent))){
            auto o=original;o.prefabAsset.clear();o.prefabRoot=o.prefabSource=0;o.prefabIds.clear();o.prefabOverrides.clear();
            if(o.render&&o.render->textureCamera&&pending.count(o.render->textureCamera))continue;
            auto id=CreateEntity(o,nullptr,&error);
            if(!id){for(auto prior:created)DestroyEntity(prior);return 0;}
            pending.erase(id);created.push_back(id);progress=true;
        }
        if(!progress){error="cyclic runtime camera/parent creation dependencies";for(auto prior:created)DestroyEntity(prior);return 0;}
    }
    SynchronizeJoints();
    return root;
}

SceneTransform RuntimeWorld::PresentedTransform(SceneObjectId id,const SceneTransform& fallback,float alpha) const {
    if(const auto* e=FindEntity(id))if(e->slot!=std::numeric_limits<std::size_t>::max()){
        const auto& body=m_dynamicBodies[e->slot];auto t=fallback;
        t.position=body.GetPresentedPosition(alpha);t.rotation=body.GetPresentedOrientation(alpha);return t;
    }
    // Physical children are independent ordinary bodies; hierarchy is not a
    // hidden constraint. Body-free visual/audio/light/camera children follow.
    for(const auto& b:m_staticBodies)if(b.id==id){auto t=fallback;const auto pose=m_physics.GetTransform(b.handle);t.position=pose.position;t.rotation=pose.rotation;return t;}
    const auto* local=m_hierarchy.Find(id);
    if(local&&local->parent){
        const auto* parent=m_hierarchy.Find(local->parent);
        if(parent){auto p=PresentedTransform(parent->id,parent->transform,alpha);auto t=local->transform;
            t.position=p.position+p.rotation*(p.scale*t.position);t.rotation=glm::normalize(p.rotation*t.rotation);t.scale=p.scale*t.scale;return t;}
    }
    if(const auto* e=FindEntity(id)){auto t=fallback;t.position=e->state.position;t.rotation=e->state.rotation;return t;}
    return local?local->transform:fallback;
}

bool RuntimeWorld::DestroyHierarchy(EntityId root,std::string& error) {
    std::vector<EntityId> ids{root};
    for(size_t i=0;i<ids.size();++i){
        for(const auto& e:m_entities)if(e.definition.parent==ids[i])ids.push_back(e.id);
        for(const auto& e:m_extraEntities)if(e.definition.parent==ids[i])ids.push_back(e.id);
    }
    for(auto id:ids)if(!ValidateEntityDestruction(id,error))return false;
    for(auto it=ids.rbegin();it!=ids.rend();++it)if(!DestroyEntity(*it,&error))return false;
    return true;
}

void RuntimeWorld::SetFidelityPolicy(std::unique_ptr<FidelityPolicy> policy) {
    m_policy = std::move(policy);
}

void RuntimeWorld::EvaluateFidelityPolicy(const FidelityPolicyContext& context, const std::vector<EntityId>& pinned) {
    m_transitionsThisStep = 0;
    if (!m_policy) return;
    for (EntityRecord& e : m_entities) {
        if (e.lifecycle == EntityLifecycle::Destroyed || !e.managed || e.requiresFull || e.forcedFidelity) continue;
        if (std::find(pinned.begin(), pinned.end(), e.id) != pinned.end()) {
            if (e.fidelity != SimulationFidelity::Full) SetEntityFidelity(e.id, SimulationFidelity::Full);
            continue;
        }
        FidelityPolicyEntity view;
        view.id = e.id;
        view.current = e.fidelity;
        EntityPhysicalState state;
        GetEntityState(e.id, state);
        view.position = state.position;
        view.linearVelocity = state.linearVelocity;
        const SimulationFidelity desired = m_policy->Desired(view, context);
        if (desired != e.fidelity) SetEntityFidelity(e.id, desired);
    }
}

RuntimeWorld::LifecycleCounts RuntimeWorld::CountLifecycle() const {
    LifecycleCounts counts;
    for (const EntityRecord& e : m_entities) {
        if (e.lifecycle == EntityLifecycle::Destroyed) ++counts.destroyed;
        else if (e.fidelity == SimulationFidelity::Full) ++counts.full;
        else if (e.fidelity == SimulationFidelity::Coarse) ++counts.coarse;
        else ++counts.dormant;
    }
    counts.physicsBodies = m_physics.AliveBodyCount();
    counts.dynamicPhysicsBodies = m_physics.DynamicBodyCount();
    counts.transitionsThisStep = m_transitionsThisStep;
    return counts;
}

Door* RuntimeWorld::FindDoor(SceneObjectId id) {
    for (std::size_t i = 0; i < m_doorIds.size(); ++i) {
        if (m_doorIds[i] == id) return &m_doors[i];
    }
    return nullptr;
}

LightSwitch* RuntimeWorld::FindLightSwitch(SceneObjectId id) {
    for (std::size_t i = 0; i < m_lightSwitchIds.size(); ++i) {
        if (m_lightSwitchIds[i] == id) return &m_lightSwitches[i];
    }
    return nullptr;
}

void RuntimeWorld::Destroy() {
    m_jointOwners.clear();m_jointParticipants.clear();m_runtimeJoints.clear();
    m_scripts.reset();m_ui.reset();m_scriptDefinitions.clear();m_touchEntityHistory.clear();m_hasScripts=false;
    EndAudio();
    m_particleEmitters.clear();
    m_audioEmitters.clear();m_audioListener.reset();m_audioSystem=nullptr;
    if (!m_built) return;
    if (m_cameraRenderer) for (const auto& camera : m_renderCameras) m_cameraRenderer->DestroyRenderTarget(camera.target);
    m_renderCameras.clear(); m_cameraRenderer = nullptr; m_cameraFrame = 0;
    for (Door& door : m_doors) door.Destroy(m_physics);
    for (const DynamicBody& body : m_dynamicBodies) {
        if (body.IsLive()) m_physics.DestroyBody(body.Handle());
    }
    for (const StaticBody& body : m_staticBodies) m_physics.DestroyBody(body.handle);
    for (const Terrain& terrain : m_terrains) m_physics.DestroyBody(terrain.handle);
    if (m_assets && m_assets->GetRenderer() && m_fluidMesh.IsValid()) {
        m_assets->GetRenderer()->DestroyMesh(m_fluidMesh);
    }
    m_fluidMesh = MeshHandle{};
    m_physics.Shutdown();

    m_doors.clear();
    m_doorIds.clear();
    m_lightSwitches.clear();
    m_lightSwitchIds.clear();
    m_dynamicBodies.clear();
    m_dynamicVisuals.clear();
    m_entities.clear();
    m_entityCategories.clear();
    m_policy.reset();
    m_nextRuntimeId = kRuntimeEntityIdBase;
    m_entityVersion = 0;
    m_transitionsThisStep = 0;
    m_simulationTime = 0.0;
    m_staticBodies.clear();
    m_terrains.clear();
    m_staticRenderables.clear();
    m_staticLights.clear();
    m_gravityMap = GravityContextMap();
    m_gravityFields.clear();
    m_gravityVolumes.clear();
    m_gravityRegions.clear();
    m_vehicle.reset();
    m_celestial.reset();
    m_celestialParticipants.clear();
    m_pointMassSources.clear();
    m_operatorThrusts.clear();
    m_atmosphere.reset();
    m_atmosphereTerrain.reset();
    m_combustion.Clear();
    m_combustibles.clear();
    m_fluidCoupling.reset();
    m_fluid.reset();
    m_fluidSettings = FluidSettings{};
    m_fluidVolumes.clear();
    m_emittedParticles = 0;
    m_playerStart.reset();
    m_pickableBodies.clear();
    if (m_assets) {
        for (const AssetId& id : m_referencedAssets) m_assets->ReleaseRef(id);
    }
    m_referencedAssets.clear();
    m_assets = nullptr;
    m_hierarchy.Clear();
    m_extraEntities.clear();
    m_baselineFingerprint.clear();
    m_built = false;
}

TextureHandle RuntimeWorld::CameraTexture(SceneObjectId id) const {
    if (!id || !m_cameraRenderer) return {};
    if (const auto* entity = FindEntity(id)) if (entity->lifecycle == EntityLifecycle::Destroyed) return {};
    for (const auto& c : m_renderCameras) if (c.id == id) return m_cameraRenderer->RenderTargetTexture(c.target);
    return {};
}


void RuntimeWorld::UpdateVisualParticles(float dt){
    for(auto& e:m_particleEmitters){
        if(const auto* entity=FindEntity(e.id))if(entity->lifecycle==EntityLifecycle::Destroyed)continue;
        const auto t=PresentedTransform(e.id,e.transform,1);
        e.pool.Update(dt,t.position,t.rotation,t.scale,Gravity());
    }
}
bool RuntimeWorld::EmitParticleBurst(SceneObjectId id,unsigned count){
    for(auto& e:m_particleEmitters)if(e.id==id){const auto t=PresentedTransform(e.id,e.transform,1);e.pool.Burst(count,t.position,t.rotation,t.scale);return true;}
    return false;
}

CategoryMask RuntimeWorld::TagsOf(EntityId id)const{
    const auto it=m_entityCategories.find(id);const auto* entity=FindEntity(id);
    return it==m_entityCategories.end()||(entity&&entity->lifecycle==EntityLifecycle::Destroyed)?0:it->second.tags;
}
unsigned RuntimeWorld::RenderLayerOf(EntityId id)const{const auto it=m_entityCategories.find(id);return it==m_entityCategories.end()?0:it->second.renderLayer;}
bool RuntimeWorld::AddTag(EntityId id,unsigned tag){
    auto it=m_entityCategories.find(id);const auto* entity=FindEntity(id);
    if(it==m_entityCategories.end()||!m_categories.tags.names.count(tag)||(entity&&entity->lifecycle==EntityLifecycle::Destroyed))return false;
    it->second.tags|=CategoryBit(tag);m_physics.SetBodyTags(it->second.body,it->second.tags);return true;
}
bool RuntimeWorld::RemoveTag(EntityId id,unsigned tag){
    auto it=m_entityCategories.find(id);if(it==m_entityCategories.end()||tag>=64)return false;
    it->second.tags&=~CategoryBit(tag);m_physics.SetBodyTags(it->second.body,it->second.tags);return true;
}
std::vector<EntityId> RuntimeWorld::QueryEntities(CategoryMask required,CategoryMask excluded,const std::vector<EntityId>* candidates)const{
    std::vector<EntityId> result;
    for(const auto& [id,info]:m_entityCategories){
        if(candidates&&std::find(candidates->begin(),candidates->end(),id)==candidates->end())continue;
        const auto* e=FindEntity(id);if(e&&e->lifecycle==EntityLifecycle::Destroyed)continue;
        if((info.tags&required)==required&&!(info.tags&excluded))result.push_back(id);
    }
    return result;
}
