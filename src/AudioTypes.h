#pragma once
#include <cstdint>
#include <glm/glm.hpp>

struct AudioClipHandle { std::uint64_t id=0; bool IsValid() const { return id!=0; } };
struct AudioVoiceHandle { std::uint64_t id=0; bool IsValid() const { return id!=0; } };
enum class AudioAttenuation { None, Inverse, Linear };
struct AudioSettings {
    bool loop=false;
    bool spatial=true;
    float volume=1.0f;
    float pitch=1.0f;
    float referenceDistance=1.0f;
    float maximumDistance=30.0f;
    float rolloff=1.0f;
    AudioAttenuation attenuation=AudioAttenuation::Inverse;
};
bool ValidAudioSettings(const AudioSettings& settings);
enum class AudioPlaybackState { Stopped, Playing, Paused, Finished };
struct AudioVoiceSnapshot {
    AudioPlaybackState state=AudioPlaybackState::Stopped;
    bool spatial=false, loop=false, spatialPanningEnabled=false;
    glm::vec3 position{0};
    float volume=0, pitch=1, referenceDistance=1, maximumDistance=30, rolloff=1;
    AudioAttenuation attenuation=AudioAttenuation::None;
    // Exact source cursor is exposed only with device-free manual advancement.
    // The backend PCM cursor is not atomic during device mixing.
    bool cursorAvailable=false;
    std::uint64_t cursorFrames=0;
};
struct AudioListenerSnapshot {
    glm::vec3 position{0}, forward{0,0,-1}, up{0,1,0};
};
