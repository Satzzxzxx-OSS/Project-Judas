#pragma once
#include "Project.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
class RuntimeWorld;
class ResourceManager;
class InteractivePlay;
// Project/session lifetime, independent of scene-owned JS heaps and entities.
class SceneSession {
public:
    SceneSession(const Project& project, const std::string& current);
    bool Request(const std::string& scene, std::string& error);
    bool Reload(std::string& error) { return Request(m_current,error); }
    LocalizationSession& Localization(ResourceManager* r){m_localization.Bind(r);return m_localization;}
    const std::string& Current() const { return m_current; }
    const std::vector<std::string>& Scenes() const { return m_scenes; }
    bool Set(const std::string& key,const std::string& json,std::string& error);
    std::string Get(const std::string& key) const;
    void Erase(const std::string& key) { m_values.erase(key); }
    // Called only after Frame/callbacks have returned. Invalid candidates never
    // replace the live world. First accepted request wins until this boundary.
    bool Apply(std::unique_ptr<RuntimeWorld>& world,InteractivePlay& play,
               ResourceManager& resources,std::string& error);
    bool Pending() const { return !m_pending.empty(); }
    void EnableSaves(bool enabled) { m_saves=enabled; }
private:
    Project m_project;
    LocalizationSession m_localization;
    std::string m_current,m_pending;
    std::vector<std::string> m_scenes;
    std::map<std::string,std::string> m_values;
    bool m_saves=true,m_accepting=true;
};
