#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// Whole, interleaved float PCM. Immutable once installed in AudioSystem.
struct AudioData {
    unsigned int channels=0, sampleRate=48000;
    std::vector<float> samples;
    std::uint64_t Frames() const { return channels ? samples.size()/channels : 0; }
};
bool DecodeAudioFromMemory(const void* bytes, std::size_t size, AudioData& out, std::string& error);
bool LoadAudioFromFile(const std::string& path, AudioData& out, std::string& error);

bool ValidateAudioFile(const std::string& path,std::string& error);
