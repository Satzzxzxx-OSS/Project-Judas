#pragma once
#include <cstdint>
struct BodyHandle {
    static constexpr unsigned int kInvalidId=0xFFFFFFFFu;
    unsigned int id=kInvalidId;
    bool IsValid()const{return id!=kInvalidId;}
};
// Monotonic lifetime identity; never a raw solver pointer or reusable slot index.
struct JointHandle {std::uint64_t id=0;bool IsValid()const{return id!=0;}};
