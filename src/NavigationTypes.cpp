#include "NavigationTypes.h"
#include "Scene.h"
#include <iomanip>
#include <sstream>
#include <locale>
#include <cmath>
#include <type_traits>
#include <set>
namespace {
bool pos(float f){return std::isfinite(f)&&f>0;}
bool vec(glm::vec3 v){return pos(v.x)&&pos(v.y)&&pos(v.z);}
bool profile(const NavigationProfile& p){return !p.name.empty()&&pos(p.radius)&&pos(p.height)&&std::isfinite(p.slope)&&p.slope>=0&&p.slope<90&&std::isfinite(p.climb)&&p.climb>=0&&p.climb<p.height;}
template<class T>std::string text(const T& v){std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9)<<v;return s.str();}
std::string text(const glm::vec3& v){return text(v.x)+" "+text(v.y)+" "+text(v.z);}
}
bool ProjectNavigation::Validate(std::string& error)const{
 if(!areas.Validate(error))return false;
 if(areas.names.empty()||!areas.names.count(0)||areas.nextId>62||profiles.empty()||profiles.size()>64||nextProfile>64){error="navigation requires Default area and 1..64 profiles; up to 62 stable areas";return false;}
 std::set<std::string> names;
 for(auto [id,p]:profiles)if(!names.insert(p.name).second||id>=nextProfile||!profile(p)){error="invalid navigation profile";return false;}
 for(auto [id,n]:areas.names)if(id>=62){error="navigation area ID exceeds 61";return false;}
 return true;
}
std::string ProjectNavigation::Serialize()const{std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9)<<"JudasNavigationProject 1 "<<areas.nextId<<' '<<areas.names.size();for(auto [id,n]:areas.names)s<<' '<<id<<' '<<std::quoted(n);s<<' '<<nextProfile<<' '<<profiles.size();for(auto [id,p]:profiles)s<<' '<<id<<' '<<std::quoted(p.name)<<' '<<p.radius<<' '<<p.height<<' '<<p.slope<<' '<<p.climb;return s.str();}
bool ProjectNavigation::Parse(const std::string& data,ProjectNavigation& out,std::string& error){std::istringstream s(data);s.imbue(std::locale::classic());std::string magic;int version;size_t count;ProjectNavigation p;p.areas.names.clear();p.profiles.clear();if(!(s>>magic>>version>>p.areas.nextId>>count)||magic!="JudasNavigationProject"||version!=1||count>62){error="invalid navigation project configuration";return false;}for(size_t i=0;i<count;++i){unsigned id;std::string n;if(!(s>>id>>std::quoted(n))||!p.areas.names.emplace(id,n).second){error="duplicate/invalid navigation area";return false;}}if(!(s>>p.nextProfile>>count)||count>64)return false;for(size_t i=0;i<count;++i){unsigned id;NavigationProfile a;if(!(s>>id>>std::quoted(a.name)>>a.radius>>a.height>>a.slope>>a.climb)||!p.profiles.emplace(id,a).second){error="invalid navigation profile";return false;}}s>>std::ws;if(!s.eof()||!p.Validate(error))return false;out=p;return true;}
std::map<std::string,std::string> NavigationProperties(const SceneObject& o){std::map<std::string,std::string> p;
#define FIELD(prefix,obj,name) p["nav." prefix "." #name]=text(obj.name)
 if(o.navigationSurface){const auto& a=*o.navigationSurface;
#define S(name) FIELD("surface",a,name)
 S(enabled);S(includeDynamic);S(profile);S(sources);S(halfExtents);S(cellSize);S(cellHeight);S(simplification);S(tileSize);S(minRegion);p["nav.surface.asset"]=a.asset;
#undef S
 }
 if(o.navigationAgent){const auto& a=*o.navigationAgent;
#define A(name) FIELD("agent",a,name)
 A(enabled);A(avoidance);A(profile);A(areas);A(speed);A(arrival);A(cornerDistance);A(repathSeconds);std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9);for(auto [id,c]:a.costs)s<<id<<' '<<c<<' ';p["nav.agent.costs"]=s.str();
#undef A
 }
 if(o.navigationObstacle){const auto& a=*o.navigationObstacle;
#define O(name) FIELD("obstacle",a,name)
 O(enabled);O(cylinder);O(halfExtents);O(radius);O(height);O(updateDistance);
#undef O
 }
 if(o.navigationLink){const auto& a=*o.navigationLink;
#define L(name) FIELD("link",a,name)
 L(enabled);L(bidirectional);L(start);L(end);L(radius);L(area);
#undef L
 }
 if(o.navigationModifier){const auto& a=*o.navigationModifier;
#define M(name) FIELD("modifier",a,name)
 M(enabled);M(excludeSource);M(blocked);M(halfExtents);M(area);
#undef M
 }
