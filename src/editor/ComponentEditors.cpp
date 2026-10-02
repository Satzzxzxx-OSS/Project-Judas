#include "ComponentEditors.h"
#include <algorithm>
#include <filesystem>
#include "ScriptSystem.h"

#include <glm/gtc/quaternion.hpp>

#include "imgui.h"

#include "AssetDatabase.h"
#include "Project.h"
#include "EditorPanels.h"
#include "EditorWidgets.h"

namespace {
const char* const kShapeNames[] = {"box", "sphere", "compound", "mesh", "terrain"};
const char* const kBodyShapeNames[] = {"box", "sphere", "compound", "(mesh: not a body shape)", "terrain"};
const char* const kMotionNames[] = {"static", "dynamic"};
const char* const kGravityKindNames[] = {"radial", "uniform"};
const char* const kRegionNames[] = {"sphere", "box"};
const char* const kLightKindNames[] = {"point", "spot"};
const char* const kVehicleGravityNames[] = {"local", "celestial"};
const char* const kViewNames[] = {"third-person", "first-person"};

// The inspector's asset fields: a combo over the project's assets of one
// type (name shown, id stored), a drop target for the Asset Browser's
// drag payload, and an honest status line for the stored id.
void AssetField(EditorDocument& doc, const char* label, std::string& assetId, AssetType type, bool allowNone,
                EditorPanelState& state) {
    std::vector<std::string> labels, values;
    if (state.assets) {
        for (const auto& [id, record] : state.assets->Records()) {
            if (record.type != type) continue;
            labels.push_back(record.relativePath + (record.missing ? "  [missing]" : ""));
            values.push_back(id);
        }
    }
    LabelledCombo(doc, label, assetId, labels, values, allowNone);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragPayload)) {
            const std::string droppedId(static_cast<const char*>(payload->Data), payload->DataSize);
            const AssetRecord* record = state.assets ? state.assets->Find(droppedId) : nullptr;
            if (record && record->type == type) {
                doc.BeginEdit();
                assetId = droppedId;
                doc.CommitEdit();
            } else {
                state.status = std::string("Dropped asset is not a ") + AssetTypeName(type);
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (assetId.empty()) return;
    const AssetRecord* record = state.assets ? state.assets->Find(assetId) : nullptr;
    if (!record) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "  unknown asset id %s", assetId.c_str());
    } else if (record->missing) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "  file missing: %s", record->relativePath.c_str());
    } else {
        ImGui::TextDisabled("  id %s", assetId.c_str());
    }
}

void DrawUIComponent(EditorDocument& doc,SceneObject& o,EditorPanelState& state){
    auto& u=*o.ui;AssetField(doc,"UI document",u.asset,AssetType::UI,false,state);Checkbox(doc,"Enabled",u.enabled);
    char name[128];std::snprintf(name,sizeof(name),"%s",u.name.c_str());if(ImGui::InputText("Runtime document name",name,sizeof(name))){doc.BeginEdit();u.name=name;doc.CommitEdit();}
    ImGui::TextWrapped("Select the UI asset in the Asset Browser to edit its hierarchy. Runtime changes never edit this source.");
}
void DrawScripts(EditorDocument& doc,SceneObject& o,EditorPanelState& state){
    size_t remove=o.scripts.size();
    for(size_t i=0;i<o.scripts.size();++i){auto& slot=o.scripts[i];ImGui::PushID(static_cast<int>(i));
        ImGui::Text("Behaviour %zu (slot %llu)",i+1,static_cast<unsigned long long>(slot.id));
        AssetField(doc,"Script asset",slot.asset,AssetType::Script,false,state);Checkbox(doc,"Enabled",slot.enabled);
        if(state.assets&&!slot.asset.empty()){
            struct Metadata {std::filesystem::file_time_type time;std::string schema,error;};
            static std::map<std::string,Metadata> cache;
            const auto* record=state.assets->Find(slot.asset);
            if(record&&!record->missing){std::error_code ec;auto stamp=std::filesystem::last_write_time(record->path,ec);
                auto it=cache.find(record->path);if(it==cache.end()||it->second.time!=stamp){Metadata m;m.time=stamp;
                    ScriptSystem::Inspect(*state.assets,slot.asset,m.schema,m.error);it=cache.insert_or_assign(record->path,std::move(m)).first;}
                auto& metadata=it->second;std::vector<ScriptProperty> fields;std::string error=metadata.error;
                if(error.empty()&&ScriptSystem::ReadProperties(metadata.schema,slot.properties,fields,error)){
                    for(auto& f:fields){bool changed=false;
                        if(f.type=="boolean")changed=ImGui::Checkbox(f.name.c_str(),&f.boolean);
                        else if(f.type=="number")changed=ImGui::InputDouble(f.name.c_str(),&f.number);
                        else {char value[1024];std::snprintf(value,sizeof(value),"%s",f.text.c_str());changed=ImGui::InputText(f.name.c_str(),value,sizeof(value));if(changed)f.text=value;}
                        if(changed){doc.BeginEdit();slot.properties=ScriptSystem::WriteProperties(fields);doc.CommitEdit();}
                    }
                }
                if(!error.empty())ImGui::TextWrapped("Script metadata: %s",error.c_str());
            }
        }
        if(ImGui::Button("Remove behaviour"))remove=i;
        ImGui::PopID();
    }
    if(remove<o.scripts.size()){doc.BeginEdit();o.scripts.erase(o.scripts.begin()+remove);doc.CommitEdit();}
    if(ImGui::Button("Add behaviour")){doc.BeginEdit();uint64_t id=1;for(const auto& slot:o.scripts)id=std::max(id,slot.id+1);o.scripts.push_back({id,"",true,"{}"});doc.CommitEdit();}
}

