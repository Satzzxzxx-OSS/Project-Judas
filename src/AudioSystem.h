#pragma once
#include <memory>
#include <string>
#include <glm/gtc/quaternion.hpp>
#include "AudioTypes.h"
#include "AudioData.h"

// Single engine audio owner. Main-thread API; miniaudio owns device/mixing threads.
// No backend object crosses this boundary. Device opens lazily at first voice.
class AudioSystem {
public:
    AudioSystem(); ~AudioSystem();
    AudioSystem(const AudioSystem&)=delete;AudioSystem& operator=(const AudioSystem&)=delete;
    bool Init(std::string& error, bool noDevice=false);
    void Shutdown();
    AudioClipHandle CreateClip(AudioData data);
    void DestroyClip(AudioClipHandle clip); // existing voices retain immutable PCM
    AudioVoiceHandle CreateVoice(AudioClipHandle clip,const AudioSettings& settings,std::string& error);
    AudioVoiceHandle PlayOneShot(AudioClipHandle clip,const AudioSettings& settings,const glm::vec3& position,std::string& error);
    void DestroyVoice(AudioVoiceHandle voice);
    bool Play(AudioVoiceHandle voice);bool Stop(AudioVoiceHandle voice);
    bool Pause(AudioVoiceHandle voice);bool Resume(AudioVoiceHandle voice);
    bool SetSettings(AudioVoiceHandle voice,const AudioSettings& settings);
    bool SetPosition(AudioVoiceHandle voice,const glm::vec3& position);
    bool SetListener(const glm::vec3& position,const glm::quat& orientation);
    bool Snapshot(AudioVoiceHandle voice,AudioVoiceSnapshot& out) const;
    AudioListenerSnapshot Listener() const;
    void Update();void StopAll();
    static float DistanceGainForDiagnostics(const AudioSettings& settings,float distance);
    bool IsInitialized() const;
    std::size_t VoiceCount() const;std::size_t ClipCount() const;
    // Deterministic device-free test/offline seam. Same backend mixer/voices;
    // consumes samples without a recording or any perceptual assertion.
    bool AdvanceWithoutDevice(std::uint64_t frames);
private:
    struct Impl;std::unique_ptr<Impl> m;
};