#undef FIELD
 return p;
}
bool ApplyNavigationProperties(const std::map<std::string,std::string>& p,SceneObject& o,std::string& error){auto read=[&](const std::string& k,auto& value){auto it=p.find(k);if(it==p.end()){error="missing "+k;return false;}std::istringstream s(it->second);s.imbue(std::locale::classic());if constexpr(std::is_same_v<std::decay_t<decltype(value)>,glm::vec3>)s>>value.x>>value.y>>value.z;else s>>value;if(s.fail()){error="invalid "+k;return false;}s>>std::ws;if(!s.eof()){error="invalid "+k;return false;}return true;};
#define READ(prefix,obj,name) if(!read("nav." prefix "." #name,obj.name))return false
 if(p.count("nav.surface.enabled")){NavigationSurfaceSettings a;
#define S(name) READ("surface",a,name)
 S(enabled);S(includeDynamic);S(profile);S(sources);S(halfExtents);S(cellSize);S(cellHeight);S(simplification);S(tileSize);S(minRegion);if(!p.count("nav.surface.asset")){error="missing surface asset field";return false;}a.asset=p.at("nav.surface.asset");o.navigationSurface=a;
#undef S
 }
 if(p.count("nav.agent.enabled")){NavigationAgentSettings a;
#define A(name) READ("agent",a,name)
 A(enabled);A(avoidance);A(profile);A(areas);A(speed);A(arrival);A(cornerDistance);A(repathSeconds);if(!p.count("nav.agent.costs")){error="missing navigation costs";return false;}std::istringstream s(p.at("nav.agent.costs"));s.imbue(std::locale::classic());unsigned id;float cost;while(s>>id){if(!(s>>cost)||!a.costs.emplace(id,cost).second){error="invalid navigation costs";return false;}}o.navigationAgent=a;
#undef A
 }
 if(p.count("nav.obstacle.enabled")){NavigationObstacleSettings a;
#define O(name) READ("obstacle",a,name)
 O(enabled);O(cylinder);O(halfExtents);O(radius);O(height);O(updateDistance);o.navigationObstacle=a;
#undef O
 }
 if(p.count("nav.link.enabled")){NavigationLinkSettings a;
#define L(name) READ("link",a,name)
 L(enabled);L(bidirectional);L(start);L(end);L(radius);L(area);o.navigationLink=a;
#undef L
 }
 if(p.count("nav.modifier.enabled")){NavigationModifierSettings a;
#define M(name) READ("modifier",a,name)
 M(enabled);M(excludeSource);M(blocked);M(halfExtents);M(area);o.navigationModifier=a;
#undef M
 }
#undef READ
 auto expected=NavigationProperties(o);if(expected.size()!=p.size()){error="unknown/incomplete navigation fields";return false;}for(const auto& v:p)if(!expected.count(v.first)){error="unknown navigation field "+v.first;return false;}return ValidateNavigationComponents(o,error);
}
bool ValidateNavigationComponents(const SceneObject& o,std::string& error){bool valid=true;
 if(o.navigationSurface){auto a=*o.navigationSurface;valid&=vec(a.halfExtents)&&pos(a.cellSize)&&pos(a.cellHeight)&&pos(a.simplification)&&a.cellSize>=.02f&&a.cellHeight>=.01f&&a.tileSize>=16&&a.tileSize<=128&&a.minRegion>=0&&a.minRegion<=100;}
 if(o.navigationAgent){auto a=*o.navigationAgent;valid&=pos(a.speed)&&pos(a.arrival)&&pos(a.cornerDistance)&&pos(a.repathSeconds);for(auto [id,c]:a.costs)valid&=id<62&&pos(c)&&c>=1;}
 if(o.navigationObstacle){auto a=*o.navigationObstacle;valid&=vec(a.halfExtents)&&pos(a.radius)&&pos(a.height)&&pos(a.updateDistance);}
 if(o.navigationLink){auto a=*o.navigationLink;valid&=a.area<62&&pos(a.radius)&&std::isfinite(glm::dot(a.start,a.start)+glm::dot(a.end,a.end));}
 if(o.navigationModifier){auto a=*o.navigationModifier;valid&=a.area<62&&vec(a.halfExtents);}
 if(!valid)error="invalid navigation component settings";
 return valid;
}
