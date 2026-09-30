#include "AudioSystem.h"
#include "miniaudio.h"
#include <algorithm>
#include <cmath>
#include <map>

extern "C" float judas_audio_distance_gain(int,float,float,float,float);
float AudioSystem::DistanceGainForDiagnostics(const AudioSettings& s,float distance){
    if(!s.spatial)return 1;
    return judas_audio_distance_gain(static_cast<int>(s.attenuation),distance,s.referenceDistance,s.maximumDistance,s.rolloff);
}

bool ValidAudioSettings(const AudioSettings& s){
    return std::isfinite(s.volume)&&s.volume>=0&&s.volume<=1&&std::isfinite(s.pitch)&&s.pitch>=.125f&&s.pitch<=8&&
        std::isfinite(s.referenceDistance)&&s.referenceDistance>0&&std::isfinite(s.maximumDistance)&&s.maximumDistance>s.referenceDistance&&
        std::isfinite(s.rolloff)&&s.rolloff>=0&&static_cast<int>(s.attenuation)>=0&&static_cast<int>(s.attenuation)<=2;
}
namespace {
glm::vec3 Vector(ma_vec3f v){return {v.x,v.y,v.z};}
// Backend "none" also bypasses panning; zero-rolloff inverse preserves spatial positioning.
ma_attenuation_model Model(AudioAttenuation model){
    return model==AudioAttenuation::Inverse?ma_attenuation_model_inverse:model==AudioAttenuation::Linear?ma_attenuation_model_linear:ma_attenuation_model_inverse;
}
}
struct AudioSystem::Impl {
    ma_engine engine{};bool initialized=false,noDevice=false;std::string initFailure;
    std::uint64_t nextClip=1,nextVoice=1;
    std::map<std::uint64_t,std::shared_ptr<const AudioData>> clips;
    struct Voice {
        std::shared_ptr<const AudioData> data;ma_audio_buffer buffer{};ma_sound sound{};
        bool paused=false,stopped=true,autoRelease=false,bufferReady=false,soundReady=false;
        AudioAttenuation attenuation=AudioAttenuation::Inverse;
        ~Voice(){if(soundReady)ma_sound_uninit(&sound);if(bufferReady)ma_audio_buffer_uninit(&buffer);}
    };
    std::map<std::uint64_t,std::unique_ptr<Voice>> voices;
};
AudioSystem::AudioSystem():m(std::make_unique<Impl>()){}
AudioSystem::~AudioSystem(){Shutdown();}
bool AudioSystem::Init(std::string& error,bool noDevice){
    if(m->initialized){error.clear();return true;}
    if(!m->initFailure.empty()){error=m->initFailure;return false;}
    auto config=ma_engine_config_init();config.listenerCount=1;config.channels=2;config.sampleRate=48000;
    config.noDevice=noDevice?MA_TRUE:MA_FALSE;
    const auto result=ma_engine_init(&config,&m->engine);
    if(result!=MA_SUCCESS){error="audio device/engine initialization failed: "+std::to_string(result);m->initFailure=error;return false;}
    m->initialized=true;m->noDevice=noDevice;error.clear();return true;
}
void AudioSystem::Shutdown(){StopAll();m->clips.clear();if(m->initialized)ma_engine_uninit(&m->engine);m->initialized=false;m->initFailure.clear();}
AudioClipHandle AudioSystem::CreateClip(AudioData data){
    if(!data.Frames()||data.channels>2||data.sampleRate!=48000)return {};
    const auto id=m->nextClip++;m->clips[id]=std::make_shared<const AudioData>(std::move(data));return {id};
}
void AudioSystem::DestroyClip(AudioClipHandle clip){m->clips.erase(clip.id);}
AudioVoiceHandle AudioSystem::CreateVoice(AudioClipHandle clip,const AudioSettings& settings,std::string& error){
    auto data=m->clips.find(clip.id);
    if(data==m->clips.end()||!ValidAudioSettings(settings)){error="invalid clip or audio settings";return {};}
    if(!Init(error))return {};
    auto voice=std::make_unique<Impl::Voice>();voice->data=data->second;
    auto bufferConfig=ma_audio_buffer_config_init(ma_format_f32,voice->data->channels,voice->data->Frames(),voice->data->samples.data(),nullptr);
    bufferConfig.sampleRate=voice->data->sampleRate;
    if(ma_audio_buffer_init(&bufferConfig,&voice->buffer)!=MA_SUCCESS){error="audio buffer initialization failed";return {};}
    voice->bufferReady=true;
    if(ma_sound_init_from_data_source(&m->engine,&voice->buffer,0,nullptr,&voice->sound)!=MA_SUCCESS){error="audio voice initialization failed";return {};}
    voice->soundReady=true;
    const auto id=m->nextVoice++;m->voices[id]=std::move(voice);AudioVoiceHandle handle{id};
    SetSettings(handle,settings);ma_sound_set_pinned_listener_index(&m->voices[id]->sound,0);ma_sound_set_doppler_factor(&m->voices[id]->sound,0);
    error.clear();return handle;
}
AudioVoiceHandle AudioSystem::PlayOneShot(AudioClipHandle clip,const AudioSettings& settings,const glm::vec3& position,std::string& error){
    auto once=settings;once.loop=false;auto voice=CreateVoice(clip,once,error);
    if(voice.IsValid()){m->voices[voice.id]->autoRelease=true;SetPosition(voice,position);Play(voice);}return voice;
}
void AudioSystem::DestroyVoice(AudioVoiceHandle voice){m->voices.erase(voice.id);}
bool AudioSystem::Play(AudioVoiceHandle handle){auto v=m->voices.find(handle.id);if(v==m->voices.end())return false;
    ma_sound_stop(&v->second->sound);ma_sound_seek_to_pcm_frame(&v->second->sound,0);v->second->paused=false;const bool started=ma_sound_start(&v->second->sound)==MA_SUCCESS;v->second->stopped=!started;return started;}
