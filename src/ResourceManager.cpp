#include "ResourceManager.h"

#include <algorithm>
#include <limits>

#include "AsyncFile.h"
#include "ModelLoader.h"
#include "SkeletalAnimation.h"
#include "RadialTerrain.h"
#include "TextureLoader.h"

const char* ResourceStateName(ResourceState state) {
    switch (state) {
        case ResourceState::Unloaded: return "unloaded";
        case ResourceState::Queued: return "queued";
        case ResourceState::Loading: return "loading";
        case ResourceState::CpuReady: return "cpu-ready";
        case ResourceState::Ready: return "ready";
        case ResourceState::Failed: return "failed";
        case ResourceState::Cancelled: return "cancelled";
    }
    return "?";
}

ResourceManager::ResourceManager(Renderer* renderer, const AssetDatabase* assets, JobSystem* jobs, AudioSystem* audio)
    : m_audio(audio), m_renderer(renderer), m_assets(assets), m_jobs(jobs), m_ownerThread(std::this_thread::get_id()) {}

ResourceManager::~ResourceManager() {
    Shutdown();
}

std::uint64_t ResourceManager::EstimateMeshBytes(const MeshData& data) {
    uint64_t bytes=static_cast<uint64_t>(data.vertices.size())*sizeof(MeshVertex)+static_cast<uint64_t>(data.skinVertices.size())*sizeof(MeshSkinVertex)+static_cast<uint64_t>(data.indices.size())*sizeof(uint32_t);
    if(data.skeletal){const auto& s=data.skeletal->skeleton;bytes+=s.rest.local.size()*sizeof(JointTransform)+(s.parents.size()+s.order.size()+s.skinNodes.size())*sizeof(int)+s.inverseBind.size()*sizeof(glm::mat4);for(const auto& name:s.names)bytes+=name.size();for(const auto& clip:data.skeletal->clips){bytes+=clip.name.size();for(const auto& track:clip.tracks)bytes+=track.times.size()*sizeof(float)+track.values.size()*sizeof(glm::vec4);}}
    return bytes;
}

std::uint64_t ResourceManager::EstimateTextureBytes(const TextureData& data) {
    // RGBA plus the mipmap chain the renderer generates (~1/3 more).
    const std::uint64_t base = static_cast<std::uint64_t>(data.width) * static_cast<std::uint64_t>(data.height) * 4ull;
    return base + base / 3ull;
}

void ResourceManager::Fail(Entry& entry, const std::string& message) {
    entry.state = ResourceState::Failed;
    entry.error = message;
    entry.task.reset();
}

bool ResourceManager::Resolve(const AssetId& id, AssetType expected, Entry& entry, std::string& outPath) {
    entry.type = expected;
    if (!m_assets) { Fail(entry, "no asset database"); return false; }
    const AssetRecord* record = m_assets->Find(id);
    if (!record) { Fail(entry, "unknown asset id " + id); return false; }
    if (record->missing) { Fail(entry, "asset file is missing: " + record->relativePath); return false; }
    if (record->type != expected) {
        Fail(entry, "asset " + record->relativePath + " is a " + AssetTypeName(record->type) + ", not a " +
                        AssetTypeName(expected));
        return false;
    }
    if (expected == AssetType::Font) { Fail(entry, "fonts are engine-level resources, not scene resources"); return false; }
    outPath = record->path;
    return true;
}

void ResourceManager::TraceTask(const LoadTask& task, ResourceTracePoint point, std::size_t bytes) {
    if (task.trace) task.trace(ResourceTraceEvent{point, task.id, task.path, task.generation, bytes});
}