void DrawParticleEmitter(EditorDocument& doc,SceneObject& o,EditorPanelState& state){
    auto& e=*o.particleEmitter;
    Checkbox(doc,"Enabled",e.enabled);Checkbox(doc,"Continuous/loop",e.loop);
    Checkbox(doc,"Local space",e.localSpace);Checkbox(doc,"Sample Judas gravity",e.useGravity);
    DragScalar(doc,"Rate / second",e.rate,1,0,100000);DragScalar(doc,"Lifetime seconds",e.lifetime,0.05f,0.01f,3600);
    DragScalar(doc,"Start size",e.size,0.01f,0,1000);DragScalar(doc,"End size",e.endSize,0.01f,0,1000);
    auto integer=[&](const char* name,int& v,int lo,int hi){int next=v;if(ImGui::DragInt(name,&next,1,lo,hi)){doc.BeginEdit();v=std::clamp(next,lo,hi);doc.CommitEdit();}};
    integer("Capacity",e.maxParticles,1,100000);integer("Startup burst",e.burst,0,e.maxParticles);
    DragVec3(doc,"Position spread",e.spread,0.01f);DragVec3(doc,"Initial velocity",e.velocity,0.05f);
    DragVec3(doc,"Velocity variation",e.velocityVariation,0.05f);DragVec3(doc,"Acceleration",e.acceleration,0.05f);
    auto color=[&](const char* label,glm::vec4& v){auto next=v;if(ImGui::ColorEdit4(label,&next.x)){doc.BeginEdit();v=next;doc.CommitEdit();}};
    color("Start color/alpha",e.color);color("End color/alpha",e.endColor);
    unsigned seed=e.seed;if(ImGui::InputScalar("Seed",ImGuiDataType_U32,&seed)){doc.BeginEdit();e.seed=seed;doc.CommitEdit();}
    AssetField(doc,"Particle texture",e.textureAsset,AssetType::Texture,true,state);
}

void DrawAudioEmitter(EditorDocument& doc,SceneObject& o,EditorPanelState& state){
    auto& a=*o.audioEmitter;
    AssetField(doc,"Audio clip",a.asset,AssetType::Audio,true,state);
    Checkbox(doc,"Enabled",a.enabled);Checkbox(doc,"Play on start",a.playOnStart);
    Checkbox(doc,"Loop",a.loop);Checkbox(doc,"Spatial (3D)",a.spatial);
    DragScalar(doc,"Volume",a.volume,.01f,0,1);DragScalar(doc,"Pitch",a.pitch,.01f,.125f,8);
    if(a.spatial){
        const char* const models[]={"None","Inverse distance","Linear distance"};
        Combo(doc,"Attenuation",a.attenuation,models,3);
        DragScalar(doc,"Reference distance",a.referenceDistance,.1f,.001f,a.maximumDistance-.001f);
        DragScalar(doc,"Maximum distance",a.maximumDistance,.1f,a.referenceDistance+.001f,100000);
        DragScalar(doc,"Rolloff",a.rolloff,.01f,0,100);
    }
}
void DrawAudioListener(EditorDocument& doc,SceneObject& o,EditorPanelState&){
    auto& l=*o.audioListener;Checkbox(doc,"Enabled",l.enabled);Checkbox(doc,"Follow active camera view",l.followActiveView);
    ImGui::TextDisabled("One enabled listener per scene. Local -Z forward / +Y up.");
}

void DrawRenderCamera(EditorDocument& doc, SceneObject& o, EditorPanelState& state) {
    auto& c = *o.renderCamera;
    if(state.project)DrawCategoryMask(doc,"Render mask",c.renderMask,state.project->Settings().classification.render);
    Checkbox(doc, "Enabled", c.enabled);
    DragInt(doc, "Target width", c.width, 1, 4096);
    DragInt(doc, "Target height", c.height, 1, 4096);
    DragInt(doc, "Every N rendered frames", c.updateEveryFrames, 1, 100000);
    DragScalar(doc, "Vertical FOV (deg)", c.verticalFovDegrees, 0.1f, 1.0f, 178.0f);
    DragScalar(doc, "Near plane", c.nearPlane, 0.01f, 0.001f, 100000.0f);
    DragScalar(doc, "Far plane", c.farPlane, 1.0f, 0.002f, 1000000.0f);
    ImGui::TextDisabled("Local -Z forward / +Y up. Live output in Play.");
}

