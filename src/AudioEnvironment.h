#pragma once
#include <string>
#include <map>
#include <glm/glm.hpp>
struct AudioEnvironmentSettings {
 float roomSize=.6f,damping=.4f,width=1,wet=.25f;
};
bool ValidAudioEnvironment(const AudioEnvironmentSettings&);
bool LoadAudioEnvironment(const std::string&,AudioEnvironmentSettings&,std::string&);
bool ParseAudioEnvironment(const std::string&,AudioEnvironmentSettings&,std::string&);
std::string SerializeAudioEnvironment(const AudioEnvironmentSettings&);
struct AudioGroupSettings {float gain=1;bool mute=false,paused=false;};
struct ProjectAudioSettings {
 std::map<std::string,AudioGroupSettings> groups;
 std::string Serialize()const;
 static bool Parse(const std::string&,ProjectAudioSettings&,std::string&);
 bool Validate(std::string&)const;
};