// The CPU stage: file read + decode. Runs on a worker (async) or on the
// caller (blocking). Touches only the task — never the manager.
void ResourceManager::RunLoadTask(LoadTask& task, const JobContext* context) {
    task.stage.store(1, std::memory_order_release);
    std::vector<std::uint8_t> bytes;
    std::string error;
    bool cancelled = false;
    TraceTask(task, ResourceTracePoint::FileReadBegin);
    const bool read = ReadWholeFile(task.path, bytes, error, context, &cancelled,
        task.trace ? std::function<void(std::size_t)>([&task](std::size_t count) {
            TraceTask(task, ResourceTracePoint::FileReadChunk, count);
        }) : std::function<void(std::size_t)>{});
    TraceTask(task, ResourceTracePoint::FileReadEnd, bytes.size());
    if (!read) {
        task.cancelled = cancelled;
        task.error = cancelled ? std::string() : error;
        task.stage.store(2, std::memory_order_release);
        TraceTask(task, ResourceTracePoint::LoadComplete, bytes.size());
        return;
    }
    if (context && context->CancelRequested()) {
        task.cancelled = true;
        task.stage.store(2, std::memory_order_release);
        TraceTask(task, ResourceTracePoint::LoadComplete, bytes.size());
        return;
    }
    task.decodeThread = std::this_thread::get_id();
    TraceTask(task, ResourceTracePoint::DecodeBegin, bytes.size());
    if (task.type == AssetType::Mesh) {
        task.succeeded = ParseModelMesh(reinterpret_cast<const char*>(bytes.data()), bytes.size(), task.path, task.mesh, task.error);
    } else if(task.type==AssetType::Liquid){task.liquid=std::make_shared<LiquidResource>();task.succeeded=DecodeLiquidResource(bytes,*task.liquid,task.error);
    } else if (task.type == AssetType::Navigation) {
        task.navigation=std::make_shared<NavigationData>();task.succeeded=DecodeNavigation(bytes,*task.navigation,task.error);
    } else if (task.type == AssetType::Audio) {
        task.succeeded = DecodeAudioFromMemory(bytes.data(),bytes.size(),task.audio,task.error);
    } else {
        task.succeeded = DecodeTextureFromMemory(bytes.data(), bytes.size(), task.path, task.texture, task.error);
    }
    TraceTask(task, ResourceTracePoint::DecodeEnd, bytes.size());
    task.stage.store(2, std::memory_order_release);
    TraceTask(task, ResourceTracePoint::LoadComplete, bytes.size());
}

ResourceManager::Entry& ResourceManager::Begin(const AssetId& id, AssetType expected, JobPriority priority) {
    Entry& entry = m_entries[id];
    entry.lastUse = ++m_useClock;
    switch (entry.state) {
        case ResourceState::Ready:
        case ResourceState::Failed:
        case ResourceState::Queued:
        case ResourceState::Loading:
        case ResourceState::CpuReady:
            ++m_stats.hits;  // answered from cache or joined the in-flight load
            return entry;
        case ResourceState::Unloaded:
        case ResourceState::Cancelled:
            break;
    }
    ++m_stats.misses;
    entry.error.clear();
    if (m_shutDown) { Fail(entry, "resource manager is shut down"); return entry; }
    if (expected != AssetType::Navigation && expected != AssetType::Liquid && (expected == AssetType::Audio ? !m_audio : (!m_renderer && !m_headlessResidency))) { Fail(entry, expected == AssetType::Audio ? "no audio system" : "no renderer (headless)"); return entry; }
    std::string path;
    if (!Resolve(id, expected, entry, path)) return entry;

    auto task = std::make_shared<LoadTask>();
    task->id = id;
    task->type = expected;
    task->path = path;
    task->generation = entry.generation;
    task->requested = std::chrono::steady_clock::now();
    task->trace = m_trace;
    entry.task = task;

    if (m_blocking || !m_jobs) {
        RunLoadTask(*task, nullptr);
        CompleteTask(entry, task);
        return entry;
    }
    entry.state = ResourceState::Queued;
    // The job owns its own reference: this manager may release the entry,
    // shut down or be destroyed while the worker is still decoding, and
    // the worker only ever touches the task.
    task->job = m_jobs->Submit([task](JobContext& context) {
        // Cancelled while queued -> never runs; cancelled while running ->
        // the read stops at its next chunk and the decode is skipped.
        RunLoadTask(*task, &context);
        if (task->cancelled) context.ReportCancelled();
        else if (!task->succeeded) context.SetError(task->error);
    }, priority, std::string("load ") + AssetTypeName(expected) + " " + path);
    if (!task->job.IsValid()) {
        Fail(entry, "job system is shut down");
        return entry;
    }
    m_inFlight.push_back(task);
    return entry;
}