void DrawRender(EditorDocument& doc, SceneObject& o, EditorPanelState& state) {
    SceneRenderComponent& r = *o.render;
    Combo(doc, "Shape", r.shape, kShapeNames, 5);
    if (r.shape == SceneShape::Box) DragVec3(doc, "Half extents", r.halfExtents, 0.01f);
    if (r.shape == SceneShape::Sphere) DragScalar(doc, "Radius", r.radius, 0.01f, 0.001f, 100000.0f);
    ColorEdit(doc, "Color", r.color);
    DragScalar(doc, "Alpha", r.alpha, 0.01f, 0.0f, 1.0f);
    if (r.shape == SceneShape::Box || r.shape == SceneShape::Sphere || r.shape == SceneShape::Mesh) {
        const auto* selected = doc.GetScene().Find(r.textureCamera);
        const std::string label = selected ? selected->name : (r.textureCamera ? "(missing camera)" : "(disk/untextured)");
        if (ImGui::BeginCombo("Camera texture", label.c_str())) {
            if (ImGui::Selectable("(disk/untextured)", r.textureCamera == 0)) {
                doc.BeginEdit(); r.textureCamera = 0; doc.CommitEdit();
            }
            for (const auto& camera : doc.GetScene().Objects()) {
                if (!camera.renderCamera) continue;
                ImGui::PushID(static_cast<int>(camera.id));
                if (ImGui::Selectable(camera.name.c_str(), r.textureCamera == camera.id)) {
                    doc.BeginEdit(); r.textureCamera = camera.id; r.textureAsset.clear(); doc.CommitEdit();
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }
    if (r.shape == SceneShape::Mesh) {
        AssetField(doc, "Mesh", r.meshAsset, AssetType::Mesh, false, state);
        if (!r.textureCamera) AssetField(doc, "Texture", r.textureAsset, AssetType::Texture, true, state);
        ImGui::TextDisabled("Drag an asset from the Asset Browser onto a field.");
    }
    if (r.shape == SceneShape::Compound) {
        ColorEdit(doc, "Wall color", r.secondaryColor);
        DragScalar(doc, "Wall alpha", r.secondaryAlpha, 0.01f, 0.0f, 1.0f);
        ImGui::TextDisabled("Geometry comes from the compound body.");
    }
    if (r.shape == SceneShape::Terrain) ImGui::TextDisabled("Geometry comes from the terrain body.");
}

void DrawAnimation(EditorDocument& doc,SceneObject& object,EditorPanelState&){
    auto& a=*object.animation;Checkbox(doc,"Animation enabled",a.enabled);Checkbox(doc,"Play on start",a.playOnStart);Checkbox(doc,"Loop clip",a.loop);
    TextField(doc,"Clip name (empty = first)",a.clip);DragScalar(doc,"Playback speed",a.speed);DragScalar(doc,"Start time (seconds)",a.time,.05f,0,100000);
    if(ImGui::Button("Add pose layer")&&a.layers.size()<16){doc.BeginEdit();AnimationLayerSettings l;l.id="Layer "+std::to_string(a.layers.size()+1);a.layers.push_back(l);doc.CommitEdit();}
    for(size_t i=0;i<a.layers.size();++i){ImGui::PushID(int(i));auto& l=a.layers[i];if(ImGui::TreeNode(l.id.c_str())){
        TextField(doc,"Layer ID",l.id);TextField(doc,"Clip",l.clip);Checkbox(doc,"Enabled",l.enabled);Checkbox(doc,"Additive",l.additive);DragScalar(doc,"Weight",l.weight,.01f,0,1);DragScalar(doc,"Speed",l.speed);DragScalar(doc,"Time",l.time,.05f,0,100000);TextField(doc,"Reference clip (empty = rest)",l.referenceClip);DragScalar(doc,"Reference time",l.referenceTime,.05f,0,100000);
        if(ImGui::Button("Add masked joint")){doc.BeginEdit();l.mask.push_back("Root");doc.CommitEdit();}for(size_t n=0;n<l.mask.size();++n){ImGui::PushID(int(n));TextField(doc,"Joint path/name",l.mask[n]);ImGui::SameLine();if(ImGui::Button("Remove")){doc.BeginEdit();l.mask.erase(l.mask.begin()+n);doc.CommitEdit();ImGui::PopID();break;}ImGui::PopID();}
        if(ImGui::Button("Remove layer")){doc.BeginEdit();a.layers.erase(a.layers.begin()+i);doc.CommitEdit();ImGui::TreePop();ImGui::PopID();break;}ImGui::TreePop();}ImGui::PopID();}
    ImGui::TextDisabled("Layers are resolved in list order; empty mask affects all joints.");
    ImGui::TextDisabled("Use a self-contained GLB/glTF mesh. Pose is independent of playback.");
    if(!object.render||object.render->shape!=SceneShape::Mesh)ImGui::TextColored(ImVec4(1,.3f,.2f,1),"Requires a mesh Render component");
}

void DrawRagdoll(EditorDocument& doc,SceneObject& object,EditorPanelState& state){
    auto frame=[&](const char* label,glm::quat& q){auto degrees=glm::degrees(glm::eulerAngles(glm::normalize(q)));if(ImGui::DragFloat3(label,&degrees.x,.5f))q=glm::normalize(glm::quat(glm::radians(degrees)));TrackEdit(doc);};
    auto& r=*object.ragdoll;Checkbox(doc,"Ragdoll enabled",r.enabled);Checkbox(doc,"Start in ragdoll",r.playOnStart);Checkbox(doc,"Self collision",r.selfCollision);
    ImGui::TextWrapped("Joint keys are imported hierarchy paths or unique names. Parent mapping must precede children. Bodies use normal physics; no pose motors. Positive uniform scales only.");
    if(ImGui::Button("Add mapped bone")&&r.bones.size()<32){doc.BeginEdit();RagdollBone b;b.joint="Root";r.bones.push_back(b);doc.CommitEdit();}
    size_t remove=r.bones.size();
    for(size_t i=0;i<r.bones.size();++i){auto& b=r.bones[i];ImGui::PushID(int(i));if(ImGui::TreeNode("Mapped bone","%s",b.joint.c_str())){
        TextField(doc,"Skeleton joint",b.joint);TextField(doc,"Physical parent key",b.parent);const char* shapes[]={"Box","Sphere"};Combo(doc,"Shape",b.shape,shapes,2);
        DragVec3(doc,"Shape offset",b.offset,.01f);frame("Shape orientation",b.orientation);DragVec3(doc,"Half extents",b.halfExtents,.01f);DragScalar(doc,"Radius",b.radius,.01f);DragScalar(doc,"Mass",b.mass,.1f);DragScalar(doc,"Friction",b.friction,.01f);DragScalar(doc,"Restitution",b.restitution,.01f);
        if(state.project){DrawCategoryLayer(doc,"Collision layer",b.collisionLayer,state.project->Settings().classification.collision);DrawCategoryMask(doc,"Collision mask",b.collisionMask,state.project->Settings().classification.collision);}
        Checkbox(doc,"Suppress parent collision",b.suppressParentCollision);Checkbox(doc,"Capture anchors from current pose",b.autoAnchors);
        const char* types[]={"Fixed","Hinge","Ball","Slider"};Combo(doc,"Constraint",b.constraint.type,types,4);Checkbox(doc,"Constraint enabled",b.constraint.enabled);Checkbox(doc,"Limits",b.constraint.limits);DragScalar(doc,"Lower",b.constraint.lower,.01f);DragScalar(doc,"Upper",b.constraint.upper,.01f);
        if(!b.autoAnchors){DragVec3(doc,"Child anchor",b.constraint.anchorA,.01f);DragVec3(doc,"Parent anchor",b.constraint.anchorB,.01f);}
        frame("Child frame",b.constraint.frameA);frame("Parent frame",b.constraint.frameB);
        if(ImGui::Button("Remove mapping"))remove=i;
        ImGui::TreePop();}ImGui::PopID();
    }
    if(remove<r.bones.size()){doc.BeginEdit();r.bones.erase(r.bones.begin()+remove);doc.CommitEdit();}
    std::string error;if(!ValidRagdollDefinition(r,error))ImGui::TextColored(ImVec4(1,.3f,.2f,1),"%s",error.c_str());
}

void DrawJoint(EditorDocument& doc, SceneObject& object, EditorPanelState&) {
    auto& joint=*object.joint;auto& settings=joint.settings;
    const char* names[]={"Fixed","Hinge","Ball/socket","Slider"};
    Combo(doc,"Joint type",settings.type,names,4);
    auto bodyField=[&](const char* label,SceneObjectId& id,bool world){
        const auto* body=doc.GetScene().Find(id);const std::string preview=body?body->name:world&&id==0?"World anchor":"Select body";
        if(ImGui::BeginCombo(label,preview.c_str())){
            if(world&&ImGui::Selectable("World anchor",id==0)){doc.BeginEdit();id=0;doc.CommitEdit();}
            for(const auto& candidate:doc.GetScene().Objects())if(candidate.body&&ImGui::Selectable((candidate.name+" ##"+std::to_string(candidate.id)).c_str(),candidate.id==id)){doc.BeginEdit();id=candidate.id;doc.CommitEdit();}
            ImGui::EndCombo();
        }
    };
    bodyField("Body A",joint.bodyA,false);bodyField("Body B",joint.bodyB,true);
    DragVec3(doc,"Anchor A (body local)",settings.anchorA);
    DragVec3(doc,joint.bodyB?"Anchor B (body local)":"Anchor B (owner local)",settings.anchorB);
    auto frame=[&](const char* label,glm::quat& q){auto degrees=glm::degrees(glm::eulerAngles(glm::normalize(q)));if(ImGui::DragFloat3(label,&degrees.x,.5f))q=glm::normalize(glm::quat(glm::radians(degrees)));TrackEdit(doc);};
    frame("Frame A (degrees)",settings.frameA);frame(joint.bodyB?"Frame B (degrees)":"World frame (owner local)",settings.frameB);
    Checkbox(doc,"Joint enabled",settings.enabled);
    ImGui::TextDisabled("Local frame X is the hinge/slider axis. Hinge values are radians.");
    if(settings.type==JointType::Hinge||settings.type==JointType::Slider){
        Checkbox(doc,"Limits",settings.limits);DragScalar(doc,"Lower",settings.lower);DragScalar(doc,"Upper",settings.upper);
        Checkbox(doc,"Motor",settings.motor);DragScalar(doc,"Target speed",settings.speed);
        DragScalar(doc,"Maximum force / torque",settings.maxForce,.1f,0,100000);
        Checkbox(doc,"Spring",settings.spring);DragScalar(doc,"Rest coordinate",settings.rest);
        DragScalar(doc,"Stiffness",settings.stiffness,.1f,0,100000);DragScalar(doc,"Damping",settings.damping,.1f,0,100000);
    }
    if(!ValidJointSettings(settings)||joint.bodyA==0||joint.bodyA==joint.bodyB)ImGui::TextColored(ImVec4(1,.3f,.2f,1),"Invalid joint settings or body references");
}

void DrawBody(EditorDocument& doc, SceneObject& o, EditorPanelState& state) {
    SceneBodyComponent& b = *o.body;
    Checkbox(doc,"Sensor (events, no response)",b.sensor);
    Checkbox(doc,"Collider enabled",b.enabled);
    if(state.project){DrawCategoryLayer(doc,"Collision layer",b.collisionLayer,state.project->Settings().classification.collision);DrawCategoryMask(doc,"Collision mask",b.collisionMask,state.project->Settings().classification.collision);}
    Combo(doc, "Motion", b.motion, kMotionNames, 2);
    Combo(doc, "Collider", b.shape, kBodyShapeNames, 5);
    if (b.shape == SceneShape::Box) DragVec3(doc, "Half extents##body", b.halfExtents, 0.01f);
    if (b.shape == SceneShape::Sphere) DragScalar(doc, "Radius##body", b.radius, 0.01f, 0.001f, 100000.0f);
    if (b.shape == SceneShape::Terrain) StringCombo(doc, "Surface", b.terrainSurface, state.terrainSurfaces, false);
    if (b.shape == SceneShape::Compound) {
        ImGui::Text("%zu child boxes", b.compoundBoxes.size());
        for (std::size_t i = 0; i < b.compoundBoxes.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            DragVec3(doc, "Center", b.compoundBoxes[i].localCenter, 0.005f);
            DragVec3(doc, "Half extents", b.compoundBoxes[i].halfExtents, 0.005f);
            ImGui::PopID();
        }
        if (ImGui::SmallButton("Add child box")) {
            doc.BeginEdit();
            b.compoundBoxes.push_back(CompoundBox{glm::vec3(0.0f), glm::vec3(0.1f)});
            doc.CommitEdit();
        }
    }
    ImGui::Text("%zu fluid cavities (body-local boxes)", b.fluidCavities.size());
    ImGui::PushID("fluid-cavities");
    for (std::size_t i = 0; i < b.fluidCavities.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto& cavity = b.fluidCavities[i];
        DragVec3(doc, "Interior center", cavity.localCenter, 0.005f);
        DragScalar(doc, "Interior half X", cavity.halfExtents.x, 0.005f, 0.001f, 100000.0f);
        DragScalar(doc, "Interior half Y", cavity.halfExtents.y, 0.005f, 0.001f, 100000.0f);
        DragScalar(doc, "Interior half Z", cavity.halfExtents.z, 0.005f, 0.001f, 100000.0f);
        const bool remove = ImGui::SmallButton("Remove cavity");
        ImGui::PopID();
        if (remove) {
            doc.BeginEdit();
            b.fluidCavities.erase(b.fluidCavities.begin() + static_cast<std::ptrdiff_t>(i));
            doc.CommitEdit();
            break;
        }
    }
    if (ImGui::SmallButton("Add fluid cavity")) {
        doc.BeginEdit();
        b.fluidCavities.emplace_back();
        doc.CommitEdit();
    }
    ImGui::PopID();
    if (!b.fluidCavities.empty()) ImGui::TextDisabled("Bounds must describe the resolved interior, up to its opening.");
    if (b.motion == SceneBodyMotion::Dynamic) {
        DragScalar(doc, "Mass (kg)", b.mass, 0.1f, 0.001f, 1.0e30f);
        DragVec3(doc, "Initial velocity", b.initialLinearVelocity, 0.05f);
        Checkbox(doc, "Pickable (G/H)", b.pickable);
        Checkbox(doc, "Managed by fidelity policy (M29)", b.managed);
        if (!b.managed) ImGui::TextDisabled("Unmanaged: always fully simulated; the policy never touches it.");
    }
    DragScalar(doc, "Friction", b.friction, 0.01f, 0.0f, 5.0f);
    DragScalar(doc, "Restitution", b.restitution, 0.01f, 0.0f, 1.0f);
}

void DrawGravity(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneGravityComponent& g = *o.gravity;
    Combo(doc, "Kind", g.kind, kGravityKindNames, 2);
    DragScalar(doc, "Magnitude (m/s^2)", g.magnitude, 0.01f, 0.0f, 1000.0f);
    Combo(doc, "Region", g.regionShape, kRegionNames, 2);
    if (g.regionShape == SceneRegionShape::Sphere) DragScalar(doc, "Region radius", g.regionRadius, 0.1f, 0.0f, 1.0e6f);
    else DragVec3(doc, "Region half extents", g.regionHalfExtents, 0.1f);
    ImGui::TextDisabled(g.kind == SceneGravityKind::Radial ? "Pulls toward this object's position."
                                                            : "Pulls along this object's local -Y.");
    ImGui::TextDisabled("Earlier objects win where regions overlap.");
}

void DrawLight(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneLightComponent& l = *o.light;
    Combo(doc, "Kind##light", l.kind, kLightKindNames, 2);
    ColorEdit(doc, "Color##light", l.color);
    DragScalar(doc, "Range", l.range, 0.1f, 0.0f, 10000.0f);
    if (l.kind == SceneLightKind::Spot) {
        DragScalar(doc, "Inner cone (deg)", l.innerConeDegrees, 0.5f, 0.0f, 89.0f);
        DragScalar(doc, "Outer cone (deg)", l.outerConeDegrees, 0.5f, 0.0f, 89.0f);
        ImGui::TextDisabled("A spot light faces this object's local -Z.");
    }
}

void DrawDoor(EditorDocument& doc, SceneObject& o, EditorPanelState& state) {
    if(state.project){auto& d=*o.door;DrawCategoryLayer(doc,"Collision layer",d.collisionLayer,state.project->Settings().classification.collision);DrawCategoryMask(doc,"Collision mask",d.collisionMask,state.project->Settings().classification.collision);}
    DragVec3(doc, "Hinge axis (local)", o.door->localHingeAxis, 0.01f);
    DragScalar(doc, "Open angle (deg)", o.door->openAngleDegrees, 0.5f, 0.0f, 180.0f);
    DragScalar(doc, "Angular speed (deg/s)", o.door->angularSpeedDegreesPerSecond, 1.0f, 0.0f, 3600.0f);
    ImGui::TextDisabled("Needs a box Render for the panel; hinge at the object's transform.");
}

void DrawLightSwitch(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneLightSwitchComponent& s = *o.lightSwitch;
    DragVec3(doc, "Hinge axis (local)##sw", s.localHingeAxis, 0.01f);
    DragScalar(doc, "Toggle angle (deg)", s.toggleAngleDegrees, 0.5f, 0.0f, 180.0f);
    DragScalar(doc, "Angular speed (deg/s)##sw", s.angularSpeedDegreesPerSecond, 1.0f, 0.0f, 3600.0f);
    DragVec3(doc, "Lamp offset (local)", s.lampLocalOffset, 0.05f);
    ColorEdit(doc, "Lamp color", s.lampColor);
    DragScalar(doc, "Lamp range", s.lampRange, 0.1f, 0.0f, 1000.0f);
}

void DrawVehicle(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneVehicleComponent& v = *o.vehicle;
    Combo(doc, "Gravity source", v.gravity, kVehicleGravityNames, 2);
    Checkbox(doc, "Headlight", v.headlight);
    Checkbox(doc, "Navigation lights", v.navigationLights);
    DragScalar(doc, "Drag coefficient", v.dragCoefficient, 0.01f, 0.0f, 10.0f);
    Checkbox(doc, "Start with pilot attached", v.initialPilotAttached);
    ImGui::TextDisabled("Needs a dynamic box Body. One vehicle per scene.");
}

void DrawCelestial(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    DragScalar(doc, "Gravitational parameter (static, m^3/s^2)", o.celestial->gravitationalParameter, 10.0f, 0.0f, 1.0e30f);
    DragScalar(doc, "Operator thrust (N)", o.celestial->operatorThrustForce, 1.0e9f, 0.0f, 1.0e30f);
    ImGui::TextDisabled("Dynamic: joins pairwise Newtonian gravity by its mass.");
}

void DrawAtmosphere(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneAtmosphereComponent& a = *o.atmosphere;
    DragScalar(doc, "Reference radius (m)", a.referenceRadius, 0.1f, 0.001f, 1.0e9f);
    DragScalar(doc, "Top radius (m)", a.topRadius, 0.1f, 0.001f, 1.0e9f);
    DragScalar(doc, "Reference density (kg/m^3)", a.referenceDensity, 0.001f, 0.0f, 1000.0f);
    DragScalar(doc, "Polytropic exponent", a.polytropicExponent, 0.001f, 1.001f, 1.999f);
    DragScalar(doc, "Oxidizer mass fraction", a.oxidizerMassFraction, 0.001f, 0.0f, 1.0f);
    DragScalar(doc, "Reference temperature (K)", a.referenceTemperatureKelvin, 1.0f, 1.0f, 10000.0f);
    ImGui::TextDisabled("Needs a static Celestial gravitational parameter.");
}

void DrawCombustible(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneCombustibleComponent& c = *o.combustible;
    DragScalar(doc, "Heat capacity (J/K)", c.heatCapacityJPerK, 1.0f, 0.001f, 1.0e9f);
    DragScalar(doc, "Fuel mass (kg)", c.initialFuelMassKg, 0.001f, 0.0f, 1.0e6f);
    DragScalar(doc, "Ignition temperature (K)", c.ignitionTemperatureK, 1.0f, 0.0f, 10000.0f);
    DragScalar(doc, "Max fuel rate (kg/s)", c.maximumFuelRateKgPerSecond, 0.0001f, 0.0f, 1000.0f);
    DragScalar(doc, "Radiative area (m^2)", c.radiativeAreaSquareMeters, 0.01f, 0.0f, 1.0e6f);
    DragScalar(doc, "Retained heat fraction", c.retainedCombustionHeatFraction, 0.01f, 0.0f, 1.0f);
}

void DrawFluidVolume(EditorDocument& doc, SceneObject& o, EditorPanelState&) {
    SceneFluidVolumeComponent& f = *o.fluidVolume;
    DragScalar(doc, "Particle spacing (m)", f.spacing, 0.001f, 0.001f, 100.0f);
    int count[3] = {f.countX, f.countY, f.countZ};
    if (ImGui::DragInt3("Lattice count", count, 1.0f, 0, 1000)) {
        f.countX = count[0]; f.countY = count[1]; f.countZ = count[2];
    }
    TrackEdit(doc);
    Checkbox(doc, "Emitter (hold B)", f.emitter);
    if (f.emitter) {
        DragVec3(doc, "Emitter offset (local)", f.emitterLocalOffset, 0.05f);
        DragInt(doc, "Max particles", f.maxParticles, 0, 100000);
    }
    ImGui::TextDisabled("Lattice grows along local +Y from the object.");
}

void DrawPlayerStart(EditorDocument& doc, SceneObject& o, EditorPanelState& state) {
    if(state.project){auto& p=*o.playerStart;DrawCategoryLayer(doc,"Collision layer",p.collisionLayer,state.project->Settings().classification.collision);DrawCategoryMask(doc,"Collision mask",p.collisionMask,state.project->Settings().classification.collision);}
    DragScalar(doc, "Yaw (deg)", o.playerStart->yawDegrees, 0.5f, -360.0f, 360.0f);
    Combo(doc, "View", o.playerStart->view, kViewNames, 2);
    DragScalar(doc, "Density (kg/m^3)", o.playerStart->density, 1.0f, 0.001f, 1.0e6f);
    DragScalar(doc, "Fluid drag (1/s)", o.playerStart->fluidDrag, 0.05f, 0.0f, 1000.0f);
    DragScalar(doc, "Swim acceleration (m/s^2)", o.playerStart->swimAcceleration, 0.1f, 0.0f, 1000.0f);
    ImGui::TextDisabled("Exactly one object may carry a player start.");
}

template <typename T>
ComponentEditor Make(const char* name, char indicator, std::optional<T> SceneObject::*member,
                     void (*draw)(EditorDocument&, SceneObject&, EditorPanelState&)) {
    // Function pointers cannot capture, so the member pointer is threaded
    // through a static per-instantiation slot; each component type is a
    // distinct T, so each instantiation has its own slot.
    static std::optional<T> SceneObject::*slot = nullptr;
    slot = member;
    ComponentEditor editor;
    editor.name = name;
    editor.indicator = indicator;
    editor.has = [](const SceneObject& o) { return (o.*slot).has_value(); };
    editor.add = [](SceneObject& o) { o.*slot = T{}; };
    editor.remove = [](SceneObject& o) { (o.*slot).reset(); };
    editor.draw = draw;
    return editor;
}
}  // namespace

const std::vector<ComponentEditor>& ComponentEditorRegistry() {
    static const std::vector<ComponentEditor> registry = {
        Make<SceneUIComponent>("Runtime UI",'U',&SceneObject::ui,DrawUIComponent),
        {"Scripts",'J',[](const SceneObject& o){return !o.scripts.empty();},[](SceneObject& o){o.scripts.push_back({1,"",true,"{}"});},[](SceneObject& o){o.scripts.clear();},DrawScripts},
        Make<ParticleEmitterSettings>("Particle emitter", 'E', &SceneObject::particleEmitter, DrawParticleEmitter),
        Make<SceneAudioEmitterComponent>("Audio emitter", 'U', &SceneObject::audioEmitter, DrawAudioEmitter),
        Make<SceneAudioListenerComponent>("Audio listener", 'N', &SceneObject::audioListener, DrawAudioListener),
        Make<SceneRenderCameraComponent>("Render camera", 'K', &SceneObject::renderCamera, DrawRenderCamera),
        Make<SceneRenderComponent>("Render", 'R', &SceneObject::render, DrawRender),
        Make<SceneAnimationComponent>("Animation",'A',&SceneObject::animation,DrawAnimation),
        Make<RagdollDefinition>("Ragdoll",'R',&SceneObject::ragdoll,DrawRagdoll),
        Make<SceneJointComponent>("Joint",'J',&SceneObject::joint,DrawJoint),
        Make<SceneBodyComponent>("Body", 'B', &SceneObject::body, DrawBody),
        Make<SceneGravityComponent>("Gravity region", 'G', &SceneObject::gravity, DrawGravity),
        Make<SceneLightComponent>("Light", 'L', &SceneObject::light, DrawLight),
        Make<SceneDoorComponent>("Door", 'D', &SceneObject::door, DrawDoor),
        Make<SceneLightSwitchComponent>("Light switch", 'S', &SceneObject::lightSwitch, DrawLightSwitch),
        Make<SceneVehicleComponent>("Vehicle", 'V', &SceneObject::vehicle, DrawVehicle),
        Make<SceneCelestialComponent>("Celestial", 'C', &SceneObject::celestial, DrawCelestial),
        Make<SceneAtmosphereComponent>("Atmosphere", 'A', &SceneObject::atmosphere, DrawAtmosphere),
        Make<SceneCombustibleComponent>("Combustible", 'F', &SceneObject::combustible, DrawCombustible),
        Make<SceneFluidVolumeComponent>("Fluid volume", 'W', &SceneObject::fluidVolume, DrawFluidVolume),
        Make<ScenePlayerStartComponent>("Player start", 'P', &SceneObject::playerStart, DrawPlayerStart),
    };
    return registry;
}

void DrawTransformEditor(EditorDocument& doc, SceneObject& o) {
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) return;
    DragVec3(doc, "Position", o.transform.position);
    // Rotation is edited as yaw/pitch/roll degrees for humans; the
    // authored value stays a quaternion.
    glm::vec3 euler = glm::degrees(glm::eulerAngles(glm::normalize(o.transform.rotation)));
    if (ImGui::DragFloat3("Rotation (deg)", &euler.x, 0.5f, 0.0f, 0.0f, "%.3g")) {
        o.transform.rotation = glm::normalize(glm::quat(glm::radians(euler)));
    }
    TrackEdit(doc);
    DragVec3(doc, "Scale", o.transform.scale, 0.01f);
    ImGui::TextDisabled("Scale applies to mesh rendering only.");
}

std::string ComponentIndicators(const SceneObject& object) {
    std::string out;
    for (const ComponentEditor& editor : ComponentEditorRegistry()) {
        if (editor.has(object)) out += editor.indicator;
    }
    return out;
}

void DrawCategoryLayer(EditorDocument& doc,const char* label,unsigned& value,const CategoryRegistry& registry){
    auto it=registry.names.find(value);const std::string preview=it==registry.names.end()?"[unregistered ID "+std::to_string(value)+"]":it->second;
    if(ImGui::BeginCombo(label,preview.c_str())){
        for(const auto& [id,name]:registry.names)if(ImGui::Selectable(name.c_str(),id==value)){doc.BeginEdit();value=id;doc.CommitEdit();}
        ImGui::EndCombo();
    }
}
void DrawCategoryMask(EditorDocument& doc,const char* label,CategoryMask& mask,const CategoryRegistry& registry,bool allowAll){
    if(ImGui::TreeNode(label)){
        if(allowAll&&ImGui::Button("All (including future layers)")){doc.BeginEdit();mask=kAllCategories;doc.CommitEdit();}
        if(ImGui::Button("None")){doc.BeginEdit();mask=0;doc.CommitEdit();}
        for(const auto& [id,name]:registry.names){bool selected=(mask&CategoryBit(id))!=0;ImGui::PushID(int(id));
            if(ImGui::Checkbox(name.c_str(),&selected)){doc.BeginEdit();if(selected)mask|=CategoryBit(id);else mask&=~CategoryBit(id);doc.CommitEdit();}
            ImGui::PopID();
        }
        if(!allowAll&&(mask&~registry.ActiveMask()))ImGui::TextColored(ImVec4(1,.3f,.2f,1),"Contains retired/unregistered tags");
        ImGui::TreePop();
    }
}
