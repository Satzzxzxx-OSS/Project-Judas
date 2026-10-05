#pragma once
#include <cstdint>
#include <string>
#include "Classification.h"
#include "AudioEnvironment.h"
#include <glm/glm.hpp>

struct AudioClipHandle { std::uint64_t id=0; bool IsValid() const { return id!=0; } };
struct AudioVoiceHandle { std::uint64_t id=0; bool IsValid() const { return id!=0; } };
enum class AudioAttenuation { None, Inverse, Linear };
enum class AudioLoading { Buffered, Streamed };
struct AudioSettings {
    AudioLoading loading=AudioLoading::Buffered;
    unsigned streamPageFrames=4096;
    std::string group;
    float doppler=0,send=0;
    bool occlusion=false,bypass=false;
    CategoryMask occlusionMask=kAllCategories;
    float occludedGain=.25f,occludedCutoff=1200;

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
    bool streamed=false,ready=false,seeking=false,starved=false;
    double positionSeconds=0,durationSeconds=0;
    bool durationKnown=false;
    std::size_t bufferBytes=0;
    std::uint64_t underruns=0;
    std::string error;
    float dopplerRatio=1,occlusionGain=1,cutoff=24000,distanceGain=1;

    AudioPlaybackState state=AudioPlaybackState::Stopped;
    bool spatial=false, loop=false, spatialPanningEnabled=false;
    glm::vec3 position{0};
    float volume=0, pitch=1, referenceDistance=1, maximumDistance=30, rolloff=1;
    AudioAttenuation attenuation=AudioAttenuation::None;
    // Atomically published media-frame cursor, including real device playback.
    bool cursorAvailable=false;
    std::uint64_t cursorFrames=0;
};
struct AudioListenerSnapshot {
    glm::vec3 position{0}, forward{0,0,-1}, up{0,1,0};
};

struct AudioDiagnostics {
 std::size_t voices=0,streams=0,bufferedBytes=0,streamBytes=0,streamHighWater=0,pendingRetirements=0,reverbProcessors=0;
 std::uint64_t underruns=0,decodedFrames=0,occlusionQueries=0;
 double maximumDetachMilliseconds=0;
};