bool AudioSystem::Stop(AudioVoiceHandle handle){auto v=m->voices.find(handle.id);if(v==m->voices.end())return false;
    ma_sound_stop(&v->second->sound);ma_sound_seek_to_pcm_frame(&v->second->sound,0);v->second->paused=false;v->second->stopped=true;return true;}
bool AudioSystem::Pause(AudioVoiceHandle handle){auto v=m->voices.find(handle.id);if(v==m->voices.end()||!ma_sound_is_playing(&v->second->sound))return false;
    ma_sound_stop(&v->second->sound);v->second->paused=true;return true;}
bool AudioSystem::Resume(AudioVoiceHandle handle){auto v=m->voices.find(handle.id);if(v==m->voices.end()||!v->second->paused)return false;
    v->second->paused=false;return ma_sound_start(&v->second->sound)==MA_SUCCESS;}
bool AudioSystem::SetSettings(AudioVoiceHandle handle,const AudioSettings& s){auto v=m->voices.find(handle.id);if(v==m->voices.end()||!ValidAudioSettings(s))return false;
    auto* sound=&v->second->sound;ma_sound_set_looping(sound,s.loop);ma_sound_set_spatialization_enabled(sound,s.spatial);
    ma_sound_set_volume(sound,s.volume);ma_sound_set_pitch(sound,s.pitch);ma_sound_set_attenuation_model(sound,Model(s.attenuation));
    ma_sound_set_min_distance(sound,s.referenceDistance);ma_sound_set_max_distance(sound,s.maximumDistance);ma_sound_set_rolloff(sound,s.attenuation==AudioAttenuation::None?0:s.rolloff);
    v->second->attenuation=s.attenuation;return true;}
bool AudioSystem::SetPosition(AudioVoiceHandle handle,const glm::vec3& p){
    auto v=m->voices.find(handle.id);
    if(v==m->voices.end()||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
    if(ma_sound_is_spatialization_enabled(&v->second->sound))ma_sound_set_position(&v->second->sound,p.x,p.y,p.z);
    return true;
}
bool AudioSystem::SetListener(const glm::vec3& p,const glm::quat& orientation){
    const float length=glm::dot(orientation,orientation);
    if(!m->initialized||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(length)||length<=0)return false;
    const auto q=glm::normalize(orientation);const auto forward=q*glm::vec3(0,0,-1),up=q*glm::vec3(0,1,0);
    ma_engine_listener_set_position(&m->engine,0,p.x,p.y,p.z);ma_engine_listener_set_direction(&m->engine,0,forward.x,forward.y,forward.z);ma_engine_listener_set_world_up(&m->engine,0,up.x,up.y,up.z);
    return true;
}
bool AudioSystem::Snapshot(AudioVoiceHandle handle,AudioVoiceSnapshot& out)const{
    auto v=m->voices.find(handle.id);if(v==m->voices.end())return false;const auto* s=&v->second->sound;
    out.state=v->second->stopped?AudioPlaybackState::Stopped:ma_sound_at_end(s)?AudioPlaybackState::Finished:v->second->paused?AudioPlaybackState::Paused:ma_sound_is_playing(s)?AudioPlaybackState::Playing:AudioPlaybackState::Stopped;
    out.position=Vector(ma_sound_get_position(s));out.spatial=ma_sound_is_spatialization_enabled(s);out.loop=ma_sound_is_looping(s);
    out.volume=ma_sound_get_volume(s);out.pitch=ma_sound_get_pitch(s);out.referenceDistance=ma_sound_get_min_distance(s);out.maximumDistance=ma_sound_get_max_distance(s);out.rolloff=ma_sound_get_rolloff(s);
    const auto model=ma_sound_get_attenuation_model(s);out.attenuation=v->second->attenuation;
    out.spatialPanningEnabled=out.spatial&&model!=ma_attenuation_model_none;
    out.cursorAvailable=m->noDevice;out.cursorFrames=0;
    if(m->noDevice){ma_uint64 cursor=0;ma_sound_get_cursor_in_pcm_frames(s,&cursor);out.cursorFrames=cursor;}
    return true;
}
AudioListenerSnapshot AudioSystem::Listener()const{
    if(!m->initialized)return {};
    return {Vector(ma_engine_listener_get_position(&m->engine,0)),Vector(ma_engine_listener_get_direction(&m->engine,0)),Vector(ma_engine_listener_get_world_up(&m->engine,0))};
}
void AudioSystem::Update(){for(auto v=m->voices.begin();v!=m->voices.end();)if(v->second->autoRelease&&ma_sound_at_end(&v->second->sound))v=m->voices.erase(v);else ++v;}
void AudioSystem::StopAll(){m->voices.clear();}
bool AudioSystem::IsInitialized()const{return m->initialized;}
std::size_t AudioSystem::VoiceCount()const{return m->voices.size();}
std::size_t AudioSystem::ClipCount()const{return m->clips.size();}
bool AudioSystem::AdvanceWithoutDevice(std::uint64_t frames){
    if(!m->initialized||!m->noDevice)return false;
    float scratch[512*2];while(frames){const auto n=std::min<std::uint64_t>(frames,512);if(ma_engine_read_pcm_frames(&m->engine,scratch,n,nullptr)!=MA_SUCCESS)return false;frames-=n;}return true;
}