// GL thread only: installs a finished task into its entry — or discards
// it when the entry moved on (generation mismatch) or the load was
// cancelled/failed.
void ResourceManager::CompleteTask(Entry& entry, const std::shared_ptr<LoadTask>& task) {
    if (entry.task != task || task->generation != entry.generation) {
        ++m_stats.staleDiscarded;
        TraceTask(*task, ResourceTracePoint::StaleDiscarded);
        return;
    }
    entry.task.reset();
    entry.decodeThread = task->decodeThread;
    if (task->cancelled) {
        entry.state = ResourceState::Cancelled;
        ++m_stats.cancelled;
        return;
    }
    if (!task->succeeded) {
        Fail(entry, task->error.empty() ? std::string("load failed") : task->error);
        return;
    }
    if (task->type != AssetType::Navigation && task->type != AssetType::Liquid && (task->type == AssetType::Audio ? !m_audio : (!m_renderer && !m_headlessResidency))) { Fail(entry, task->type == AssetType::Audio ? "no audio system" : "no renderer (headless)"); return; }
    if (task->type == AssetType::Mesh) {
        if (m_renderer) entry.mesh = m_renderer->CreateMesh(task->mesh);
        entry.bytes = EstimateMeshBytes(task->mesh);
        entry.skeletal=task->mesh.skeletal;
    } else if(task->type==AssetType::Liquid){entry.liquid=task->liquid;entry.bytes=entry.liquid->geometry.cells.size()*sizeof(LiquidTet);if(entry.liquid->basin)entry.bytes+=entry.liquid->basin->geometry.cells.size()*sizeof(LiquidTet)+entry.liquid->basin->curve.size()*sizeof(LiquidCurvePoint);
    } else if (task->type == AssetType::Navigation) {
        entry.navigation=task->navigation;entry.bytes=0;for(auto& layer:entry.navigation->layers)entry.bytes+=layer.size();
    } else if (task->type == AssetType::Audio) {
        entry.bytes=task->audio.samples.size()*sizeof(float);
        entry.audio=m_audio->CreateClip(std::move(task->audio));
        if(!entry.audio.IsValid()){Fail(entry,"audio clip installation failed");return;}
    } else {
        if (m_renderer) entry.texture = m_renderer->CreateTexture(task->texture);
        entry.bytes = EstimateTextureBytes(task->texture);
    }
    entry.state = ResourceState::Ready;
    entry.loadMilliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - task->requested).count();
    if(task->type!=AssetType::Navigation&&task->type!=AssetType::Liquid)++m_stats.uploads;
    m_stats.bytesResident += entry.bytes;
    m_stats.peakBytesResident = std::max(m_stats.peakBytesResident, m_stats.bytesResident);
}

namespace {
// An id has one type; asking for a loaded (or loading) entry as another
// type is a caller error reported as Failed without disturbing the entry.
bool TypeMismatch(ResourceState state, AssetType actual, AssetType expected) {
    return state != ResourceState::Unloaded && state != ResourceState::Cancelled && actual != expected;
}
}  // namespace

