#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

// Optional diagnostics at actual resource operations, never a substitute for
// IO, decoding, upload or state transitions. Installed on the owner thread
// before requests; each worker captures its own copy. Callbacks must be thread
// safe, must not throw, and must not reenter the observed engine operation
// or mutate engine state from workers. Captures must outlive shutdown. Tools
// may pause a worker at these cooperative boundaries to reproduce races.
enum class ResourceTracePoint {
    FileReadBegin, FileReadChunk, FileReadEnd, DecodeBegin, DecodeEnd, LoadComplete,
    CancelRequested, StaleDiscarded, CpuReady, ManagerShutdownBegin, ManagerShutdownEnd,
    MeshCreated, MeshDestroyed, TextureCreated, TextureDestroyed,
    RendererShutdownBegin, RendererShutdownEnd, WorkersJoined, ContextDestroyed
};
struct ResourceTraceEvent {
    ResourceTracePoint point;
    std::string asset;
    std::string path;
    std::uint64_t generation = 0;
    std::size_t bytes = 0;
    unsigned int handle = 0;
    bool contextCurrent = false;
    std::thread::id thread = std::this_thread::get_id();
};
using ResourceTrace = std::function<void(const ResourceTraceEvent&)>;
