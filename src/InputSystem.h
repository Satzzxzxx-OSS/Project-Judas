#pragma once
#include <map>
#include <string>
#include <vector>

// Portable control tokens (key:W, mouse:Left, pad:South, stick:LeftX, ...).
// SDL identifiers belong exclusively to Window's raw-device adapter.
struct InputBinding {
    std::string control;
    float scale=1, deadzone=0;
};
struct InputEntry {
    std::string name;
    bool axis=false;
    std::vector<InputBinding> bindings;
};
struct InputMap {
    std::vector<InputEntry> entries;
    static InputMap Defaults();
    bool Validate(std::string& error) const;
    std::string Serialize() const;
    static bool Parse(const std::string& text,InputMap& out,std::string& error);
    InputEntry* Find(const std::string& name);
    const InputEntry* Find(const std::string& name) const;
    bool Add(const std::string& name,bool axis);
    bool Rename(const std::string& from,const std::string& to);
    bool Remove(const std::string& name);
    bool AddBinding(const std::string& name,InputBinding binding);
    bool ReplaceBinding(const std::string& name,std::size_t index,InputBinding binding);
    bool RemoveBinding(const std::string& name,std::size_t index);
};
struct InputActionState { bool held=false,pressed=false,released=false; };
class InputSystem {
public:
    InputSystem();
    bool SetMap(const InputMap& map,std::string& error);
    const InputMap& Map() const {return m_map;}
    void BeginFrame();
    void SetPhysical(const std::string& control,float value);
    void AddDelta(const std::string& control,float delta);
    void ClearDevice(const std::string& prefix);
    void Reset();
    void DiscardPending();
    InputActionState Action(const std::string& name) const;
    float Axis(const std::string& name) const;
    // All consumers see the same snapshot. Edges accumulate across zero-step
    // frames; first fixed step drains them, later catch-up steps see no repeat.
    void BeginFixedStep() const;
    InputActionState FixedAction(const std::string& name) const;
    static float Deadzone(float value,float deadzone);
private:
    void Evaluate();
    InputMap m_map;
    std::map<std::string,float> m_raw,m_axes;
    std::map<std::string,InputActionState> m_actions;
    mutable std::map<std::string,InputActionState> m_pending,m_fixed;
};
