#include "AudioSystem.h"
#include "JobSystem.h"
#include "SceneSerialization.h"
#include "Scene.h"
#include "Project.h"
#include "Prefab.h"
#include "SceneFingerprint.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
namespace {
int checks=0,failures=0;
void Check(bool ok,const char* label){++checks;failures+=!ok;std::printf("%s %s\n",ok?"PASS":"FAIL",label);}
using Clock=std::chrono::steady_clock;
AudioData Tone(float frequency=1000,float seconds=3){AudioData d;d.sampleRate=48000;d.channels=2;d.samples.resize(size_t(seconds*48000)*2);for(size_t i=0;i<d.Frames();++i)d.samples[i*2]=d.samples[i*2+1]=.2f*std::sin(float(i)*6.283185307f*frequency/48000);return d;}
std::vector<float> Capture(AudioSystem& a,unsigned n){std::vector<float> pcm(n*2);Check(a.ReadWithoutDevice(pcm.data(),n),"actual production graph PCM read");return pcm;}
double RMS(const std::vector<float>& p,size_t start=0){double e=0;for(size_t i=start;i<p.size();++i){if(!std::isfinite(p[i]))return INFINITY;e+=p[i]*p[i];}return std::sqrt(e/std::max<size_t>(1,p.size()-start));}
double Frequency(const std::vector<float>& p){unsigned crossings=0;for(size_t i=2;i<p.size();i+=2)if(p[i-2]<=0&&p[i]>0)++crossings;return crossings*48000./(p.size()/2);}
bool WaitReady(AudioSystem& a,AudioVoiceHandle h){auto deadline=Clock::now()+std::chrono::seconds(3);AudioVoiceSnapshot s;while(Clock::now()<deadline){a.Update();if(a.Snapshot(h,s)&&s.ready&&!s.seeking)return true;if(!s.error.empty())return false;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false;}
void Drain(AudioSystem& a){auto deadline=Clock::now()+std::chrono::seconds(3);while(a.Diagnostics().pendingRetirements&&Clock::now()<deadline){a.Update();std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
}
int main(int argc,char** argv){
 if(argc<2){std::fprintf(stderr,"usage: audio_acoustics_tests fixture-directory\n");return 2;}
 std::filesystem::path fixtures=argv[1];JobSystem jobs(2);AudioSystem a;std::string error;a.SetJobSystem(&jobs);Check(a.Init(error,true),"device-free production miniaudio engine");
 AudioSettings s;s.spatial=false;s.loading=AudioLoading::Streamed;s.streamPageFrames=2048;
 AudioVoiceSnapshot snap;
 std::string fileHash;
 Check(SceneFingerprintSha256File((fixtures/"long.wav").string(),fileHash,error),"bounded streaming file identity");
 std::ifstream hashFile(fixtures/"long.wav",std::ios::binary);std::string hashBytes((std::istreambuf_iterator<char>(hashFile)),{});
 Check(fileHash==SceneFingerprintSha256(hashBytes),"incremental digest identical to canonical primitive across pages");
 fileHash="unchanged";Check(!SceneFingerprintSha256File((fixtures/"long.wav").string(),fileHash,error,[]{return true;})&&fileHash=="unchanged","cancelled incremental hash leaves identity unchanged");

 for(const char* format:{"wav","mp3","flac"}){
  auto path=fixtures/(std::string("long.")+format);auto v=a.CreateStreamVoice(path.string(),s,error);Check(v.IsValid(),"incremental WAV/MP3/FLAC source created");auto begin=Clock::now();Check(WaitReady(a,v),"bounded prefill completes");a.Snapshot(v,snap);
  Check((std::string(format)=="mp3"?!snap.durationKnown:(snap.durationKnown&&snap.durationSeconds>59))&&snap.bufferBytes==65536,"known header length or explicit unknown MP3 length with fixed PCM allocation");
  std::printf("STREAM codec=%s prefill_ms=%.4f bytes=%zu duration_known=%d duration_s=%.5f\n",format,std::chrono::duration<double,std::milli>(Clock::now()-begin).count(),snap.bufferBytes,snap.durationKnown,snap.durationSeconds);
  Check(a.Play(v)&&WaitReady(a,v),"stream starts after ready generation");auto p=Capture(a,2048);Check(RMS(p)>.02,"stream produces real decoded PCM");
  Check(a.Pause(v),"stream pause");a.Snapshot(v,snap);auto position=snap.positionSeconds;Capture(a,2048);a.Snapshot(v,snap);Check(snap.positionSeconds==position,"paused independent cursor holds");
  auto seekBegin=Clock::now();Check(a.Seek(v,30)&&WaitReady(a,v),"seek prefill is bounded and generation-current");std::printf("SEEK codec=%s latency_ms=%.4f\n",format,std::chrono::duration<double,std::milli>(Clock::now()-seekBegin).count());a.Snapshot(v,snap);Check(std::abs(snap.positionSeconds-30)<.001,"seek seconds converted to media frames");Check(a.Resume(v),"resume stream after seek");Capture(a,2048);a.Snapshot(v,snap);Check(snap.positionSeconds>=30,"seek does not replay old pages");
  for(double t:{2.,50.,5.})Check(a.Seek(v,t),"rapid seek accepted");
  Check(WaitReady(a,v),"only latest rapid seek generation readies");a.Snapshot(v,snap);Check(std::abs(snap.positionSeconds-5)<.001,"rapid seek final target wins");
  a.DestroyVoice(v);Drain(a);Check(a.Diagnostics().pendingRetirements==0,"decoder and pages retire on worker");
 }
 auto unknown=a.CreateStreamVoice((fixtures/"no-length.mp3").string(),s,error);Check(WaitReady(a,unknown)&&a.Snapshot(unknown,snap)&&!snap.durationKnown,"MP3 without Xing length starts without whole-file duration scan");a.Play(unknown);Check(a.Seek(unknown,59.98)&&WaitReady(a,unknown),"unknown-duration compressed stream accepts worker seek");Capture(a,8192);a.Snapshot(unknown,snap);Check(snap.durationKnown&&snap.durationSeconds>59&&snap.state==AudioPlaybackState::Finished,"natural MP3 EOF publishes learned length without a prefill scan");a.DestroyVoice(unknown);Drain(a);
 auto v=a.CreateStreamVoice((fixtures/"long.wav").string(),s,error),v2=a.CreateStreamVoice((fixtures/"long.wav").string(),s,error);a.Play(v);a.Play(v2);Check(WaitReady(a,v)&&WaitReady(a,v2),"same immutable stream asset independent prefill");a.Pause(v2);Capture(a,2048);AudioVoiceSnapshot b;a.Snapshot(v,snap);a.Snapshot(v2,b);Check(snap.positionSeconds>b.positionSeconds,"two same-asset cursors independent");
 Check(a.Seek(v,59.98)&&WaitReady(a,v),"seek near EOF");Capture(a,4096);a.Snapshot(v,snap);Check(snap.state==AudioPlaybackState::Finished&&!snap.starved,"EOF distinct from starvation");
 Check(a.Seek(v,12)&&WaitReady(a,v),"seek from EOF");a.Update();Capture(a,1024);a.Snapshot(v,snap);Check(snap.positionSeconds>=12&&snap.positionSeconds<12.1,"EOF restart does not overwrite requested seek");
 a.StopAll();Drain(a);
 s.loop=true;auto loop=a.CreateStreamVoice((fixtures/"loop.wav").string(),s,error);a.Play(loop);Check(WaitReady(a,loop),"loop-compatible fixture prefill");auto p=Capture(a,4096);Check(RMS(p)>.05&&std::abs(Frequency(p)-1000)<40,"loop crosses page/EOF boundaries without inserted silence");
 a.SetStreamDecodeDelay(loop,150);Capture(a,32768);a.Snapshot(loop,snap);Check(snap.starved&&snap.underruns>0&&snap.state!=AudioPlaybackState::Finished,"delayed worker underrun is safe silence, never EOF");auto held=snap.positionSeconds;auto silence=Capture(a,4096);a.Snapshot(loop,snap);Check(RMS(silence)==0&&snap.positionSeconds==held,"starvation silence holds media cursor");a.SetStreamDecodeDelay(loop,0);auto deadline=Clock::now()+std::chrono::seconds(2);std::vector<float> recovered;while(Clock::now()<deadline){a.Update();std::this_thread::sleep_for(std::chrono::milliseconds(2));recovered.resize(2048);a.ReadWithoutDevice(recovered.data(),1024);if(RMS(recovered)>.02)break;}Check(RMS(recovered)>.02,"underrun recovers real decoded PCM");a.SetStreamDecodeDelay(loop,150);Capture(a,32768);a.Update();std::this_thread::sleep_for(std::chrono::milliseconds(5));
 auto retireBegin=Clock::now();a.DestroyVoice(loop);const auto detachMs=std::chrono::duration<double,std::milli>(Clock::now()-retireBegin).count();std::printf("RETIRE delayed_decode_ms=150 detach_ms=%.4f\n",detachMs);Check(detachMs<30,"normal voice removal does not wait for delayed decoder");Drain(a);Check(a.Diagnostics().pendingRetirements==0&&!a.Snapshot(loop,snap),"retired handle cannot resurrect");
 s.loop=false;auto broken=a.CreateStreamVoice((fixtures/"missing.flac").string(),s,error);Check(!WaitReady(a,broken)&&a.Snapshot(broken,snap)&&!snap.error.empty(),"missing stream reports decoder error");a.DestroyVoice(broken);Drain(a);
 std::ofstream(fixtures/"corrupt.flac",std::ios::binary)<<"not an audio stream";auto corrupt=a.CreateStreamVoice((fixtures/"corrupt.flac").string(),s,error);Check(!WaitReady(a,corrupt)&&a.Snapshot(corrupt,snap)&&!snap.error.empty(),"corrupt stream fails explicitly without publishing PCM");a.DestroyVoice(corrupt);Drain(a);Check(a.Diagnostics().pendingRetirements==0,"failed decoder retirement is safe");
 // Independent expectations measured from real mixed tones, not just ratio flags.
 s={};s.spatial=true;s.attenuation=AudioAttenuation::None;s.loop=true;s.doppler=1;auto clip=a.CreateClip(Tone());
 auto tone=a.CreateVoice(clip,s,error);a.SetListener({0,0,0},glm::quat(1,0,0,0));a.SetPosition(tone,{0,0,-10});a.Play(tone);
 for(auto velocities:{std::pair<glm::vec3,glm::vec3>{{0,0,0},{0,0,0}},{{0,0,34.33f},{0,0,0}},{{0,0,-34.33f},{0,0,0}},{{0,0,34.33f},{0,0,34.33f}}}){
  a.SetListenerMotion(velocities.second);a.SetMotion(tone,velocities.first);Capture(a,4096);auto pcm=Capture(a,48000);double actual=Frequency(pcm),expected=1000*AudioSystem::DopplerRatio({0,0,-10},{0,0,0},velocities.first,velocities.second,1);std::printf("DOPPLER actual_hz=%.2f expected_hz=%.2f\n",actual,expected);Check(std::abs(actual-expected)<10,"PCM frequency follows stationary/approach/recede/co-moving ratio");
 }
 auto rotation=glm::angleAxis(.9f,glm::normalize(glm::vec3(1,2,3)));Check(std::abs(AudioSystem::DopplerRatio(rotation*glm::vec3(0,0,-10),{},rotation*glm::vec3(0,0,34.33),{},1)-1/0.9f)<1e-5,"arbitrary orientation Doppler equivariance");Check(std::isfinite(AudioSystem::DopplerRatio({}, {},{1e9,0,0},{},4)),"coincident/extreme motion finite");
 const glm::vec3 origin(100000,-200000,300000);a.SetListener(origin,rotation);a.SetListenerMotion({0,0,0});a.SetPosition(tone,origin+rotation*glm::vec3(0,0,-10));a.SetMotion(tone,rotation*glm::vec3(0,0,34.33));Capture(a,4096);auto rotatedPcm=Capture(a,48000);Check(std::abs(Frequency(rotatedPcm)-1111.11)<10,"real PCM Doppler at rotated fixed-origin coordinates");a.StopAll();
 a.SetListener({0,0,0},glm::quat(1,0,0,0));
 // A high-frequency source sees real filter DSP, gain separately controlled.
 s.doppler=0;s.occlusion=true;s.occludedGain=1;s.occludedCutoff=500;auto high=a.CreateClip(Tone(8000));tone=a.CreateVoice(high,s,error);a.SetPosition(tone,{0,0,-1});a.Play(tone);auto dry=Capture(a,24000);a.SetOcclusion(tone,1);Capture(a,24000);auto wet=Capture(a,24000);Check(RMS(wet)<RMS(dry)*.15,"real obstruction low-pass suppresses high-frequency energy");s.occludedGain=.25;a.SetSettings(tone,s);Capture(a,24000);auto low=Capture(a,24000);Check(RMS(low)<RMS(wet)*.35,"occlusion gain is separate single application");s.bypass=true;a.SetSettings(tone,s);Capture(a,48000);auto bypass=Capture(a,24000);Check(RMS(bypass)>RMS(dry)*.95,"actual bypass restores dry gain and spectrum");a.StopAll();
 // Shared reverb, dry stays single. Tail exists after source stops.
 AudioData impulse;impulse.channels=2;impulse.sampleRate=48000;impulse.samples.resize(48000*2);impulse.samples[2048]=impulse.samples[2049]=.8f;auto pulse=a.CreateClip(impulse);s={};s.spatial=false;s.send=1;AudioEnvironmentSettings room;room.roomSize=.8;room.wet=.5;a.SetEnvironment(room,1);auto shot=a.CreateVoice(pulse,s,error);a.Play(shot);auto tail=Capture(a,48000);Check(RMS(tail,5000)>.00001,"impulse produces real decaying reverb tail");std::printf("REVERB tail_rms=%.9g\n",RMS(tail,5000));a.Stop(shot);auto tail2=Capture(a,48000);Check(RMS(tail2)>1e-7&&RMS(tail2)<RMS(tail,5000),"stopped source leaves a decaying bounded wet tail");Check(a.Diagnostics().reverbProcessors==1,"one shared reverb processor");for(int i=0;i<20;++i){room.roomSize=float(i%10)/10;room.damping=1-room.roomSize;a.SetEnvironment(room,float(i%2));Check(std::isfinite(RMS(Capture(a,1024))),"zone parameter changes remain finite");}a.StopAll();Check(a.Diagnostics().reverbProcessors==0,"full world Stop clears reverb state");
 ProjectAudioSettings groups;groups.groups["music"]={};groups.groups["effects"]={};a.ConfigureGroups(groups);s={};s.loop=true;s.spatial=false;s.group="music";tone=a.CreateVoice(clip,s,error);a.Play(tone);auto full=Capture(a,24000);Check(a.SetGroup("music",{.25f,false,false},.1f),"authored group fade");Capture(a,12000);auto quiet=Capture(a,12000);Check(std::abs(RMS(quiet)/RMS(full)-.25)<.01,"audio-clock fade reaches actual group gain");a.SetGroup("effects",{1,false,true});a.Snapshot(tone,snap);Check(snap.state==AudioPlaybackState::Playing,"unrelated pause does not stop music");a.SetGroup("music",{1,false,true});a.Snapshot(tone,snap);held=snap.positionSeconds;auto paused=Capture(a,1024);a.Snapshot(tone,snap);Check(snap.state==AudioPlaybackState::Paused&&snap.positionSeconds==held&&RMS(paused)==0,"group pause freezes cursor without restart/burst");a.SetGroup("music",{});Capture(a,1024);a.Snapshot(tone,snap);Check(snap.positionSeconds>held,"group resume preserves cursor");a.StopAll();
 AudioEnvironmentSettings decoded;Check(ParseAudioEnvironment(SerializeAudioEnvironment(room),decoded,error)&&decoded.roomSize==room.roomSize,"invariant reverb asset roundtrip");ProjectAudioSettings parsed;Check(ProjectAudioSettings::Parse(groups.Serialize(),parsed,error)&&parsed.groups.size()==2,"project sound groups roundtrip");
 a.StopAll();a.SetGroup("effects",{1,false,true});s.group="effects";bool dropped=true;for(int i=0;i<8;++i)dropped&=!a.PlayOneShot(clip,s,{},error).IsValid()&&error.empty();Check(dropped&&a.VoiceCount()==0,"paused group drops independent one-shot requests without allocating voices");a.SetGroup("effects",{});Check(RMS(Capture(a,1024))==0,"resume emits no deferred one-shot burst");
 ProjectSettings projectSettings;projectSettings.audio=groups;ProjectSettings projectRound;
 Check(Project::ParseFromString(Project::SerializeToString(projectSettings),projectRound,error)&&projectRound.audio.groups.size()==2,"audio groups fit normal single-line project tokenizer");
 Scene scene,round;auto& emitter=scene.CreateObject("Emitter");emitter.audioEmitter=SceneAudioEmitterComponent{};emitter.audioEmitter->loading=AudioLoading::Streamed;emitter.audioEmitter->doppler=1;emitter.audioEmitter->occlusion=true;emitter.audioEmitter->send=.4;emitter.audioEmitter->group="music";auto& zone=scene.CreateObject("Zone");zone.audioZone=SceneAudioZoneComponent{};zone.audioZone->asset="60000000000000000000000000000001";std::string text;SaveSceneToString(scene,text);Check(LoadSceneFromString(text,round,error)&&ScenesEqual(scene,round),"stream/acoustic/zone settings normal scene roundtrip");std::string before,after;ComputeSceneFingerprint(scene,before,error);round.Objects()[0].audioEmitter->doppler=2;ComputeSceneFingerprint(round,after,error);Check(before!=after,"new authored acoustic fields affect canonical fingerprint");
 Scene prefab;Check(CreatePrefab(scene,1,prefab,error),"normal audio prefab authoring");Scene instance;SceneObjectId instanceId=0;
 Check(InstantiatePrefab(instance,prefab,"60606060606060606060606060606099",{},instanceId,error),"normal linked audio prefab instance");auto previous=instance;instance.Find(instanceId)->audioEmitter->send=.8;CapturePrefabEdits(previous,instance);
 Check(instance.Find(instanceId)->prefabOverrides.count("audio.send"),"generic prefab acoustic property override captured");
 // Duration and repeated page consumption cannot grow the configured PCM pool.
 s={};s.spatial=false;s.loading=AudioLoading::Streamed;s.streamPageFrames=2048;s.loop=true;
 auto longer=a.CreateStreamVoice("projects/audio_lab/Assets/audio/music.flac",s,error);a.Play(longer);Check(WaitReady(a,longer),"actual 180-second music incremental prefill");a.Snapshot(longer,snap);Check(snap.durationSeconds>179&&snap.bufferBytes==65536,"three-times-longer file uses identical PCM allocation");
 for(int i=0;i<12;++i){a.Update();std::this_thread::sleep_for(std::chrono::milliseconds(1));std::vector<float> page(2048);a.ReadWithoutDevice(page.data(),1024);}
 a.Snapshot(longer,snap);Check(snap.bufferBytes==65536&&snap.positionSeconds>.2,"stream PCM plateaus across repeated pages");a.StopAll();Drain(a);
 for(int count:{0,8,32}){s={};s.spatial=true;s.loop=true;s.attenuation=AudioAttenuation::None;s.occlusion=true;s.send=.2;a.SetEnvironment(room,count?1:0);std::vector<AudioVoiceHandle> voices;for(int i=0;i<count;++i){auto h=a.CreateVoice(clip,s,error);a.SetPosition(h,{float(i),0,-10});a.SetOcclusion(h,i%2);a.Play(h);voices.push_back(h);}auto start=Clock::now();auto pcm=Capture(a,48000);double cost=std::chrono::duration<double,std::milli>(Clock::now()-start).count();auto stats=a.Diagnostics();std::printf("PERFORMANCE voices=%d dsp_one_second_ms=%.6f reverb=%zu buffered_bytes=%zu streams=%zu\n",count,cost,stats.reverbProcessors,stats.bufferedBytes,stats.streams);Check(std::isfinite(RMS(pcm)),"modest workload output finite");a.StopAll();}
 a.Shutdown();a.SetJobSystem(nullptr);jobs.Shutdown();std::printf("SUMMARY checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
