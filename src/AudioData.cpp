#include "AudioData.h"
#include "AsyncFile.h"
#include "miniaudio.h"
#include <cmath>

bool DecodeAudioFromMemory(const void* bytes, std::size_t size, AudioData& out, std::string& error) {
    ma_decoder decoder{};
    const auto config=ma_decoder_config_init(ma_format_f32,0,48000);
    if (!bytes || !size || ma_decoder_init_memory(bytes,size,&config,&decoder)!=MA_SUCCESS) {
        error="could not decode audio (WAV, MP3 or FLAC required)"; return false;
    }
    ma_uint64 frames=0;
    const bool valid=ma_decoder_get_length_in_pcm_frames(&decoder,&frames)==MA_SUCCESS && frames>0 &&
        decoder.outputChannels>0 && decoder.outputChannels<=2 &&
        frames<=512ull*1024*1024/(sizeof(float)*decoder.outputChannels);
    if (!valid) { ma_decoder_uninit(&decoder);error="invalid/empty audio, unsupported channels, or decoded clip exceeds 512 MiB";return false; }
    AudioData data;data.channels=decoder.outputChannels;data.sampleRate=decoder.outputSampleRate;
    data.samples.resize(static_cast<std::size_t>(frames)*data.channels);
    ma_uint64 read=0;const auto result=ma_decoder_read_pcm_frames(&decoder,data.samples.data(),frames,&read);
    ma_decoder_uninit(&decoder);
    if ((result!=MA_SUCCESS && result!=MA_AT_END) || !read) {error="audio decode failed";return false;}
    data.samples.resize(static_cast<std::size_t>(read)*data.channels);
    for(float value:data.samples) if(!std::isfinite(value)){error="non-finite decoded audio";return false;}
    out=std::move(data);error.clear();return true;
}
bool LoadAudioFromFile(const std::string& path, AudioData& out, std::string& error) {
    std::vector<std::uint8_t> bytes;
    return ReadWholeFile(path,bytes,error) && DecodeAudioFromMemory(bytes.data(),bytes.size(),out,error);
}