ResourceState ResourceManager::RequestAudio(const AssetId& id, JobPriority priority) {
    if(id.empty())return ResourceState::Unloaded;
    const auto& e=Begin(id,AssetType::Audio,priority);
    return TypeMismatch(e.state,e.type,AssetType::Audio)?ResourceState::Failed:e.state;
}
AudioClipHandle ResourceManager::GetAudio(const AssetId& id,std::string& error,JobPriority priority) {
    if(id.empty()){error="no audio asset selected";return {};}
    const auto& e=Begin(id,AssetType::Audio,priority);
    if(TypeMismatch(e.state,e.type,AssetType::Audio)){error="asset is not audio";return {};}
    if(e.state==ResourceState::Failed)error=e.error;
    else if(e.state!=ResourceState::Ready)error="loading";
    else error.clear();
    return e.state==ResourceState::Ready?e.audio:AudioClipHandle{};
}

ResourceState ResourceManager::RequestMesh(const AssetId& id, JobPriority priority) {
    if (id.empty()) return ResourceState::Unloaded;
    const Entry& entry = Begin(id, AssetType::Mesh, priority);
    return TypeMismatch(entry.state, entry.type, AssetType::Mesh) ? ResourceState::Failed : entry.state;
}

ResourceState ResourceManager::RequestTexture(const AssetId& id, JobPriority priority) {
    if (id.empty()) return ResourceState::Unloaded;
    const Entry& entry = Begin(id, AssetType::Texture, priority);
    return TypeMismatch(entry.state, entry.type, AssetType::Texture) ? ResourceState::Failed : entry.state;
}

MeshHandle ResourceManager::GetMesh(const AssetId& id, std::string& outError, JobPriority priority) {
    if (id.empty()) return MeshHandle{};
    const Entry& entry = Begin(id, AssetType::Mesh, priority);
    if (TypeMismatch(entry.state, entry.type, AssetType::Mesh)) { outError = "asset " + id + " is not a mesh"; return MeshHandle{}; }
    if (entry.state == ResourceState::Failed) outError = entry.error;
    else if (entry.state != ResourceState::Ready) outError = "loading";
    return entry.state == ResourceState::Ready ? entry.mesh : MeshHandle{};
}

TextureHandle ResourceManager::GetTexture(const AssetId& id, std::string& outError, JobPriority priority) {
    if (id.empty()) return TextureHandle{};
    const Entry& entry = Begin(id, AssetType::Texture, priority);
    if (TypeMismatch(entry.state, entry.type, AssetType::Texture)) { outError = "asset " + id + " is not a texture"; return TextureHandle{}; }
    if (entry.state == ResourceState::Failed) outError = entry.error;
    else if (entry.state != ResourceState::Ready) outError = "loading";
    return entry.state == ResourceState::Ready ? entry.texture : TextureHandle{};
}

MeshHandle ResourceManager::TryGetMesh(const AssetId& id) {
    const auto it = m_entries.find(id);
    if (it == m_entries.end() || it->second.state != ResourceState::Ready) return MeshHandle{};
    it->second.lastUse = ++m_useClock;
    ++m_stats.hits;
    return it->second.mesh;
}

TextureHandle ResourceManager::TryGetTexture(const AssetId& id) {
    const auto it = m_entries.find(id);
    if (it == m_entries.end() || it->second.state != ResourceState::Ready) return TextureHandle{};
    it->second.lastUse = ++m_useClock;
    ++m_stats.hits;
    return it->second.texture;
}

MeshHandle ResourceManager::GetTerrainMesh(const std::string& identifier, const RadialTerrain& surface) {
    if (!m_renderer || m_shutDown) return MeshHandle{};
    const auto it = m_terrainMeshes.find(identifier);
    if (it != m_terrainMeshes.end()) {
        ++m_stats.hits;
        return it->second;
    }
    ++m_stats.misses;
    // The same tessellation M25 accepted for its authored planet.
    const MeshHandle handle = m_renderer->CreateMesh(surface.BuildMesh(96, 128));
    m_terrainMeshes[identifier] = handle;
    ++m_stats.loadedTerrainMeshes;
    return handle;
}

