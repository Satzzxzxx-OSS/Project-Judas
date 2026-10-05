#include "NavigationBackend.h"
#include "NavigationAsset.h"
#include "SceneFingerprint.h"
#include "TerrainLibrary.h"
#include "RadialTerrain.h"
#include <glm/gtc/quaternion.hpp>
#include <fstream>
#include <sstream>
#include <locale>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace {
void triangle(NavigationGeometry& g,glm::vec3 a,glm::vec3 b,glm::vec3 c){int base=g.vertices.size()/3;for(auto v:{a,b,c}){g.vertices.insert(g.vertices.end(),{v.x,v.y,v.z});}g.triangles.insert(g.triangles.end(),{base,base+1,base+2});}
void box(NavigationGeometry& g,glm::vec3 h,const SceneTransform& t,glm::vec3 offset,const SceneTransform& frame){glm::vec3 v[8];for(int i=0;i<8;++i)v[i]=glm::inverse(frame.rotation)*(t.position+t.rotation*(offset+glm::vec3((i&1)?h.x:-h.x,(i&2)?h.y:-h.y,(i&4)?h.z:-h.z))-frame.position);int faces[][4]={{0,4,6,2},{1,3,7,5},{0,1,5,4},{2,6,7,3},{0,2,3,1},{4,5,7,6}};for(auto& f:faces){triangle(g,v[f[0]],v[f[1]],v[f[2]]);triangle(g,v[f[0]],v[f[2]],v[f[3]]);}}
}
bool CollectNavigationGeometry(const Scene& scene,const SceneObject& surface,const ProjectNavigation& project,NavigationGeometry& out,std::string& error){
 if(!surface.navigationSurface||!ValidateNavigationComponents(surface,error)||!project.Validate(error)){error="invalid navigation surface/configuration: "+error;return false;}auto settings=*surface.navigationSurface;auto found=project.profiles.find(settings.profile);if(found==project.profiles.end()){error="unknown navigation agent profile";return false;}
 NavigationGeometry g;
 auto ordered=scene.Objects();std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.id<b.id;});
 for(const auto& o:ordered){
  if(o.navigationModifier&&o.navigationModifier->enabled){auto m=*o.navigationModifier;if(!project.areas.names.count(m.area)){error="unknown navigation modifier area";return false;}NavigationGeometry::Volume v;v.area=m.area;v.blocked=m.blocked;v.bottom=1e30f;v.top=-1e30f;glm::vec3 corners[8];for(int i=0;i<8;++i){auto h=m.halfExtents;corners[i]=glm::inverse(surface.transform.rotation)*(o.transform.position+o.transform.rotation*glm::vec3((i&1)?h.x:-h.x,(i&2)?h.y:-h.y,(i&4)?h.z:-h.z)-surface.transform.position);v.bottom=std::min(v.bottom,corners[i].y);v.top=std::max(v.top,corners[i].y);} // Conservative projected box hull.
   float x0=1e30f,z0=x0,x1=-x0,z1=-x0;for(auto c:corners){x0=std::min(x0,c.x);x1=std::max(x1,c.x);z0=std::min(z0,c.z);z1=std::max(z1,c.z);}v.polygon={x0,0,z0,x0,0,z1,x1,0,z1,x1,0,z0};g.modifiers.push_back(v);
  }
  if(!o.body||!o.body->enabled||o.body->sensor||o.navigationObstacle||(o.navigationModifier&&o.navigationModifier->enabled&&o.navigationModifier->excludeSource))continue;
  const auto& b=*o.body;if(!(settings.sources&CategoryBit(b.collisionLayer))||(!settings.includeDynamic&&b.motion==SceneBodyMotion::Dynamic))continue;
  if(b.shape==SceneShape::Box)box(g,b.halfExtents,o.transform,glm::vec3(0),surface.transform);
  else if(b.shape==SceneShape::Compound)for(auto part:b.compoundBoxes)box(g,part.halfExtents,o.transform,part.localCenter,surface.transform);
  else if(b.shape==SceneShape::Sphere||b.shape==SceneShape::Terrain){MeshData mesh;
   if(b.shape==SceneShape::Terrain){auto terrain=CreateTerrainSurface(b.terrainSurface);if(!terrain){error="unknown terrain navigation source";return false;}mesh=terrain->BuildMesh(96,192);}else{ // Tessellate the actual sphere collider, not its visual mesh.
    const int lat=24,lon=48;for(int j=0;j<=lat;++j)for(int i=0;i<=lon;++i){float a=3.14159265f*j/lat,p=6.2831853f*i/lon;MeshVertex v;v.position=b.radius*glm::vec3(std::sin(a)*std::cos(p),std::cos(a),std::sin(a)*std::sin(p));mesh.vertices.push_back(v);}for(int j=0;j<lat;++j)for(int i=0;i<lon;++i){unsigned a=j*(lon+1)+i,c=a+lon+1;mesh.indices.insert(mesh.indices.end(),{a,a+1,c,a+1,c+1,c});}}
   auto cv=[&](unsigned i){return glm::inverse(surface.transform.rotation)*(o.transform.position+o.transform.rotation*mesh.vertices[i].position-surface.transform.position);};for(size_t i=0;i+2<mesh.indices.size();i+=3)triangle(g,cv(mesh.indices[i]),cv(mesh.indices[i+1]),cv(mesh.indices[i+2]));
  }else{error="unsupported authoritative navigation collider shape";return false;}
 }
 if(g.triangles.empty()){error="navigation surface has no included physical geometry";return false;}
 std::ostringstream canonical;canonical.imbue(std::locale::classic());canonical<<std::setprecision(9)<<"JudasNavBake1 "<<project.Serialize()<<' ';SceneObject config=surface;config.navigationSurface->asset.clear();for(auto [k,v]:NavigationProperties(config))canonical<<k<<' '<<std::quoted(v)<<' ';for(auto v:g.vertices){if(!std::isfinite(v)){error="nonfinite navigation source";return false;}canonical<<(v==0?0:v)<<' ';}for(auto i:g.triangles)canonical<<i<<' ';for(auto v:g.modifiers){canonical<<v.area<<' '<<v.blocked<<' '<<v.bottom<<' '<<v.top<<' ';for(auto f:v.polygon)canonical<<f<<' ';}g.fingerprint=SceneFingerprintSha256(canonical.str());out=std::move(g);return true;
}
namespace {
void u32(std::vector<unsigned char>& out,unsigned n){for(int i=0;i<4;++i)out.push_back((n>>(i*8))&255);}
}
bool SaveNavigation(const std::string& path,const NavigationData& data,std::string& error){std::vector<unsigned char> bytes;std::string head="JudasNav1";bytes.insert(bytes.end(),head.begin(),head.end());std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9)<<data.fingerprint<<' '<<data.settings.profile<<' '<<data.settings.tileSize<<' '<<data.settings.cellSize<<' '<<data.settings.cellHeight<<' '<<data.settings.simplification<<' '<<data.settings.halfExtents.x<<' '<<data.settings.halfExtents.y<<' '<<data.settings.halfExtents.z<<' '<<std::quoted(data.profile.name)<<' '<<data.profile.radius<<' '<<data.profile.height<<' '<<data.profile.slope<<' '<<data.profile.climb;auto header=s.str();u32(bytes,header.size());bytes.insert(bytes.end(),header.begin(),header.end());u32(bytes,data.layers.size());for(auto& layer:data.layers){u32(bytes,layer.size());bytes.insert(bytes.end(),layer.begin(),layer.end());}std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!f){error="cannot write navigation asset "+path;return false;}return true;}
bool DecodeNavigation(const std::vector<unsigned char>& bytes,NavigationData& out,std::string& error){size_t at=9;auto number=[&](unsigned& n){if(at+4>bytes.size())return false;n=0;for(int i=0;i<4;++i)n|=unsigned(bytes[at++])<<(i*8);return true;};auto fail=[&](){error="invalid/truncated navigation asset";return false;};if(bytes.size()<13||std::string(bytes.begin(),bytes.begin()+9)!="JudasNav1")return fail();unsigned n;if(!number(n)||n>4096||at+n>bytes.size())return fail();std::istringstream s(std::string(bytes.begin()+at,bytes.begin()+at+n));s.imbue(std::locale::classic());at+=n;NavigationData d;if(!(s>>d.fingerprint>>d.settings.profile>>d.settings.tileSize>>d.settings.cellSize>>d.settings.cellHeight>>d.settings.simplification>>d.settings.halfExtents.x>>d.settings.halfExtents.y>>d.settings.halfExtents.z>>std::quoted(d.profile.name)>>d.profile.radius>>d.profile.height>>d.profile.slope>>d.profile.climb)||d.fingerprint.size()!=64)return fail();SceneObject o;o.navigationSurface=d.settings;if(!ValidateNavigationComponents(o,error))return fail();if(!number(n)||n==0||n>16384)return fail();for(unsigned i=0;i<n;++i){unsigned count;if(!number(count)||count<56||count>8*1024*1024||at+count>bytes.size())return fail();d.layers.emplace_back(bytes.begin()+at,bytes.begin()+at+count);at+=count;}for(const auto& layer:d.layers){dtTileCacheLayerHeader h{};if(layer.size()<sizeof(h))return fail();std::memcpy(&h,layer.data(),sizeof(h));if(h.magic!=DT_TILECACHE_MAGIC||h.version!=DT_TILECACHE_VERSION||h.width==0||h.height==0||h.width>d.settings.tileSize||h.height>d.settings.tileSize||h.minx>h.maxx||h.miny>h.maxy||h.maxx>=h.width||h.maxy>=h.height||h.hmin>h.hmax||layer.size()<((sizeof(h)+3)&~size_t(3))+size_t(h.width)*h.height*3)return fail();for(float x:h.bmin)if(!std::isfinite(x))return fail();for(float x:h.bmax)if(!std::isfinite(x))return fail();}
ProjectNavigation profile;profile.profiles[0]=d.profile;if(!profile.Validate(error))return fail();if(at!=bytes.size())return fail();out=std::move(d);return true;}
bool LoadNavigation(const std::string& path,NavigationData& out,std::string& error){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>128*1024*1024){error="cannot read navigation asset "+path;return false;}std::vector<unsigned char> bytes(static_cast<size_t>(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!f){error="navigation read failed";return false;}return DecodeNavigation(bytes,out,error);}
