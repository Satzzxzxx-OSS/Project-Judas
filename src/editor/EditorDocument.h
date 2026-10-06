#pragma once

#include <string>
#include <vector>

#include "Scene.h"

// Milestone 28: the editor's view of one authored Scene — the file it
// came from, whether it has unsaved changes, the current selection, and a
// snapshot-based undo/redo history. Every edit the panels make goes
// through BeginEdit/CommitEdit so history and the dirty flag stay exact.
//
// Undo stores whole-Scene snapshots. Scenes are small authored documents
// (tens of objects, a few hundred numbers), so a copy per committed edit
// is cheap, trivially correct, and covers creation, deletion, reordering,
// component add/remove and settings changes with one mechanism — no
// per-property command classes to keep in sync with the data model.
class EditorDocument {
public:
    Scene& GetScene() { return m_scene; }
    const Scene& GetScene() const { return m_scene; }

    const std::string& Path() const { return m_path; }
    bool IsDirty() const { return m_dirty; }
    SceneObjectId Selected() const { return m_selected; }
    void Select(SceneObjectId id,bool toggle=false);
    const std::vector<SceneObjectId>& Selection() const {return m_selection;}
    bool IsSelected(SceneObjectId id)const;
    void PruneSelection();
    bool BatchProperties(const std::map<std::string,std::string>&,std::string& error);
    bool BatchTransform(glm::vec3 translation,glm::quat rotation,glm::vec3 scale,std::string& error);
    bool ReparentSelection(SceneObjectId parent,std::string& error);
    bool GroupSelection(std::string& error);
    bool DuplicateSelection(std::string& error);
    void CopyComponent(const std::string& prefix);
    bool PasteComponent(std::string& error);
    SceneObject* SelectedObject() { return m_scene.Find(m_selected); }

    void NewScene();
    bool Load(const std::string& path, std::string& outError);
    bool Save(std::string& outError);            // to Path()
    bool SaveAs(const std::string& path, std::string& outError);

    // An edit is a snapshot taken before the change and confirmed after
    // it. CancelEdit discards the snapshot (a widget activated but its
    // value never changed).
    void BeginEdit();
    void CommitEdit(bool capturePrefab = true);
    void CancelEdit();
    bool EditInProgress() const { return m_editInProgress; }

    bool CanUndo() const { return !m_undo.empty(); }
    bool CanRedo() const { return !m_redo.empty(); }
    void Undo();
    void Redo();

private:
    Scene m_scene;
    std::string m_path;
    bool m_dirty = false;
    std::vector<SceneObjectId> m_selection;
    std::map<std::string,std::string> m_componentClipboard;
    std::string m_clipboardPrefix;
    SceneObjectId m_selected = kInvalidSceneObjectId;
    std::vector<Scene> m_undo;
    std::vector<Scene> m_redo;
    Scene m_pendingSnapshot;
    bool m_editInProgress = false;
};