void ResourceManager::AddRef(const AssetId& id) {
    if (id.empty()) return;
    ++m_entries[id].refs;
}

void ResourceManager::ReleaseRef(const AssetId& id) {
    const auto it = m_entries.find(id);
    if (it == m_entries.end() || it->second.refs == 0) return;
    Entry& entry = it->second;
    --entry.refs;
    if (entry.refs > 0) return;
    // Nobody needs it: cancel queued/running work and discard prepared
    // CPU data that is still waiting for an upload slot. CpuReady has no
    // GPU resource yet; uploading it after demand ended wastes residency.
    // The generation bump rejects completion through the same stale path.
    if (entry.task && (entry.state == ResourceState::Queued || entry.state == ResourceState::Loading ||
                       entry.state == ResourceState::CpuReady)) {
        if (m_jobs) {
            m_jobs->Cancel(entry.task->job);
            TraceTask(*entry.task, ResourceTracePoint::CancelRequested);
        }
        ++entry.generation;
        entry.task.reset();
        entry.state = ResourceState::Cancelled;
        ++m_stats.cancelled;
    }
}

unsigned int ResourceManager::RefCount(const AssetId& id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() ? 0u : it->second.refs;
}

void ResourceManager::Pump(std::size_t maxUploads) {
    if (m_shutDown) return;
    std::size_t uploads = 0;
    for (std::size_t i = 0; i < m_inFlight.size();) {
        const std::shared_ptr<LoadTask>& task = m_inFlight[i];
        const int stage = task->stage.load(std::memory_order_acquire);
        auto entryIt = m_entries.find(task->id);
        Entry* entry = entryIt == m_entries.end() ? nullptr : &entryIt->second;
        // Reflect the worker's progress.
        if (entry && entry->task == task && stage == 1 && entry->state == ResourceState::Queued) {
            entry->state = ResourceState::Loading;
        }
        bool finished = false;
        if (m_jobs) {
            const JobState jobState = m_jobs->StateOf(task->job);
            finished = jobState == JobState::Completed || jobState == JobState::Failed ||
                       jobState == JobState::Cancelled || jobState == JobState::Unknown;
            if (jobState == JobState::Cancelled && stage == 0) task->cancelled = true;
        } else {
            finished = stage == 2;
        }
        if (!finished) { ++i; continue; }
        if (entry && entry->task == task && task->succeeded && !task->cancelled && uploads >= maxUploads) {
            // Upload budget for this frame spent; the data waits.
            if (entry->state != ResourceState::CpuReady) {
                entry->state = ResourceState::CpuReady;
                TraceTask(*task, ResourceTracePoint::CpuReady);
            }
            ++i;
            continue;
        }
        if (entry && entry->task == task) {
            if (task->succeeded && !task->cancelled) ++uploads;
            CompleteTask(*entry, task);
        } else {
            ++m_stats.staleDiscarded;  // released/invalidated meanwhile
            TraceTask(*task, ResourceTracePoint::StaleDiscarded);
        }
        if (m_jobs) m_jobs->Forget(task->job);
        m_inFlight.erase(m_inFlight.begin() + static_cast<std::ptrdiff_t>(i));
    }
    if (m_stats.bytesResident > m_budgetBytes) EvictToFit(m_budgetBytes);
}

void ResourceManager::WaitForAll() {
    while (!m_inFlight.empty()) {
        if (m_jobs) {
            for (const std::shared_ptr<LoadTask>& task : m_inFlight) m_jobs->Wait(task->job);
        }
        Pump(std::numeric_limits<std::size_t>::max());
    }
}

