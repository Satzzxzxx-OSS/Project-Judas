#include "AudioEnvironment.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
bool ValidAudioEnvironment(const AudioEnvironmentSettings& s){for(float v:{s.roomSize,s.damping,s.width,s.wet})if(!std::isfinite(v)||v<0||v>1)return false;return true;}
std::string SerializeAudioEnvironment(const AudioEnvironmentSettings& s){std::ostringstream o;o.imbue(std::locale::classic());o<<std::setprecision(9)<<"JudasReverb 1\n"<<s.roomSize<<' '<<s.damping<<' '<<s.width<<' '<<s.wet<<'\n';return o.str();}
bool ParseAudioEnvironment(const std::string& text,AudioEnvironmentSettings& s,std::string& error){
 std::istringstream in(text);in.imbue(std::locale::classic());std::string magic,extra;int version=0;AudioEnvironmentSettings a;
 if(!(in>>magic>>version>>a.roomSize>>a.damping>>a.width>>a.wet)||magic!="JudasReverb"||version!=1||!ValidAudioEnvironment(a)||(in>>extra)){error="Invalid JudasReverb 1 settings";return false;}s=a;error.clear();return true;
}
bool LoadAudioEnvironment(const std::string& path,AudioEnvironmentSettings& s,std::string& error){std::ifstream f(path);if(!f){error="Cannot read reverb settings: "+path;return false;}std::ostringstream o;o<<f.rdbuf();return ParseAudioEnvironment(o.str(),s,error);}
bool ProjectAudioSettings::Validate(std::string& error)const{
 if(groups.size()>16){error="At most 16 authored sound groups";return false;}
 for(auto& [name,g]:groups)if(name.empty()||name.size()>64||name=="master"||!std::isfinite(g.gain)||g.gain<0||g.gain>1){error="Invalid audio group name/gain (master is reserved)";return false;}
 error.clear();return true;
}
std::string ProjectAudioSettings::Serialize()const{std::ostringstream o;o.imbue(std::locale::classic());o<<std::setprecision(9)<<"AudioGroups 1 ";for(auto& [name,g]:groups)o<<std::quoted(name)<<' '<<g.gain<<' '<<g.mute<<' '<<g.paused<<' ';return o.str();}
bool ProjectAudioSettings::Parse(const std::string& text,ProjectAudioSettings& out,std::string& error){
 std::istringstream in(text);in.imbue(std::locale::classic());std::string magic,name;int version=0;ProjectAudioSettings result;
 if(!(in>>magic>>version)||magic!="AudioGroups"||version!=1){error="Invalid AudioGroups 1";return false;}
 while(in>>std::quoted(name)){AudioGroupSettings g;int mute=0,paused=0;if(!(in>>g.gain>>mute>>paused)||mute<0||mute>1||paused<0||paused>1||result.groups.count(name)){error="Invalid audio group";return false;}g.mute=mute;g.paused=paused;result.groups[name]=g;}
 if(!in.eof()||!result.Validate(error))return false;
 out=std::move(result);return true;
}
