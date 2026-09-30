#pragma once
#include "RigidBody.h"
#include <vector>
#include <stdexcept>

// Step-local authoritative motion. Observations never restart the integrator.
// The owner is the full slot/generation handle, not a pointer into body storage.
struct RigidMotionSegment {
    unsigned int owner = 0;
    double begin = 0, end = 0;
    float inverseMass = 0;
    glm::vec3 position{0}, linearVelocity{0}, angularVelocity{0};
    glm::quat orientation{1,0,0,0};
    glm::vec3 endPosition{0};
    glm::quat endOrientation{1,0,0,0};
    RigidBody Evaluate(double time) const {
        if (time < begin || time > end) throw std::out_of_range("motion segment time");
        RigidBody result;
        result.inverseMass = inverseMass;
        result.position = position; result.orientation = orientation;
        result.linearVelocity = linearVelocity; result.angularVelocity = angularVelocity;
        // Call the original arithmetic from a fixed anchor: full no-impact dt
        // has exactly the original binary32 representation and arithmetic order.
        IntegrateRigidBodyPosition(result, static_cast<float>(time - begin));
        return result;
    }
};
class RigidMotion {
public:
    void Begin(unsigned int owner, const RigidBody& body, double duration) {
        segments.clear(); Append(owner, body, 0, duration);
    }
    RigidBody Evaluate(double time) const {
        for (const auto& segment : segments)
            if (time >= segment.begin && time <= segment.end) return segment.Evaluate(time);
        throw std::out_of_range("motion ledger time");
    }
    void ChangeVelocity(const RigidBody& body, double time) {
        if (segments.empty()) throw std::logic_error("motion ledger not started");
        auto& previous = segments.back();
        const double finish = previous.end;
        const auto pose = previous.Evaluate(time);
        previous.end = time; previous.endPosition = pose.position; previous.endOrientation = pose.orientation;
        RigidBody next = body; next.position = pose.position; next.orientation = pose.orientation;
        const auto owner = previous.owner;
        Append(owner, next, time, finish);
    }
    void Clear() { segments.clear(); }
    std::size_t StorageBytes() const { return segments.capacity()*sizeof(RigidMotionSegment); }
    const std::vector<RigidMotionSegment>& Segments() const { return segments; }
private:
    void Append(unsigned int owner, const RigidBody& body, double begin, double end) {
        RigidMotionSegment s;
        s.inverseMass=body.inverseMass; s.owner=owner; s.begin=begin; s.end=end; s.position=body.position;
        s.orientation=body.orientation; s.linearVelocity=body.linearVelocity; s.angularVelocity=body.angularVelocity;
        const auto pose=s.Evaluate(end); s.endPosition=pose.position; s.endOrientation=pose.orientation;
        segments.push_back(s);
    }
    std::vector<RigidMotionSegment> segments;
};