std::size_t ResourceManager::EvictToFit(std::uint64_t targetBytes) {
    std::size_t evicted = 0;
    while (m_stats.bytesResident > targetBytes) {
        // Least recently used, unreferenced, Ready.
        Entry* victim = nullptr;
        AssetId victimId;
        for (auto& [id, entry] : m_entries) {
            if (entry.state != ResourceState::Ready || entry.refs > 0) continue;
            if (!victim || entry.lastUse < victim->lastUse) { victim = &entry; victimId = id; }
        }
        if (!victim) break;
        DestroyGpu(*victim);
        ++victim->generation;
        victim->state = ResourceState::Unloaded;
        ++evicted;
        ++m_stats.evictions;
    }
    return evicted;
}

void ResourceManager::DestroyGpu(Entry& entry) {
    if (entry.state == ResourceState::Ready) {
        if (m_renderer && entry.mesh.IsValid()) m_renderer->DestroyMesh(entry.mesh);
        if (m_renderer && entry.texture.IsValid()) m_renderer->DestroyTexture(entry.texture);
        if (m_audio && entry.audio.IsValid()) m_audio->DestroyClip(entry.audio);
        m_stats.bytesResident -= std::min(m_stats.bytesResident, entry.bytes);
    }
    entry.mesh = MeshHandle{};
    entry.skeletal.reset();entry.navigation.reset();entry.liquid.reset();
    entry.texture = TextureHandle{};
    entry.audio = AudioClipHandle{};
    entry.bytes = 0;
}

ResourceState ResourceManager::StateOf(const AssetId& id) const {
    const auto it = m_entries.find(id);
    if (it == m_entries.end()) return ResourceState::Unloaded;
    const Entry& entry = it->second;
    if (entry.task && entry.state == ResourceState::Queued && entry.task->stage.load(std::memory_order_acquire) >= 1) {
        return ResourceState::Loading;
    }
    return entry.state;
}

std::string ResourceManager::ErrorOf(const AssetId& id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() ? std::string() : it->second.error;
}

std::uint64_t ResourceManager::BytesOf(const AssetId& id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() || it->second.state != ResourceState::Ready ? 0ull : it->second.bytes;
}

std::thread::id ResourceManager::DecodeThreadOf(const AssetId& id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() ? std::thread::id() : it->second.decodeThread;
}

void ResourceManager::Release(const AssetId& id) {
    const auto it = m_entries.find(id);
    if (it == m_entries.end()) return;
    Entry& entry = it->second;
    if (entry.task && m_jobs) {
        m_jobs->Cancel(entry.task->job);
        TraceTask(*entry.task, ResourceTracePoint::CancelRequested);
    }
    DestroyGpu(entry);
    ++entry.generation;  // any completion for the old generation is stale
    entry.task.reset();
    entry.error.clear();
    entry.state = ResourceState::Unloaded;
    entry.decodeThread = std::thread::id();
    entry.loadMilliseconds = 0.0;
    // Keep the entry (its refs and generation) so demand bookkeeping survives.
}

void ResourceManager::ReleaseAll() {
    for (auto& [id, entry] : m_entries) {
        if (entry.task && m_jobs) {
            m_jobs->Cancel(entry.task->job);
            TraceTask(*entry.task, ResourceTracePoint::CancelRequested);
        }
        DestroyGpu(entry);
        ++entry.generation;
        entry.task.reset();
        entry.error.clear();
        entry.state = ResourceState::Unloaded;
    }
    if (m_renderer) {
        for (auto& [id, handle] : m_terrainMeshes) {
            if (handle.IsValid()) m_renderer->DestroyMesh(handle);
        }
    }
    m_terrainMeshes.clear();
    // Project handoff/shutdown drains synchronously: workers finish their
    // current cooperative unit, then these old results are discarded here.
    if (m_jobs) {
        for (const std::shared_ptr<LoadTask>& task : m_inFlight) m_jobs->Wait(task->job);
        for (const std::shared_ptr<LoadTask>& task : m_inFlight) m_jobs->Forget(task->job);
    }
    for (const std::shared_ptr<LoadTask>& task : m_inFlight) TraceTask(*task, ResourceTracePoint::StaleDiscarded);
    m_inFlight.clear();
    m_stats.loadedMeshes = m_stats.loadedTextures = m_stats.loadedAudio = m_stats.loadedTerrainMeshes = m_stats.failed = 0;
    m_stats.bytesResident = 0;
}

