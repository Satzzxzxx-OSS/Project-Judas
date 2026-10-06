#include "ScriptSystem.h"
#include "Ragdoll.h"
#include <algorithm>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include "EditorDocument.h"

#include "SceneSerialization.h"
#include "Prefab.h"

namespace {
constexpr std::size_t kMaxHistory = 200;
bool ComponentKey(const std::string& key,const std::string& prefix){
 if(key==prefix||key.rfind(prefix+".",0)==0)return true;
 return (prefix=="audio"&&key=="audio-emitter")||(prefix=="scripts"&&key.rfind("script.",0)==0);
}
}

void EditorDocument::NewScene() {
    m_scene.Clear();
    m_scene.Settings().name = "Untitled scene";
    m_path.clear();
    m_dirty = false;
    m_selected = kInvalidSceneObjectId;m_selection.clear();
    m_undo.clear();
    m_redo.clear();
    m_editInProgress = false;
}

bool EditorDocument::Load(const std::string& path, std::string& outError) {
    Scene loaded;
    if (!LoadSceneFromFile(path, loaded, outError)) return false;
    m_scene = std::move(loaded);
    m_path = path;
    m_dirty = false;
    m_selected = kInvalidSceneObjectId;m_selection.clear();
    m_undo.clear();
    m_redo.clear();
    m_editInProgress = false;
    return true;
}

bool EditorDocument::Save(std::string& outError) {
    if (m_path.empty()) {
        outError = "the scene has no file path yet; use Save As";
        return false;
    }
    if (!SaveSceneToFile(m_scene, m_path, outError)) return false;
    m_dirty = false;
    return true;
}

bool EditorDocument::SaveAs(const std::string& path, std::string& outError) {
    if (!SaveSceneToFile(m_scene, path, outError)) return false;
    m_path = path;
    m_dirty = false;
    return true;
}

void EditorDocument::BeginEdit() {
    if (m_editInProgress) return;
    m_pendingSnapshot = m_scene;
    m_editInProgress = true;
}

void EditorDocument::CommitEdit(bool capturePrefab) {
    if (!m_editInProgress) return;
    m_editInProgress = false;
    if(capturePrefab)CapturePrefabEdits(m_pendingSnapshot,m_scene);
    if (ScenesEqual(m_pendingSnapshot, m_scene)) return;
    m_undo.push_back(std::move(m_pendingSnapshot));
    if (m_undo.size() > kMaxHistory) m_undo.erase(m_undo.begin());
    m_redo.clear();
    m_dirty = true;
}

void EditorDocument::CancelEdit() {
    m_editInProgress = false;
}

void EditorDocument::Undo() {
    if (m_undo.empty()) return;
    m_redo.push_back(m_scene);
    m_scene = std::move(m_undo.back());
    m_undo.pop_back();
    m_dirty = true;
    PruneSelection();
}

void EditorDocument::Redo() {
    if (m_redo.empty()) return;
    m_undo.push_back(m_scene);
    m_scene = std::move(m_redo.back());
    m_redo.pop_back();
    m_dirty = true;
    PruneSelection();
}

void EditorDocument::Select(SceneObjectId id,bool toggle){
 if(!toggle){m_selection.clear();if(id&&m_scene.Find(id))m_selection.push_back(id);m_selected=m_selection.empty()?0:id;return;}
 auto it=std::find(m_selection.begin(),m_selection.end(),id);if(it!=m_selection.end())m_selection.erase(it);else if(m_scene.Find(id))m_selection.push_back(id);m_selected=m_selection.empty()?0:m_selection.back();
}
bool EditorDocument::IsSelected(SceneObjectId id)const{return std::find(m_selection.begin(),m_selection.end(),id)!=m_selection.end();}
void EditorDocument::PruneSelection(){m_selection.erase(std::remove_if(m_selection.begin(),m_selection.end(),[&](auto id){return !m_scene.Find(id);}),m_selection.end());if(!IsSelected(m_selected))m_selected=m_selection.empty()?0:m_selection.back();}
bool EditorDocument::BatchProperties(const std::map<std::string,std::string>& properties,std::string& error){
 PruneSelection();if(m_selection.empty()){error="select objects first";return false;}auto candidate=m_scene;for(auto id:m_selection)if(!ApplyObjectProperties(*candidate.Find(id),properties,error))return false;
 if(!ValidateHierarchy(candidate,error))return false;
 BeginEdit();m_scene=std::move(candidate);CommitEdit();return true;
}
bool EditorDocument::BatchTransform(glm::vec3 translation,glm::quat rotation,glm::vec3 scale,std::string& error){
 PruneSelection();auto candidate=m_scene;for(auto id:m_selection){auto& t=candidate.Find(id)->transform;t.position+=translation;t.rotation=glm::normalize(rotation*t.rotation);t.scale*=scale;auto check=ObjectProperties(*candidate.Find(id));if(!ApplyObjectProperties(*candidate.Find(id),check,error))return false;}
 BeginEdit();m_scene=std::move(candidate);CommitEdit();return true;
}
bool EditorDocument::ReparentSelection(SceneObjectId parent,std::string& error){
 PruneSelection();Scene flat;if(!FlattenHierarchy(m_scene,flat,error))return false;auto candidate=m_scene;glm::mat4 inverse(1);if(parent){auto* p=flat.Find(parent);if(!p){error="missing parent";return false;}auto& t=p->transform;inverse=glm::inverse(glm::translate(glm::mat4(1),t.position)*glm::mat4_cast(t.rotation)*glm::scale(glm::mat4(1),t.scale));}
 for(auto id:m_selection){auto* o=candidate.Find(id);if(o->prefabRoot&&o->prefabRoot!=id){error="reparent prefab roots; unpack source members first";return false;}auto& t=flat.Find(id)->transform;JointTransform local;if(!DecomposeRigidPose(inverse*glm::translate(glm::mat4(1),t.position)*glm::mat4_cast(t.rotation)*glm::scale(glm::mat4(1),t.scale),local,error))return false;o->parent=parent;o->transform={local.translation,local.rotation,local.scale};}
 if(!ValidateHierarchy(candidate,error))return false;
 BeginEdit();m_scene=std::move(candidate);CommitEdit();return true;
}
bool EditorDocument::GroupSelection(std::string& error){
 PruneSelection();if(m_selection.empty()){error="select objects before grouping";return false;}
 auto original=m_scene;auto group=m_scene.CreateObject("Group").id;if(!ReparentSelection(group,error)){m_scene=std::move(original);return false;}
 // ReparentSelection records one snapshot; include group creation in that action.
 if(!m_undo.empty())m_undo.back()=std::move(original);
 Select(group);return true;
}
void EditorDocument::CopyComponent(const std::string& prefix){m_componentClipboard.clear();m_clipboardPrefix=prefix;auto* o=SelectedObject();if(!o)return;for(auto& [key,value]:ObjectProperties(*o))if(ComponentKey(key,prefix))m_componentClipboard[key]=value;}
bool EditorDocument::PasteComponent(std::string& error){if(m_componentClipboard.empty()){error="component clipboard is empty";return false;}auto candidate=m_scene;for(auto id:m_selection){auto* o=candidate.Find(id);if(!o)continue;auto properties=m_componentClipboard;for(const auto& [key,_]:ObjectProperties(*o))if(ComponentKey(key,m_clipboardPrefix)&&!properties.count(key))properties[key]="@remove";if(!ApplyObjectProperties(*o,properties,error))return false;}if(!ValidateHierarchy(candidate,error))return false;
 BeginEdit();m_scene=std::move(candidate);CommitEdit();return true;}
bool EditorDocument::DuplicateSelection(std::string& error){
 PruneSelection();if(m_selection.empty())return false;std::set<SceneObjectId> members(m_selection.begin(),m_selection.end());bool changed=true;while(changed){changed=false;for(auto& o:m_scene.Objects())if(members.count(o.parent))changed|=members.insert(o.id).second;}
 auto candidate=m_scene;std::map<SceneObjectId,SceneObjectId> ids;for(auto id:members)ids[id]=candidate.CreateObject("copy").id;
 for(auto& o:m_scene.Objects())if(members.count(o.id)){auto copy=o;copy.id=ids.at(o.id);copy.name+=" copy";auto remap=[&](SceneObjectId& id){if(ids.count(id))id=ids.at(id);};remap(copy.parent);remap(copy.prefabRoot);if(copy.socket)remap(copy.socket->target);if(copy.render)remap(copy.render->textureCamera);if(copy.joint){remap(copy.joint->bodyA);remap(copy.joint->bodyB);}if(copy.liquidConnection){remap(copy.liquidConnection->source);remap(copy.liquidConnection->destination);}if(copy.deformable)for(auto& a:copy.deformable->attachments)remap(a.target);for(auto& slot:copy.scripts)slot.properties=ScriptSystem::RemapPropertyEntities(slot.properties,ids);for(auto& [_,id]:copy.prefabIds)remap(id);*candidate.Find(copy.id)=std::move(copy);}
 if(!ValidateHierarchy(candidate,error))return false;
 BeginEdit();m_scene=std::move(candidate);CommitEdit(false);std::vector<SceneObjectId> selection;for(auto id:m_selection)selection.push_back(ids.at(id));m_selection=selection;m_selected=m_selection.back();return true;
}