void ResourceManager::Shutdown() {
    if (m_shutDown) return;
    if (m_trace) m_trace(ResourceTraceEvent{ResourceTracePoint::ManagerShutdownBegin, {}, {}});
    ReleaseAll();
    m_entries.clear();
    m_shutDown = true;
    if (m_trace) m_trace(ResourceTraceEvent{ResourceTracePoint::ManagerShutdownEnd, {}, {}});
}

void ResourceManager::RefreshCounts() const {
    std::size_t loading = 0, ready = 0, failed = 0, meshes = 0, textures = 0, audio = 0, navigation = 0;
    for (const auto& [id, entry] : m_entries) {
        switch (entry.state) {
            case ResourceState::Queued:
            case ResourceState::Loading:
            case ResourceState::CpuReady: ++loading; break;
            case ResourceState::Ready:
                ++ready;
                if (entry.mesh.IsValid()) ++meshes;
                if (entry.texture.IsValid()) ++textures;
                if (entry.audio.IsValid()) ++audio;
                if(entry.navigation)++navigation;
                break;
            case ResourceState::Failed: ++failed; break;
            default: break;
        }
    }
    m_stats.loading = loading;
    m_stats.ready = ready;
    m_stats.failed = failed;
    m_stats.loadedMeshes = meshes;
    m_stats.loadedTextures = textures;
    m_stats.loadedAudio = audio;
    m_stats.loadedNavigation=navigation;
    m_stats.budgetBytes = m_budgetBytes;
}

const ResourceStats& ResourceManager::Stats() const {
    RefreshCounts();
    return m_stats;
}

std::vector<ResourceManager::EntryView> ResourceManager::Entries() const {
    std::vector<EntryView> out;
    for (const auto& [id, entry] : m_entries) {
        EntryView view;
        view.id = id;
        view.type = entry.type;
        view.state = StateOf(id);
        view.bytes = entry.bytes;
        view.refs = entry.refs;
        view.loadMilliseconds = entry.loadMilliseconds;
        out.push_back(view);
    }
    return out;
}

std::shared_ptr<const SkeletalAsset> ResourceManager::TryGetSkeletal(const AssetId& id) const{
 auto it=m_entries.find(id);return it!=m_entries.end()&&it->second.state==ResourceState::Ready?it->second.skeletal:nullptr;
}

ResourceState ResourceManager::RequestNavigation(const AssetId& id,JobPriority p){auto& e=Begin(id,AssetType::Navigation,p);return TypeMismatch(e.state,e.type,AssetType::Navigation)?ResourceState::Failed:e.state;}
std::shared_ptr<const NavigationData> ResourceManager::GetNavigation(const AssetId& id,std::string& error){auto& e=Begin(id,AssetType::Navigation,JobPriority::Normal);if(e.type!=AssetType::Navigation){error="asset is not navigation";return nullptr;}if(e.state!=ResourceState::Ready){error=e.state==ResourceState::Failed?e.error:"loading";return nullptr;}error.clear();return e.navigation;}

ResourceState ResourceManager::RequestLiquid(const AssetId& id,JobPriority p){auto& e=Begin(id,AssetType::Liquid,p);return TypeMismatch(e.state,e.type,AssetType::Liquid)?ResourceState::Failed:e.state;}
std::shared_ptr<const LiquidResource> ResourceManager::GetLiquid(const AssetId& id,std::string& error){auto& e=Begin(id,AssetType::Liquid,JobPriority::Normal);if(e.type!=AssetType::Liquid){error="asset is not liquid data";return nullptr;}if(e.state!=ResourceState::Ready){error=e.state==ResourceState::Failed?e.error:"loading";return nullptr;}error.clear();return e.liquid;}
